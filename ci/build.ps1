# Install latest cmake version for appveyor ci
if (!($env:GITHUB_CI -eq "true")) {
    $cmake_ver="3.21.3"
    curl "https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-windows-x86_64.zip" -o "cmake-$cmake_ver-windows-x86_64.zip" -L
    Expand-Archive -Path cmake-$cmake_ver-windows-x86_64.zip -DestinationPath .\
    $cmake_bin = (Resolve-Path .\cmake-$cmake_ver-windows-x86_64\bin).Path
    $env:Path = "$cmake_bin;$env:Path"
}

cmake --version

# Generate
echo "env:BUILD_ARCH=$env:BUILD_ARCH"

if ($env:GITHUB_CI -eq "true") {
    if ($env:TOOLCHAIN -eq "msvc") { # Generate vs2019 on github ci
       if ($env:BUILD_ARCH -eq "x64") {
           cmake -B build -DYASIO_SSL_BACKEND=1
       }
       else {
           cmake -A Win32 -B build -DYASIO_SSL_BACKEND=1
       }
    }
    else { # Generate mingw
        cmake -G "MinGW Makefiles" -B build
    }
}
else { # Generate vs2013 on appveyor ci
    if ($env:BUILD_ARCH -eq "x64") {
        cmake -G "Visual Studio 12 2013 Win64" -B build -DYASIO_BUILD_WITH_LUA=ON
    }
    else {
        cmake -G "Visual Studio 12 2013" -B build -DYASIO_BUILD_WITH_LUA=ON
    }
}

# Build
cmake --build build --config $env:BUILD_TYPE
