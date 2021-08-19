
cmake --version

if [[ "${TRAVIS_OS_NAME}" == "linux" && "${BUILD_TARGET}" == "android" ]]; then
    curl -o ~/android-ndk-$NDK_VER-linux-x86_64.zip https://dl.google.com/android/repository/android-ndk-$NDK_VER-linux-x86_64.zip
    unzip -q ~/android-ndk-$NDK_VER-linux-x86_64.zip -d ~
    wget -O ~/ninja-linux.zip https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
    unzip ~/ninja-linux.zip -d ~
    ~/ninja --version
fi
