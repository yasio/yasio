#!/bin/bash

set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
YASIO_ROOT="$DIR"/..

function build_linux()
{
    echo "Building linux..."
    cd $YASIO_ROOT/build
    mkdir -p build_linux
    cmake ../ -G "Unix Makefiles" -Bbuild_linux -DCMAKE_BUILD_TYPE=Release
    cmake --build build_linux -- -j `nproc`
}

function build_osx()
{
    NUM_OF_CORES=`getconf _NPROCESSORS_ONLN`

    cd $YASIO_ROOT/build
    
    # osx
    echo "Building macOS..."
    mkdir -p build_osx
    cd build_osx
    cmake ../../ -GXcode
    cmake --build . --config Release
    cd ..
    
    # ios
    mkdir -p build_ios
    echo "Building iOS..."
    cd build_ios
    cmake ../../ -G Xcode -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake -DCMAKE_SYSTEM_NAME=iOS -DPLATFORM=OS
    cmake --build . --config Release
    cd ..

    
    exit 0
}

if [ $TRAVIS_OS_NAME == "linux" ]
then
    build_linux
elif [ $TRAVIS_OS_NAME == "osx" ]
then
    build_osx
else
  exit 0
fi
