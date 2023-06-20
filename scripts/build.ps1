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
# 
# The build.ps1
# options
#  -p: build target platform: win32,winuwp,linux,android,osx,ios,tvos,watchos
#  -a: build arch: x86,x64,arm,arm64
#  -cc: c/c++ compiler toolchain: clang, msvc, gcc, mingw-gcc or empty use default installed on current OS
#  -cm: additional cmake options: i.e.  -cm '-DCXX_STD=23','-DYASIO_ENABLE_EXT_HTTP=OFF'
# support matrix
#   | OS        |   Build targets     |  Build toolchain     |
#   +----------+----------------------+----------------------+
#   | Windows  |  win32,winuwp        | msvc,clang,mingw-gcc |
#   | Linux    | linux,android        | gcc,clang            |        
#   | macOS    | osx,ios,tvos,watchos | clang                |
#  
#

$options = @{p = 'win32'; a = 'x64'; cc = 'msvc120';  cm = @(); }

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

# if found or installed, the ndk_root indicate the root path of installed ndk
$ndk_root = $null
$ninja_prog = $null

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

$CI_CHECKS = ("$env:GITHUB_ACTIONS" -eq 'true') -or ("$env:APPVEYOR_BUILD_VERSION" -ne '')

# for ci check, enable high preformance platform I/O multiplexing
if ($CI_CHECKS) {
    Write-Host "Enable high performance I/O multiplexing for ci checks"
    $CONFIG_DEFAULT_OPTIONS = @('-DYASIO_ENABLE_HPERF_IO=1')
} else {
    $CONFIG_DEFAULT_OPTIONS = @()
}

# determine toolchain
$TOOLCHAIN = $options.cc
$toolchains = @{ 
    'win32' = 'msvc';
    'winuwp' = 'msvc';
    'linux' = 'gcc'; 
    'android' = 'clang';
    'osx' = 'clang';
    'ios' = 'clang';
    'tvos' = 'clang';
    'watchos' = 'clang';
}
if (!$TOOLCHAIN) {
    $TOOLCHAIN = $toolchains[$options.p]
}
$TOOLCHAIN_INFO = ([regex]::Matches($TOOLCHAIN, '(\d+)|(\D+)')).Value
if ($TOOLCHAIN_INFO.Count -ge 2) {
    $TOOLCHAIN_NAME = $TOOLCHAIN_INFO[0]
    $TOOLCHAIN_VER = $TOOLCHAIN_INFO[1]
} else {
    $TOOLCHAIN_NAME = $TOOLCHAIN
    $TOOLCHAIN_VER = $null
}

$HOST_NAME = $('windows', 'linux', 'macos').Get($hostOS)

