# //////////////////////////////////////////////////////////////////////////////////////////
# // A multi-platform support c++11 library with focus on asynchronous socket I/O for any
# // client application.
# //////////////////////////////////////////////////////////////////////////////////////////
# 
# The MIT License (MIT)
# 
# Copyright (c) 2012-2023 HALX99
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
# 

# The build.ps1 for ci, support all target platforms
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
            Write-Host "Warning: ignore unrecognized option: $optName"
        }
        $optName = $null
    }
}

$pwsh_ver = $PSVersionTable.PSVersion.ToString()

Write-Host "PowerShell $pwsh_ver"
Write-Host $(Out-String -InputObject $options)

$yasio_root = (Resolve-Path "$PSScriptRoot/..").Path
$yasio_tools = Join-Path -Path $yasio_root -ChildPath 'tools'

# The preferred cmake version to install when system installed cmake < 3.13.0
$cmake_ver = '3.26.4'

Set-Location $yasio_root

Write-Host "yasio_root=$yasio_root"

$HOST_WIN   = 0 # targets: win,uwp,android
$HOST_LINUX = 1 # targets: linux,android 
$HOST_OSX   = 2 # targets: android,ios,osx(macos),tvos,watchos

# 0: windows, 1: linux, 2: macos
if ($IsWindows -or ("$env:OS" -eq 'Windows_NT')) {
    $hostOS = $HOST_WIN
    $envPathSep = ';'
}
else {
    $envPathSep = ':'
    if($IsLinux) {
        $hostOS = $HOST_LINUX
    }
    elseif($IsMacOS) {
        $hostOS = $HOST_OSX
    }
    else {
        Write-Error "Unsupported host OS for building target $(options.p)"
        exit 1
    }
}

$exeSuffix = if ($hostOS -eq 0) {'.exe'} else {''}

if (!(Test-Path "$yasio_tools" -PathType Container)) {
    mkdir $yasio_tools
}

# for ci check, enable high preformance platform I/O multiplexing
if ($env:GITHUB_ACTIONS -eq 'true') {
    Write-Host "Enable high performance I/O multiplexing for ci checks"
    $CONFIG_ALL_OPTIONS = @('-DYASIO_ENABLE_HPERF_IO=1')
} else {
    $CONFIG_ALL_OPTIONS = @()
}

# now windows only
function setup_cmake() {
    $cmake_prog=(Get-Command "cmake" -ErrorAction SilentlyContinue).Source
    if ($cmake_prog) {
        $_cmake_ver = $($(cmake --version | Select-Object -First 1) -split ' ')[2]
    } else {
        $_cmake_ver = '0.0.0'
    }
    if ($_cmake_ver -ge '3.13.0') {
        Write-Host "Using system installed cmake $cmake_prog, version: $_cmake_ver"
    } else {
        
        Write-Host "The installed cmake $_cmake_ver too old, installing newer $cmake_ver ..."

        $hostName = $('windows', 'linux', 'macos').Get($hostOS)
        $cmake_root = $(Join-Path -Path $yasio_tools -ChildPath "cmake-$cmake_ver-$hostName-x86_64")
        if (!(Test-Path $cmake_root -PathType Container)) {
            Write-Host "Downloading cmake-$cmake_ver-windows-x86_64.zip ..."
            if ($hostOS -eq $HOST_WIN) {
                $cmake_url = "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip"
                if ($pwsh_ver -lt '7.0')  {
                    curl $cmake_url -o "$cmake_root.zip"
                } else {
                    curl -L $cmake_url -o "$cmake_root.zip"
                }
                Expand-Archive -Path "$cmake_root.zip" -DestinationPath $yasio_tools\
            }
            elseif($hostOS -eq $HOST_LINUX) {
                if (!(Test-Path "$cmake_root.sh" -PathType Leaf)) {
                    $cmake_url = "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-linux-x86_64.sh"
                    curl -L $cmake_url -o "$cmake_root.sh"
                }
                chmod 'u+x' "$cmake_root.sh"
                mkdir $cmake_root
                & "$cmake_root.sh" '--skip-license' '--exclude-subdir' "--prefix=$cmake_root"
            }
        }
        $cmake_bin = Join-Path -Path $cmake_root -ChildPath 'bin'
        if ($env:PATH.IndexOf($cmake_bin) -eq -1) {
            $env:PATH = "$cmake_bin$envPathSep$env:PATH"
        }
        $cmake_prog=(Get-Command "cmake" -ErrorAction SilentlyContinue).Source
        if ($cmake_prog) {
            $_cmake_ver = $($(cmake --version | Select-Object -First 1) -split ' ')[2]
        }
        if ($_cmake_ver -ge '3.13.0') {
            Write-Host "Install cmake $_cmake_ver succeed"
        }
        else {
            Write-Host "Install cmake $_cmake_ver fail"
            exit 1
        }
    }
}

