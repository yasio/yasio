# Install latest cmake version for appveyor ci
# refer to: https://docs.github.com/en/actions/learn-github-actions/environment-variables

# options
#  -a: build arch: x86,x64,arm,arm64
#  -p: build target platform: win,uwp,linux,android,osx(macos),ios,tvos,watchos
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

# 0: windows, 1: linux, 2: macos
if ($IsWindows -or ("$env:OS" -eq 'Windows_NT')) {
    $hostOS = 0
}
elseif($IsLinux) {
    $hostOS = 1
}
elseif($IsMacOS) {
    $hostOS = 2
}
else {
    Write-Error "Unsupported host OS for building target $(options.p)"
    exit 1
}

$exeSuffix = if ($hostOS -eq 0) {'.exe'} else {''}
$myHome = (Resolve-Path ~).Path

function setup_ninja($addToPath = $False) {
    $ninja_prog=(Get-Command "ninja" -ErrorAction SilentlyContinue).Source
    if ($ninja_prog) {
        Write-Host "Using system installed ninja: $ninja_prog"
        return $ninja_prog
    }
    # install ninja
    $osName = $('win', 'linux', 'mac').Get($hostOS)
    $ninja_bin = (Resolve-Path "$myHome/ninja-$osName" -ErrorAction SilentlyContinue).Path
    if (!$ninja_bin) {
        
        curl -L "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-$osName.zip" -o $myHome/ninja-$osName.zip 
        # unzip ~/ninja-$osName.zip -d ~
        Expand-Archive -Path $myHome/ninja-$osName.zip -DestinationPath "$myHome/ninja-$osName/"
        & $ninja_bin/ninja --version
        $ninja_bin = (Resolve-Path "$myHome/ninja-$osName" -ErrorAction SilentlyContinue).Path
    }
    if ($addToPath) {
        if ($env:PATH.IndexOf($ninja_bin) -ne -1) {
            $env:Path = "$ninja_bin;$env:Path"
        }
    }

    $ninja_proj = (Join-Path -Path $ninja_bin -ChildPath ninja$exeSuffix)
    return $ninja_proj
}

function setup_ndk() {
    # install ndk
    if ($env:GITHUB_ACTIONS -eq "true") {
        Write-Host "Using github action ndk ..."
        Write-Host "ANDROID_NDK=$env:ANDROID_NDK"
        Write-Host "ANDROID_NDK_HOME=$env:ANDROID_NDK_HOME"
        Write-Host "ANDROID_NDK_ROOT=$env:ANDROID_NDK_ROOT"

        $ndk_root = $env:ANDROID_NDK
        if ($ndk_root -eq '') {
            $ndk_root = $env:ANDROID_NDK_HOME
            if ($ndk_root -eq '') {
                $ndk_root = $env:ANDROID_NDK_ROOT
            }
        }
    } elseif("$env:ANDROID_HOME" -ne '') {
        # find ndk in sdk
        foreach($item in $(Get-ChildItem -Path "$env:ANDROID_HOME/ndk")) {
            $sourceProps = "$item/source.properties"
            if (Test-Path $sourceProps -PathType Leaf) {
                $verLine = $(Get-Content $sourceProps | Select-Object -Index 1)
                $ndk_rev = $($verLine -split '=').Trim()[1]
                if ($ndk_rev -ge "19.0") {
                    $ndk_root = $item.ToString()
                    break
                }
            }
        }
    }
    
    if (Test-Path "$ndk_root" -PathType Container)
    {
        Write-Host "Using exist ndk: $ndk_root ..."
    }
    else {  
        # since r23 no suffix
        $osName = $('windows', 'linux', 'darwin').Get($hostOS)
        $suffix=if ("$env:NDK_VER" -le "r22z") {'-x86_64'} else {''}
        $ndk_package="android-ndk-$env:NDK_VER-$osName$suffix"
        Write-Host "Downloading ndk package $ndk_package ..."
        curl -o $myHome/$ndk_package.zip https://dl.google.com/android/repository/$ndk_package.zip
        Expand-Archive -Path $myHome/$ndk_package.zip -DestinationPath $myHome/
        $ndk_root=$myHome/$ndk_package
    }

    return $ndk_root
}