# now windows only
function setup_cmake() {
    $cmake_prog = (Get-Command "cmake" -ErrorAction SilentlyContinue).Source
    if ($cmake_prog) {
        $_cmake_ver = $($(cmake --version | Select-Object -First 1) -split ' ')[2]
    } else {
        $_cmake_ver = '0.0.0'
    }
    if ($_cmake_ver -ge '3.13.0') {
        Write-Host "Using installed cmake $cmake_prog, version: $_cmake_ver"
    } else {
        
        Write-Host "The installed cmake $_cmake_ver too old, installing newer $cmake_ver ..."

        $cmake_root = $(Join-Path -Path $yasio_tools -ChildPath "cmake-$cmake_ver-$HOST_NAME-x86_64")
        if (!(Test-Path $cmake_root -PathType Container)) {
            Write-Host "Downloading cmake-$cmake_ver-windows-x86_64.zip ..."
            if ($hostOS -eq $HOST_WIN) {
                $cmake_url = "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-$HOST_NAME-x86_64.zip"
                if ($pwsh_ver -lt '7.0')  {
                    curl $cmake_url -o "$cmake_root.zip"
                } else {
                    curl -L $cmake_url -o "$cmake_root.zip"
                }
                Expand-Archive -Path "$cmake_root.zip" -DestinationPath $yasio_tools\
            }
            elseif($hostOS -eq $HOST_LINUX) {
                if (!(Test-Path "$cmake_root.sh" -PathType Leaf)) {
                    $cmake_url = "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-$HOST_NAME-x86_64.sh"
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
        $suffix = $('win', 'linux', 'mac').Get($hostOS)
        $ninja_bin = (Resolve-Path "$yasio_tools/ninja-$suffix" -ErrorAction SilentlyContinue).Path
        if (!$ninja_bin) {
            Write-Host "Downloading ninja-$suffix.zip ..."
            curl -L "https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-$suffix.zip" -o $yasio_tools/ninja-$suffix.zip
            Expand-Archive -Path $yasio_tools/ninja-$suffix.zip -DestinationPath "$yasio_tools/ninja-$suffix/"
            $ninja_bin = (Resolve-Path "$yasio_tools/ninja-$suffix" -ErrorAction SilentlyContinue).Path
        }
        if ($env:PATH.IndexOf($ninja_bin) -eq -1) {
            $env:PATH = "$ninja_bin$envPathSep$env:PATH"
        }
        $ninja_prog = (Join-Path -Path $ninja_bin -ChildPath ninja$exeSuffix)
    } else {
        Write-Host "Using installed ninja: $ninja_prog, version: $(ninja --version)"
    }
    return $ninja_prog
}

function setup_ndk() {
    # setup ndk
    $ndk_ver = $env:NDK_VER
    if (!$ndk_ver) {
        $ndk_ver = 'r16b+'
    }

    $IsGraterThan = $ndk_ver.EndsWith('+')
    if($IsGraterThan) {
        $ndk_ver = $ndk_ver.Substring(0, $ndk_ver.Length - 1)
    }

    if("$env:ANDROID_HOME" -ne '') {
        $ndk_minor_base = [int][char]'a'

        $ndk_major = ($ndk_ver -replace '[^0-9]', '')
        $ndk_minor_off = "$ndk_major".Length + 1
        $ndk_minor = if($ndk_minor_off -lt $ndk_ver.Length) {"$([int][char]$ndk_ver.Substring($ndk_minor_off) - $ndk_minor_base)"} else {'0'}
        $ndk_rev_base = "$ndk_major.$ndk_minor"

        # find ndk in sdk
        $ndks = [ordered]@{}
        $ndk_rev_max = '0.0'
        foreach($item in $(Get-ChildItem -Path "$env:ANDROID_HOME/ndk")) {
            $ndkDir = $item.FullName
            $sourceProps = "$ndkDir/source.properties"
            if (Test-Path $sourceProps -PathType Leaf) {
                $verLine = $(Get-Content $sourceProps | Select-Object -Index 1)
                $ndk_rev = $($verLine -split '=').Trim()[1].split('.')[0..1] -join '.'
                $ndks.Add($ndk_rev, $ndkDir)
                if ($ndk_rev_max -le $ndk_rev) {
                    $ndk_rev_max = $ndk_rev
                }
            }
        }
        if ($IsGraterThan) {
            if ($ndk_rev_max -ge $ndk_rev_base) {
                $ndk_root = $ndks[$ndk_rev_max]
            }
        } else {
            $ndk_root = $ndks[$ndk_rev_base]
        }
    }
    
    if (Test-Path "$ndk_root" -PathType Container)
    {
        Write-Host "Using installed ndk: $ndk_root ..."
    }
    else {
        $suffix = "$(('windows', 'linux', 'darwin').Get($hostOS))$(if ("$ndk_ver" -le "r22z") {'-x86_64'} else {''})"
        $ndk_root = "$yasio_tools/android-ndk-$ndk_ver"
        if (!(Test-Path "$ndk_root" -PathType Container)) {
            Write-Host "Downloading ndk package $ndk_package ..."
            $ndk_package="android-ndk-$ndk_ver-$suffix"
            curl -o $yasio_tools/$ndk_package.zip https://dl.google.com/android/repository/$ndk_package.zip
            Expand-Archive -Path $yasio_tools/$ndk_package.zip -DestinationPath $yasio_tools/
        }
    }

    return $ndk_root
}

# preprocess methods: 
#   <param>-inputOptions</param> [CMAKE_OPTIONS]
function preprocess_win {
    Param(
        [string[]]$inputOptions
    )
    $outputOptions = $inputOptions
    $outputOptions += '-DYASIO_ENABLE_WEPOLL=1'

    if ($TOOLCHAIN_NAME -eq 'msvc') { # Generate vs2019 on github ci
        # Determine arch name
        $arch=""
        if ($options.a -eq "x86") {
            $arch="Win32"
        }
        else {
            $arch=$options.a
        }

        $VSWHERE_EXE = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        $eap = $ErrorActionPreference
        $ErrorActionPreference = 'SilentlyContinue'
        $VS2019_OR_LATER_VESION = $null
        $VS2019_OR_LATER_VESION = (& $VSWHERE_EXE -version '16.0' -property installationVersion)
        $ErrorActionPreference = $eap

        # arch
        if($VS2019_OR_LATER_VESION) {
            $outputOptions += '-A', $arch
            if ($TOOLCHAIN_VER) {
                $outputOptions += "-Tv$TOOLCHAIN_VER"
            }
        }
        else {
            $gens = @{
                '120' = 'Visual Studio 12 2013';
                '140' = 'Visual Studio 14 2015'
                "150" = 'Visual Studio 15 2017';
            }
            $gen = $gens[$TOOLCHAIN_VER]
            if(!$gen) {
                Write-Error "Unsupported toolchain: $TOOLCHAIN"
                exit 1
            }
            if ($options.a -eq "x64") {
                $gen += ' Win64'
            }
            $outputOptions += '-G', $gen
        }
        
        # platform
        if ($options.p -eq "winuwp") {
            '-DCMAKE_SYSTEM_NAME=WindowsStore', '-DCMAKE_SYSTEM_VERSION=10.0', '-DBUILD_SHARED_LIBS=OFF'
        }
        
        # common options
        if ($CI_CHECKS) {
            $outputOptions += '-DYASIO_ENABLE_WITH_LUA=ON', '-DYAISO_ENABLE_NI=ON'
        }

        # openssl prebuilt was built from vs2022, so we set ssl backend to use mbedtls-2.28.3
        # Notes: mbedtls-3.x no longer support compile with vs2013, will encounter many compiling errors
        if ($TOOLCHAIN_VER -lt '170') {
            $outputOptions += '-DYASIO_SSL_BACKEND=2'
        }
    }
    elseif($TOOLCHAIN_NAME -eq 'clang') {
        Write-Host (clang --version)
        $outputOptions += '-G', 'Ninja Multi-Config', '-DCMAKE_C_COMPILER=clang', '-DCMAKE_CXX_COMPILER=clang++'
    }
    else { # Generate mingw
        $outputOptions += '-G', 'Ninja Multi-Config'
    }
    return $outputOptions
}

function preprocess_linux {
    Param(
        [string[]]$inputOptions
    )
    $outputOptions = $inputOptions
    $outputOptions += '-DCMAKE_BUILD_TYPE=Release', '-DYASIO_USE_CARES=ON', '-DYASIO_ENABLE_ARES_PROFILER=ON', '-DYAISO_ENABLE_NI=YES', '-DCXX_STD=17', '-DYASIO_ENABLE_WITH_LUA=ON', '-DBUILD_SHARED_LIBS=ON'

    return $outputOptions
}

function preprocess_andorid {
    Param(
        [string[]]$inputOptions
    )
    $outputOptions = $inputOptions

    # building
    $arch=$options.a
    if ($arch -eq 'arm') {
        $arch = 'armeabi-v7a'
    } elseif($arch -eq 'arm64') {
        $arch = 'arm64-v8a'
    } elseif($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    $cmake_toolchain_file = "$ndk_root\build\cmake\android.toolchain.cmake"
    $outputOptions += '-G', 'Ninja', '-DANDROID_STL=c++_shared', "-DCMAKE_MAKE_PROGRAM=$ninja_prog", "-DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file", "-DANDROID_ABI=$arch", '-DCMAKE_BUILD_TYPE=Release', '-DYASIO_USE_CARES=ON'
    $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH'
    $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH'
    $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH'
    # by default, we want find host program only when cross-compiling
    $outputOptions += '-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER'
    
    return $outputOptions
}

function preprocess_osx {
    Param(
        [string[]]$inputOptions
    )
    $outputOptions = $inputOptions
    $arch = $options.a
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }

    $outputOptions += '-GXcode', '-DYASIO_USE_CARES=ON', "-DCMAKE_OSX_ARCHITECTURES=$arch"
    return $outputOptions
}

# build ios famliy (ios,tvos,watchos)
function preprocess_ios {
    Param(
        [string[]]$inputOptions
    )
    $outputOptions = $inputOptions
    if ($arch -eq 'x64') {
        $arch = 'x86_64'
    }
    $cmake_toolchain_file = Join-Path -Path $yasio_root -ChildPath 'cmake' -AdditionalChildPath 'ios.cmake'
    $outputOptions += '-GXcode', "-DCMAKE_TOOLCHAIN_FILE=$cmake_toolchain_file", "-DARCHS=$arch", '-DYASIO_USE_CARES=ON'
    if ($options.p -eq 'tvos') {
        $outputOptions += '-DPLAT=tvOS'
    }
    elseif ($options.p -eq 'watchos') {
        $outputOptions += '-DPLAT=watchOS', '-DYASIO_SSL_BACKEND=0'
    }
    return $outputOptions
}

$proprocessTable = @{ 
    'win32' = ${function:preprocess_win};
    'winuwp' = ${function:preprocess_win};
    'linux' = ${function:preprocess_linux}; 
    'android' = ${function:preprocess_andorid};
    'osx' = ${function:preprocess_osx};
    'ios' = ${function:preprocess_ios};
    'tvos' = ${function:preprocess_ios};
    'watchos' = ${function:preprocess_ios};
}

# run tests
$testTable = @{
    'win32' = {
        $buildDir = $args[0]
        if (($options.p -ne 'winuwp') -and ($TOOLCHAIN_NAME -ne 'mingw-gcc')) {
            Write-Host "run icmptest on windows ..."
            Invoke-Expression -Command ".\$buildDir\tests\icmp\Release\icmptest.exe $env:PING_HOST"
        }
    };
    'linux' = {
        $buildDir = $args[0]
        if ($CI_CHECKS) {
            Write-Host "run issue201 on linux..."
            ./$buildDir/tests/issue201/issue201
            
            Write-Host "run httptest on linux..."
            ./$buildDir/tests/http/httptest
        
            Write-Host "run ssltest on linux..."
            ./$buildDir/tests/ssl/ssltest
        
            Write-Host "run icmp test on linux..."
            ./$buildDir/tests/icmp/icmptest $env:PING_HOST
        }
    }; 
    'osx' = {
        $buildDir = $args[0]
        if ($CI_CHECKS -and ($options.a -eq 'x64')) {
            Write-Host "run test tcptest on osx ..."
            ./$buildDir/tests/tcp/Release/tcptest
            
            Write-Host "run test issue384 on osx ..."
            ./$buildDir/tests/issue384/Release/issue384
    
            Write-Host "run test icmp on osx ..."
            ./$buildDir/tests/icmp/Release/icmptest $env:PING_HOST
        }
    };
    'winuwp' = {};
    'android' = {};
    'ios' = {};
    'tvos' = {};
    'watchos' = {};
}

# setup tools: cmake, ninja, ndk if required for target build
if ($hostOS -ne $HOST_OSX) {
    setup_cmake
}

if (($options.p -eq 'android' -or $options.p -eq 'win32') -and ($TOOLCHAIN_NAME -ne 'msvc')) {
    $ninja_prog = setup_ninja
}

if ($options.p -eq 'android') {
    $ndk_root = setup_ndk
}

# enter building steps
Write-Host "Building target $($options.p) on $hostName with toolchain $TOOLCHAIN ..."

# step1. preprocess cmake options
$CONFIG_ALL_OPTIONS = $(& $proprocessTable[$options.p] -inputOptions $CONFIG_DEFAULT_OPTIONS)

# step2. appli additional cmake options
if ($options.cm.Count -gt 0) {
    Write-Host ("Apply additional cmake options: $($options.cm), Count={0}" -f $options.cm.Count)
    $CONFIG_ALL_OPTIONS += $options.cm
}
if ("$($options.cm)".IndexOf(('-B')) -eq -1) {
    $BUILD_DIR = "build_${TOOLCHAIN}_$($options.a)"
} else {
    foreach($opt in $options.cm) {
        if ($opt.StartsWith('-B')) {
            $BUILD_DIR = $opt.Substring(2).Trim()
            break
        }
    }
}
Write-Host ("CONFIG_ALL_OPTIONS=$CONFIG_ALL_OPTIONS, Count={0}" -f $CONFIG_ALL_OPTIONS.Count)

# step3. configure & build
cmake -B $BUILD_DIR $CONFIG_ALL_OPTIONS
cmake --build $BUILD_DIR --config Release

# run test
$run_test = $testTable[$options.p]
if ($run_test) {
    & $run_test($BUILD_DIR)
}