function setup_ninja() {
    $ninja_prog=(Get-Command "ninja" -ErrorAction SilentlyContinue).Source
    if (!$ninja_prog) {
        $hostName = $('win', 'linux', 'mac').Get($hostOS)
        $ninja_bin = (Resolve-Path "$yasio_tools/ninja-$hostName" -ErrorAction SilentlyContinue).Path
        if (!$ninja_bin) {
            Write-Host "Downloading ninja-$hostName.zip ..."
            curl -L "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-$hostName.zip" -o $yasio_tools/ninja-$hostName.zip
            Expand-Archive -Path $yasio_tools/ninja-$hostName.zip -DestinationPath "$yasio_tools/ninja-$hostName/"
            $ninja_bin = (Resolve-Path "$yasio_tools/ninja-$hostName" -ErrorAction SilentlyContinue).Path
        }
        if ($env:PATH.IndexOf($ninja_bin) -eq -1) {
            $env:PATH = "$ninja_bin$envPathSep$env:PATH"
        }
        $ninja_prog = (Join-Path -Path $ninja_bin -ChildPath ninja$exeSuffix)
    } else {
        Write-Host "Using system installed ninja: $ninja_prog"
    }
    Write-Host (ninja --version)
    return $ninja_prog
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
        $ndk_ver = $env:NDK_VER
        if ("$ndk_ver" -eq '') { $ndk_ver = 'r19c' }
        $hostName = $('windows', 'linux', 'darwin').Get($hostOS)
        $suffix = if ("$ndk_ver" -le "r22z") {'-x86_64'} else {''}
        $ndk_root = "$yasio_tools/android-ndk-$ndk_ver"
        if (!(Test-Path "$ndk_root" -PathType Container)) {
            Write-Host "Downloading ndk package $ndk_package ..."
            $ndk_package="android-ndk-$ndk_ver-$hostName$suffix"
            curl -o $yasio_tools/$ndk_package.zip https://dl.google.com/android/repository/$ndk_package.zip
            Expand-Archive -Path $yasio_tools/$ndk_package.zip -DestinationPath $yasio_tools/
        }
    }

    return $ndk_root
}

# build methods
function build_win {
    Param(
        [string[]]$cmakeOptions
    )
    $CONFIG_ALL_OPTIONS = $cmakeOptions

    $CONFIG_ALL_OPTIONS += '-DYASIO_ENABLE_WEPOLL=1'

    Write-Host "Building target $($options.p) on windows ..."
    if ($options.cc -eq '') {
        $options.cc = 'msvc'
    }

    $toolchain = $options.cc
    
    if ($toolchain -ne 'msvc') { # install ninja for non msvc compilers
        setup_ninja
    }

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
                $CONFIG_ALL_OPTIONS += '-A', $arch, '-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0', '-DBUILD_SHARED_LIBS=OFF', '-DYAISO_BUILD_NI=ON', '-DYASIO_SSL_BACKEND=0'
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
            # openssl prebuilt was built from vs2022, so we set ssl backend to use mbedtls-2.28.3
            # Notes: mbedtls-3.x no longer support compile with vs2013, will encounter many compiling errors
            $CONFIG_ALL_OPTIONS += '-DYASIO_SSL_BACKEND=2'
        }
    }
    elseif($toolchain -eq 'clang') {
        clang --version
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'
        cmake -B build $CONFIG_ALL_OPTIONS
    }
    else { # Generate mingw
        $CONFIG_ALL_OPTIONS += '-G', 'Ninja Multi-Config'
    }

    if ($options -ne 'msvc') {
        # requires c++17 to build example 'ftp_server' for non-msvc compilers
        $CONFIG_ALL_OPTIONS += '-DCXX_STD=17'
    }
   
    Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)

    $build_dir="build_$toolchain"

    # Configure
    cmake -B $build_dir $CONFIG_ALL_OPTIONS
    # Build
    cmake --build $build_dir --config Release

    if (($options.p -ne 'uwp') -and ($options.cc -ne 'mingw-gcc')) {
        Write-Host "run icmptest on windows ..."
        Invoke-Expression -Command ".\$build_dir\tests\icmp\Release\icmptest.exe $env:PING_HOST"
    }
}

