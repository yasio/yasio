rem require VS2019
@echo off

pushd %~dp0

if not exist build_x86 mkdir build_x86
cd build_x86
cmake -A Win32 ../../

cmake --build . --config Debug
rem cmake --build . --config Debug --target yasio
rem ccmake --build . --config Release --target yasio

popd
