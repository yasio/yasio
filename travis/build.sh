#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
YASIO_ROOT="$DIR"/..

function build_osx()
{   
    echo "Building osx..."
    NUM_OF_CORES=`getconf _NPROCESSORS_ONLN`

    cd $YASIO_ROOT/build
    
    mkdir -p build_osx
    
    cmake ../ -GXcode -Bbuild_osx
    cmake --build build_osx --config Release
    
    echo "run test issue201 on osx..."
    build_osx/tests/issue201/Release/issue201
    
    exit 0
}

function build_ios()
{  
    echo "Building iOS..."
    cd $YASIO_ROOT/build
    
    mkdir -p build_ios
    
    cmake ../ -GXcode -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator
    cmake --build build_ios --config Release
    
    exit 0
}

function build_linux()
{
    echo "Building linux..."
    cd $YASIO_ROOT/build
    mkdir -p build_linux
    cmake ../ -G "Unix Makefiles" -Bbuild_linux -DCMAKE_BUILD_TYPE=Release
    cmake --build build_linux -- -j `nproc`
    
    echo "run test issue201 on linux..."
    build_linux/tests/issue201/issue201
    
    exit 0
}

function build_android()
{
    echo "Building android..."
    cd $YASIO_ROOT/build
    mkdir -p build_armv7
    cmake ../ -G "Unix Makefiles" -Bbuild_armv7 -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=~/android-ndk-$NDK_VER/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release
    cmake --build build_armv7 --target yasio
    
    exit 0
}

if [ $BUILD_TARGET == "linux" ]
then
    build_linux
elif [ $BUILD_TARGET == "osx" ]
then
    build_osx
elif [ $BUILD_TARGET == "ios" ]
then
    build_ios
elif [ $BUILD_TARGET == "android" ]
then
    build_android
else
  exit 0
fi
