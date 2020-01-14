@echo off

pushd %~dp0

if not exist build_x86 mkdir build_x86
cd build_x86
cmake -A Win32 ../../
cmake --build . --config Debug

popd
