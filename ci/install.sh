
cmake --version

# RUNNER_OS can be: Windows, macOS, Linux
# refer to: https://docs.github.com/en/actions/learn-github-actions/environment-variables
if [[ "${RUNNER_OS}" == "Linux" && "${BUILD_TARGET}" == "android" ]]; then
    curl -o ~/android-ndk-$NDK_VER-linux-x86_64.zip https://dl.google.com/android/repository/android-ndk-$NDK_VER-linux-x86_64.zip
    unzip -q ~/android-ndk-$NDK_VER-linux-x86_64.zip -d ~
    wget -O ~/ninja-linux.zip https://github.com/ninja-build/ninja/releases/download/v1.10.2/ninja-linux.zip
    unzip ~/ninja-linux.zip -d ~
    ~/ninja --version
fi