function build_linux {
    Param(
        [string[]]$cmakeOptions
    )
    $CONFIG_ALL_OPTIONS = $cmakeOptions

    Write-Host "Building linux ..."

    $CONFIG_ALL_OPTIONS += '-DCMAKE_BUILD_TYPE=Release', '-DYASIO_USE_CARES=ON', '-DYASIO_ENABLE_ARES_PROFILER=ON', '-DYAISO_BUILD_NI=YES', '-DCXX_STD=17', '-DYASIO_BUILD_WITH_LUA=ON', '-DBUILD_SHARED_LIBS=ON'
    Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)
    cmake -Bbuild $CONFIG_ALL_OPTIONS
    cmake --build build -- -j $(nproc)

    if ($env:GITHUB_ACTIONS -eq "true") {
        Write-Host "run issue201 on linux..."
        ./build/tests/issue201/issue201
        
        Write-Host "run httptest on linux..."
        ./build/tests/http/httptest
    
        Write-Host "run ssltest on linux..."
        ./build/tests/ssl/ssltest
    
        Write-Host "run icmp test on linux..."
        ./build/tests/icmp/icmptest $env:PING_HOST
    }
}

function build_andorid {
    Param(
        [string[]]$cmakeOptions
    )
    $CONFIG_ALL_OPTIONS = $cmakeOptions

    Write-Host "Building android ..."

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
    $CONFIG_ALL_OPTIONS += '-G', 'Ninja', '-DANDROID_STL=c++_shared', "-DCMAKE_MAKE_PROGRAM=$ninja_prog", "-DCMAKE_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake", "-DANDROID_ABI=$arch", '-DCMAKE_BUILD_TYPE=Release', '-DYASIO_USE_CARES=ON'
    $CONFIG_ALL_OPTIONS += '-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH'
    $CONFIG_ALL_OPTIONS += '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH'
    $CONFIG_ALL_OPTIONS += '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH'
    # by default, we want find host program only when cross-compiling
    $CONFIG_ALL_OPTIONS += '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER'
    Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)
    cmake -B build_a $CONFIG_ALL_OPTIONS
    cmake --build build_a --config Release 
}

function build_osx {
    Param(
        [string[]]$cmakeOptions
    )
    $CONFIG_ALL_OPTIONS = $cmakeOptions

    Write-Host "Building $($options.p) ..."
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }

    $CONFIG_ALL_OPTIONS += '-GXcode', '-DYASIO_USE_CARES=ON', "-DCMAKE_OSX_ARCHITECTURES=$arch"
    Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)
    cmake -Bbuild $CONFIG_ALL_OPTIONS
    cmake --build build --config Release

    if (($env:GITHUB_ACTIONS -eq "true") -and ($options.a -eq 'x64')) {
        Write-Host "run test tcptest on osx ..."
        ./build/tests/tcp/Release/tcptest
        
        Write-Host "run test issue384 on osx ..."
        ./build/tests/issue384/Release/issue384

        Write-Host "run test icmp on osx ..."
        ./build/tests/icmp/Release/icmptest $env:PING_HOST
    }
}

# build ios famliy (ios,tvos,watchos)
function build_ios {
    Param(
        [string[]]$cmakeOptions
    )
    $CONFIG_ALL_OPTIONS = $cmakeOptions

    Write-Host "Building $($options.p) ..."
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    $CONFIG_ALL_OPTIONS += '-GXcode', "-DCMAKE_TOOLCHAIN_FILE=$yasio_root/cmake/ios.cmake", "-DARCHS=$arch", '-DYASIO_USE_CARES=ON'
    if ($options.p -eq 'tvos') {
        $CONFIG_ALL_OPTIONS += '-DPLAT=tvOS'
        cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$yasio_root/cmake/ios.cmake" "-DARCHS=$arch" -DPLAT=tvOS -DYASIO_USE_CARES=ON
    }
    elseif ($options.p -eq 'watchos') {
        $CONFIG_ALL_OPTIONS += '-DPLAT=watchOS', '-DYASIO_SSL_BACKEND=0'
    }
    Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)
    cmake -Bbuild $CONFIG_ALL_OPTIONS
    cmake --build build --config Release
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

setup_cmake

& $builds[$options.p] -cmakeOptions $CONFIG_ALL_OPTIONS
