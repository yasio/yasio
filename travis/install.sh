#!/bin/bash

cmake --version

if [[ "${TRAVIS_OS_NAME}" == "linux" && "${BUILD_TARGET}" == "android" ]]; then
    curl -o ~/android-ndk.zip https://dl.google.com/android/repository/android-ndk-r16b-linux-x86_64.zip
    unzip -q ~/android-ndk.zip -d ~ \
    'android-ndk-r16b/build/cmake/*' \
    'android-ndk-r16b/build/core/toolchains/arm-linux-androideabi-*/*' \
    'android-ndk-r16b/platforms/android-14/arch-arm/*' \
    'android-ndk-r16b/source.properties' \
    'android-ndk-r16b/sources/android/support/include/*' \
    'android-ndk-r16b/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a/*' \
    'android-ndk-r16b/sources/cxx-stl/llvm-libc++/include/*' \
    'android-ndk-r16b/sysroot/*' \
    'android-ndk-r16b/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/*' \
    'android-ndk-r16b/toolchains/llvm/prebuilt/linux-x86_64/*'
fi
