set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
YASIO_ROOT="$DIR"/..

function build_osx()
{   
    echo "Building osx..."
    NUM_OF_CORES=`getconf _NPROCESSORS_ONLN`

    cmake -GXcode -Bbuild -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake --build build --config Release
    
    echo "run test tcptest on osx ..."
    ./build/tests/tcp/Release/tcptest
    
    echo "run test issue384 on osx ..."
    ./build/tests/issue384/Release/issue384

    echo "run test icmp on osx ..."
    ./build/tests/icmp/Release/icmptest $PING_HOST
    
    exit 0
}

function build_ios()
{  
    echo "Building iOS..."

    # cmake .. -GXcode -Bbuild_ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$YASIO_ROOT/cmake/ios.cmake" "-DARCHS=arm64" -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake --build build --config Release
    
    exit 0
}

function build_tvos()
{  
    echo "Building tvos..."

    # cmake .. -GXcode -Bbuild_ios -DCMAKE_SYSTEM_NAME=tvOS -DCMAKE_OSX_SYSROOT=appletvsimulator -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$YASIO_ROOT/cmake/ios.cmake" "-DARCHS=arm64" -DPLAT=tvOS -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake --build build --config Release
    
    exit 0
}

function build_watchos()
{  
    echo "Building watchos..."

    cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$YASIO_ROOT/cmake/ios.cmake" "-DARCHS=arm64" -DPLAT=watchOS -DYASIO_SSL_BACKEND=0 -DYASIO_USE_CARES=ON
    cmake --build build --config Release
    
    exit 0
}

function build_linux()
{
    echo "Building linux..."
    cmake -Bbuild -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=2 -DYASIO_USE_CARES=ON -DYASIO_ENABLE_ARES_PROFILER=ON -DYAISO_BUILD_NI=YES -DCXX_STD=17 -DYASIO_BUILD_WITH_LUA=ON -DBUILD_SHARED_LIBS=ON

    cmake --build build -- -j `nproc`
    
    echo "run issue201 on linux..."
    build/tests/issue201/issue201
    
    echo "run httptest on linux..."
    build/tests/http/httptest

    echo "run ssltest on linux..."
    build/tests/ssl/ssltest

    echo "run icmp test on linux..."
    build/tests/icmp/icmptest $PING_HOST
    
    exit 0
}

function build_android()
{
    echo "Building android..."
    echo "ndk_root=$ndk_root"
    NINJA_PATH=~/ninja
    cmake -G "Ninja" -B build -DANDROID_STL=c++_shared -DCMAKE_MAKE_PROGRAM=$NINJA_PATH -DCMAKE_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DANDROID_ABI=$BUILD_ARCH -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=1 -DYASIO_USE_CARES=ON
    cmake --build build --target yasio
    cmake --build build --target yasio_http
    
    exit 0
}

cmake --version

if [ $BUILD_TARGET = "linux" ]; then
    build_linux
elif [ $BUILD_TARGET = "osx" ]; then
    build_osx
elif [ $BUILD_TARGET = "ios" ]; then
    build_ios
elif [ $BUILD_TARGET = "tvos" ]; then
    build_tvos
elif [ $BUILD_TARGET = "watchos" ]; then
    build_watchos
elif [ $BUILD_TARGET = "android" ]; then
    build_android
else
  exit 0
fi
