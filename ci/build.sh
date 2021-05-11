set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
YASIO_ROOT="$DIR"/..

function build_osx()
{   
    echo "Building osx..."
    NUM_OF_CORES=`getconf _NPROCESSORS_ONLN`

    cd $YASIO_ROOT/build
    
    mkdir -p build_osx
    
    cmake .. -GXcode -Bbuild_osx -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build_osx --config Release
    
    echo "run test tcptest on osx..."
    build_osx/tests/tcp/Release/tcptest
    
    exit 0
}

function build_ios()
{  
    echo "Building iOS..."
    cd $YASIO_ROOT/build
    
    mkdir -p build_ios
    
    # cmake .. -GXcode -Bbuild_ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    # cmake official cross-compiling doesn't works on latest macOS 10.15 + xcode11
    # https://cmake.org/cmake/help/v3.17/manual/cmake-toolchains.7.html?highlight=iphoneos#cross-compiling-for-ios-tvos-or-watchos
    cmake .. -GXcode -Bbuild_ios "-DCMAKE_TOOLCHAIN_FILE=$YASIO_ROOT/cmake/ios.mini.cmake" "-DCMAKE_OSX_ARCHITECTURES=armv7;arm64" -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build_ios --config Release
    
    exit 0
}

function build_linux()
{
    echo "Building linux..."
    cd $YASIO_ROOT/build
    mkdir -p build_linux
    cmake .. -G "Unix Makefiles" -Bbuild_linux -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=2 -DYASIO_HAVE_KCP=ON -DYASIO_HAVE_CARES=ON -DYAISO_BUILD_NI=YES -DCXX_STD=17 -DYASIO_VERBOSE_LOG=ON -DYASIO_BUILD_WITH_LUA=ON -DBUILD_SHARED_LIBS=ON

    cmake --build build_linux -- -j `nproc`
    
    echo "run test issue201 on linux..."
    build_linux/tests/issue201/issue201
    
    exit 0
}

function build_freebsd()
{
    echo "Building freebsd..."
    cd $YASIO_ROOT/build
    mkdir -p build_freebsd
    cmake .. -G "Unix Makefiles" -Bbuild_freebsd -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=0 -DYASIO_HAVE_CARES=ON
    cmake --build build_freebsd -- -j `nproc`
    
    echo "run test issue201 on freebsd..."
    build_freebsd/tests/issue201/issue201
    
    exit 0
}

function build_android()
{
    echo "Building android..."
    cd $YASIO_ROOT/build
    mkdir -p build_armv7
    cmake .. -G "Unix Makefiles" -Bbuild_armv7 -DANDROID_STL=c++_shared -DCMAKE_TOOLCHAIN_FILE=~/android-ndk-$NDK_VER/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build_armv7 --target yasio
    
    exit 0
}

cmake --version

if [ $BUILD_TARGET == "linux" ]
then
    build_linux
elif [ $BUILD_TARGET == "freebsd" ]
then
    build_freebsd
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
