
cmake --version

if [[ "${TRAVIS_OS_NAME}" == "linux" && "${BUILD_TARGET}" == "android" ]]; then
    curl -o ~/android-ndk.zip https://dl.google.com/android/repository/android-ndk-$NDK_VER-linux-x86_64.zip
    unzip -q ~/android-ndk.zip -d ~ \
    "android-ndk-$NDK_VER/build/cmake/*" \
    "android-ndk-$NDK_VER/build/core/toolchains/arm-linux-androideabi-*/*" \
    "android-ndk-$NDK_VER/platforms/android-14/arch-arm/*" \
    "android-ndk-$NDK_VER/source.properties" \
    "android-ndk-$NDK_VER/sources/android/support/include/*" \
    "android-ndk-$NDK_VER/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a/*" \
    "android-ndk-$NDK_VER/sources/cxx-stl/llvm-libc++/include/*" \
    "android-ndk-$NDK_VER/sysroot/*" \
    "android-ndk-$NDK_VER/toolchains/arm-linux-androideabi-4.9/prebuilt/linux-x86_64/*" \
    "android-ndk-$NDK_VER/toolchains/llvm/prebuilt/linux-x86_64/*"
fi
