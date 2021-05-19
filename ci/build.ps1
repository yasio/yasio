# install latest cmake version
curl https://github.com/Kitware/CMake/releases/download/v3.20.2/cmake-3.20.2-windows-x86_64.zip -o cmake-3.20.2-windows-x86_64.zip
Expand-Archive -Path cmake-3.20.2-windows-x86_64.zip -DestinationPath .\
$cmake_bin = (Resolve-Path .\).Path
$cmake_bin = "$cmake_bin\cmake-3.20.2-windows-x86_64\bin"
$env:Path = "$cmake_bin;$env:Path"
cmake --version

# run build
echo "env:ARCH=$env:ARCH"
if($env:ARCH -eq "x64") {
    cmake -G "Visual Studio 12 2013 Win64" -B build
}
else {
    cmake -G "Visual Studio 12 2013" -B build
}

cmake --build build --config Release --clean-first
