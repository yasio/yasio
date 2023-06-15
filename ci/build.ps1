# Install latest cmake version for appveyor ci
# refer to: https://docs.github.com/en/actions/learn-github-actions/environment-variables

# options
#  -arch: build arch: x86,x64,arm,arm64
#  -plat: build target platform: win,uwp,linux,android,osx(macos),ios,tvos,watchos
#  -cc: compiler id: clang, msvc, gcc, mingw-gcc or empty use default installed on current OS
#  -t: toolset name for visual studio: v12,v141,v143

$options = @{ a = 'x64'; p = 'win'; cc = ''; t = ''}

$optName = $null
foreach ($arg in $args) {
    if (!$optName) {
        if ($arg.StartsWith('-')) {
            $optName = $arg.SubString(1)
        }
    } else {
        if ($options.Contains($optName)) {
            $options[$optName] = $arg
        } else {
            Write-Output "Warning: ignore unrecognized option: $optName"
        }
        $optName = $null
    }
}

$pwsh_ver = $PSVersionTable.PSVersion.ToString()

Write-Output "PowerShell $pwsh_ver"

Write-Output $options

$yasio_root = (Resolve-Path "$PSScriptRoot/..").Path

Write-Output "yasio_root=$yasio_root"

if ($IsWindows) { # On Windows, we can build for target win, winuwp, mingw
    if ($options.cc -eq '') {
        $options.cc = 'msvc'
    }

    if ($options.cc -ne 'msvc') { # install ninja for non msvc compilers
        if(!(Get-Command "ninja" -ErrorAction SilentlyContinue)) {
            Write-Output "Install ninja ..."
            $ninja_ver='1.11.1'
            if (!(Test-Path '.\ninja-win' -PathType Container)) {
                curl -L "https://github.com/ninja-build/ninja/releases/download/v$ninja_ver/ninja-win.zip" -o "ninja-win.zip"
                Expand-Archive -Path ninja-win.zip -DestinationPath .\ninja-win\
            }
            $ninja_bin = (Resolve-Path .\ninja-win).Path
            if ($env:PATH.IndexOf($ninja_bin) -ne -1) {
                $env:Path = "$ninja_bin;$env:Path"
            }
        }
        ninja --version
    }

    if (!($env:GITHUB_ACTIONS -eq "true")) {
        $cmake_ver="3.27.0-rc2"
        if (!(Test-Path ".\cmake-$cmake_ver-windows-x86_64" -PathType Container)) {
            if ($pwsh_ver -lt '7.0')  {
                curl "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip" -o "cmake-$cmake_ver-windows-x86_64.zip"
            } else {
                curl -L "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip" -o "cmake-$cmake_ver-windows-x86_64.zip"
            }
            Expand-Archive -Path cmake-$cmake_ver-windows-x86_64.zip -DestinationPath .\
        }
        $cmake_bin = (Resolve-Path .\cmake-$cmake_ver-windows-x86_64\bin).Path
        if ($env:PATH.IndexOf($cmake_bin) -ne -1) {
            $env:Path = "$cmake_bin;$env:Path"
        }
    }

    cmake --version
    $CONFIG_ALL_OPTIONS=@()
    if ($options.cc -eq 'msvc') { # Generate vs2019 on github ci
        # Determine arch name
        $arch=""
        if ($options.a -eq "x86") {
            $arch="Win32"
        }
        else {
            $arch=$options.a
        }
        
        if($options.t -eq '') {
            if ($options.p -eq "uwp") {
                $CONFIG_ALL_OPTIONS += '-A', $arch, '-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0', '-DBUILD_SHARED_LIBS=ON', '-DYAISO_BUILD_NI=ON', '-DYASIO_SSL_BACKEND=0'
            }
            else {
                $CONFIG_ALL_OPTIONS += '-A', $arch
            }
        } else { # vs2013
            if ($options.a -eq "x64") {
                $CONFIG_ALL_OPTIONS += '-G', "Visual Studio 12 2013 Win64", '-DYASIO_BUILD_WITH_LUA=ON'
            }
            else {
                $CONFIG_ALL_OPTIONS += 'G', "Visual Studio 12 2013", '-DYASIO_BUILD_WITH_LUA=ON'
            }
        }
    }
    elseif($options.cc -eq 'clang') {
        clang --version
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++', '-DYASIO_SSL_BACKEND=1'
        cmake -B build $CONFIG_ALL_OPTIONS
    }
    else { # Generate mingw
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config'
    }
   
    Write-Output ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)

    # Configure
    cmake -B build $CONFIG_ALL_OPTIONS
    # Build
    cmake --build build --config Release

    if ($options.p -ne 'uwp') {
        Write-Output "run icmptest on windows ..."
        Invoke-Expression -Command ".\build\tests\icmp\Release\icmptest.exe $env:PING_HOST"
    }
}
elseif($IsLinux) { # On Linux, we build targets: android, linux
    if ($options.p -eq 'linux') {
        Write-Output "Building linux..."
        cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=2 -DYASIO_USE_CARES=ON -DYASIO_ENABLE_ARES_PROFILER=ON -DYAISO_BUILD_NI=YES -DCXX_STD=17 -DYASIO_BUILD_WITH_LUA=ON -DBUILD_SHARED_LIBS=ON
    
        cmake --build build -- -j $(nproc)

        if ($env:GITHUB_ACTIONS -eq "true") {
            Write-Output "run issue201 on linux..."
            ./build/tests/issue201/issue201
            
            Write-Output "run httptest on linux..."
            ./build/tests/http/httptest
        
            Write-Output "run ssltest on linux..."
            ./build/tests/ssl/ssltest
        
            Write-Output "run icmp test on linux..."
            ./build/tests/icmp/icmptest $env:PING_HOST
        }
    }
    elseif($options.p -eq 'android') {
        # install ninja
        wget -O ~/ninja-linux.zip 'https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip'
        unzip ~/ninja-linux.zip -d ~
        ~/ninja --version

        # install ndk
        if ($env:GITHUB_ACTIONS -eq "true") {
            Write-Output "Using github action ndk ..."
            Write-Output "ANDROID_NDK=$env:ANDROID_NDK"
            Write-Output "ANDROID_NDK_HOME=$env:ANDROID_NDK_HOME"
            Write-Output "ANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT"

            $ndk_root = $env:ANDROID_NDK
            if ($ndk_root -eq '') {
                $ndk_root = $env:ANDROID_NDK_HOME
                if ($ndk_root -eq '') {
                    $ndk_root = $env:ANDROID_NDK_ROOT
                }
            }
        }
        
        if ($ndk_root -eq '') {
            # since r23 no suffix
            $suffix=if ("$env:NDK_VER" -le "r22z") {'x86_64'} else {''}
            $ndk_package="android-ndk-$env:NDK_VER-linux$suffix"
            Write-Output "Downloading ndk package $ndk_package ..."
            curl -o ~/$ndk_package.zip https://dl.google.com/android/repository/$ndk_package.zip
            unzip -q ~/$ndk_package.zip -d ~
            $ndk_root=~/$ndk_package
        }

        # building
        $arch=$options.a
        if ($arch -eq 'arm') {
            $arch = 'armeabi-v7a'
        } elseif($arch -eq 'arm64') {
            $arch = 'arm64-v8a'
        } elseif($arch -eq 'x64') {
            $arch = 'x86_64'
        }
        $NINJA_PATH="$home/ninja"
        cmake -G "Ninja" -B build "-DANDROID_STL=c++_shared" "-DCMAKE_MAKE_PROGRAM=$NINJA_PATH" "-DCMAKE_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake" -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang "-DANDROID_ABI=$arch" -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
        cmake --build build --target yasio
        cmake --build build --target yasio_http
    }
}
elseif($IsMacOS) { # On macOS, we build targets: osx(macos),ios,tvos,watchos
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    if ($options.p -eq 'osx') {
        Write-Output "Building osx..."
        cmake -GXcode -Bbuild -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON "-DCMAKE_OSX_ARCHITECTURES=$arch"
        cmake --build build --config Release

        if (($env:GITHUB_ACTIONS -eq "true") -and ($options.a -eq 'x64')) {
            Write-Output "run test tcptest on osx ..."
            ./build/tests/tcp/Release/tcptest
            
            Write-Output "run test issue384 on osx ..."
            ./build/tests/issue384/Release/issue384

            Write-Output "run test icmp on osx ..."
            ./build/tests/icmp/Release/icmptest $env:PING_HOST
        }
    }
    elseif ($options.p -eq 'ios') {
        Write-Output "Building iOS..."
        cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$yasio_root/cmake/ios.cmake" "-DARCHS=$arch" -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
        cmake --build build --config Release
    } 
    elseif ($options.p -eq 'tvos') {
        Write-Output "Building tvOS..."
        cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$yasio_root/cmake/ios.cmake" "-DARCHS=$arch" -DPLAT=tvOS -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
        cmake --build build --config Release
    }
    elseif ($options.p -eq 'watchos') {
        Write-Output "Building  watchOS..."
        cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$yasio_root/cmake/ios.cmake" "-DARCHS=$arch" -DPLAT=watchOS -DYASIO_SSL_BACKEND=0 -DYASIO_USE_CARES=ON
        cmake --build build --config Release
    }  
}
