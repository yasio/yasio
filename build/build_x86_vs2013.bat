@echo off

pushd %~dp0

if not exist build_x86 mkdir build_x86
cd build_x86
cmake -G "Visual Studio 12 2013" ../../
cmake --build . --config Debug

popd
