set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
YASIO_ROOT="$DIR"/..

function build_osx()
{   
    echo "Building osx..."
    NUM_OF_CORES=`getconf _NPROCESSORS_ONLN`

    cmake -GXcode -Bbuild -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build --config Release
    
    echo "run test tcptest on osx ..."
    ./build/tests/tcp/Release/tcptest
    
    echo "run test issue384 on osx ..."
    ./build/tests/issue384/Release/issue384

    echo "run test icmp on osx ..."
    ./build/tests/icmp/Release/icmptest
    
    exit 0
}

function build_ios()
{  
    echo "Building iOS..."

    # cmake .. -GXcode -Bbuild_ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphonesimulator -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    # cmake official cross-compiling doesn't works on latest macOS 10.15 + xcode11
    # https://cmake.org/cmake/help/v3.17/manual/cmake-toolchains.7.html?highlight=iphoneos#cross-compiling-for-ios-tvos-or-watchos
    cmake -GXcode -Bbuild "-DCMAKE_TOOLCHAIN_FILE=$YASIO_ROOT/cmake/ios.mini.cmake" "-DCMAKE_OSX_ARCHITECTURES=arm64" -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build --config Release
    
    exit 0
}

function build_linux()
{
    echo "Building linux..."
    cmake -G "Unix Makefiles" -Bbuild -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=2 -DYASIO_HAVE_KCP=ON -DYASIO_HAVE_CARES=ON -DYASIO_ENABLE_ARES_PROFILER=ON -DYAISO_BUILD_NI=YES -DCXX_STD=17 -DYASIO_BUILD_WITH_LUA=ON -DBUILD_SHARED_LIBS=ON

    cmake --build build -- -j `nproc`
    
    echo "run issue201 on linux..."
    build/tests/issue201/issue201
    
    echo "run httptest on linux..."
    build/tests/http/httptest

    echo "run ssltest on linux..."
    build/tests/ssl/ssltest

    echo "run icmp test on linux..."
    build/tests/icmp/icmptest
    
    exit 0
}

function build_android()
{
    echo "Building android..."
    NINJA_PATH=~/ninja
    cmake -G "Ninja" -B build -DANDROID_STL=c++_shared -DCMAKE_MAKE_PROGRAM=$NINJA_PATH -DCMAKE_TOOLCHAIN_FILE=~/android-ndk-$NDK_VER/build/cmake/android.toolchain.cmake -DCMAKE_ANDROID_NDK_TOOLCHAIN_VERSION=clang -DANDROID_ABI=$BUILD_ARCH -DCMAKE_BUILD_TYPE=Release -DYASIO_SSL_BACKEND=1 -DYASIO_HAVE_CARES=ON
    cmake --build build --target yasio
    cmake --build build --target yasio_http
    
    exit 0
}

cmake --version

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