$ndk_root = setup_ndk

# build methods
function build_win() {
    Write-Output "Building target $($options.p) on windows ..."
    if ($options.cc -eq '') {
        $options.cc = 'msvc'
    }

    $toolchain = $options.cc

    if ($toolchain -ne 'msvc') { # install ninja for non msvc compilers
        setup_ninja($True)
        ninja --version
    }

    $cmake_ver=$($(cmake --version | Select-Object -First 1) -split ' ')[2]
    Write-Output "Checking cmake version: $cmake_ver"
    if ($cmake_ver -lt '3.13.0') {
        $cmake_ver = '3.27.0-rc2'
        Write-Output "The cmake too old, installing cmake-$cmake_ver ..."
        if (!(Test-Path ".\cmake-$cmake_ver-windows-x86_64" -PathType Container)) {
            if ($pwsh_ver -lt '7.0')  {
                curl "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip" -o "cmake-$cmake_ver-windows-x86_64.zip"
            } else {
                curl -L "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip" -o "cmake-$cmake_ver-windows-x86_64.zip"
            }
            Expand-Archive -Path cmake-$cmake_ver-windows-x86_64.zip -DestinationPath .\
        }
        $cmake_bin = (Resolve-Path .\cmake-$cmake_ver-windows-x86_64\bin).Path
        if ($env:PATH.IndexOf($cmake_bin) -eq -1) {
            $env:Path = "$cmake_bin;$env:Path"
        }
        cmake --version
    }

    $CONFIG_ALL_OPTIONS=@()
    if ($toolchain -eq 'msvc') { # Generate vs2019 on github ci
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
                $CONFIG_ALL_OPTIONS += '-G', "Visual Studio 12 2013", '-DYASIO_BUILD_WITH_LUA=ON'
            }
        }
    }
    elseif($toolchain -eq 'clang') {
        clang --version
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++', '-DYASIO_SSL_BACKEND=1'
        cmake -B build $CONFIG_ALL_OPTIONS
    }
    else { # Generate mingw
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config'
    }

    if ($options -ne 'msvc') {
        # requires c++17 to build example 'ftp_server' for non-msvc compilers
        $CONFIG_ALL_OPTIONS += '-DCXX_STD=17'
    }
   
    Write-Output ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)

    $build_dir="build_$toolchain"

    # Configure
    cmake -B $build_dir $CONFIG_ALL_OPTIONS
    # Build
    cmake --build $build_dir --config Release

    if (($options.p -ne 'uwp') -and ($options.cc -ne 'mingw-gcc')) {
        Write-Output "run icmptest on windows ..."
        Invoke-Expression -Command ".\$build_dir\tests\icmp\Release\icmptest.exe $env:PING_HOST"
    }
}

function build_linux() {
    Write-Output "Building linux ..."

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

function build_andorid() {
    Write-Output "Building android ..."

    $ninja_prog = setup_ninja
    $ndk_root = setup_ndk
    
    # building
    $arch=$options.a
    if ($arch -eq 'arm') {
        $arch = 'armeabi-v7a'
    } elseif($arch -eq 'arm64') {
        $arch = 'arm64-v8a'
    } elseif($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    cmake -G "Ninja" -B build "-DANDROID_STL=c++_shared" "-DCMAKE_MAKE_PROGRAM=$ninja_prog" "-DCMAKE_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake" -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DANDROID_PLATFORM=16 "-DANDROID_ABI=$arch" -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake --build build --config Release 
}

function build_osx() {
    Write-Output "Building $($options.p) ..."
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }

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

# build ios famliy (ios,tvos,watchos)
function build_ios() {
    Write-Output "Building $($options.p) ..."
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    if ($options.p -eq 'ios') {
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

$builds = @{ 
    'win' = ${function:build_win};
    'uwp' = ${function:build_win};
    'linux' = ${function:build_linux}; 
    'android' = ${function:build_andorid};
    'osx' = ${function:build_osx};
    'ios' = ${function:build_ios};
    'tvos' = ${function:build_ios};
    'watchos' = ${function:build_ios};
}

& $builds[$options.p]
