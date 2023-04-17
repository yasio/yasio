
cmake --version

# RUNNER_OS can be: Windows, macOS, Linux
# refer to: https://docs.github.com/en/actions/learn-github-actions/environment-variables
if [[ "${RUNNER_OS}" == "Linux" && "${BUILD_TARGET}" == "android" ]]; then
    # install ninja
    wget -O ~/ninja-linux.zip https://github.com/ninja-build/ninja/releases/download/v1.11.1/ninja-linux.zip
    unzip ~/ninja-linux.zip -d ~
    ~/ninja --version

    # install ndk
    if [ "$USE_GH_NDK" = "true" ]; then
       echo "Find installed ndk on gh action vm ..."
       echo "ANDROID_NDK=$ANDROID_NDK"
       echo "ANDROID_NDK_HOME=$ANDROID_NDK_HOME"
       echo "ANDROID_NDK_ROOT=$ANDROID_NDK_ROOT"

       ndk_root=$ANDROID_NDK
       if [ "$ndk_root" != "" ]; then
           ndk_root=$ANDROID_NDK_HOME
           if [ "$ndk_root" != "" ]; then
               ndk_root=$ANDROID_NDK_ROOT
           fi
       fi
    fi

    if [ "$ndk_root" != "" ]; then
        echo "Using gh action vm installed ndk: $ndk_root"
        echo "ndk_root=$ndk_root" >> $GITHUB_ENV
    else
        suffix=-x86_64
        if [ "$NDK_VER" \> "r22z" ]; then
            # since ndk-r23, no arch suffix
            suffix=
        fi
        ndk_package=android-ndk-$NDK_VER-linux$suffix
        echo "Downloading ndk package $ndk_package ..."
        curl -o ~/$ndk_package.zip https://dl.google.com/android/repository/$ndk_package.zip
        unzip -q ~/$ndk_package.zip -d ~
        ndk_root=~/$ndk_package
    fi

    if [ "$GITHUB_ENV" != "" ]; then
        echo "ndk_root=$ndk_root" >> $GITHUB_ENV
    fi
    export ndk_root=$ndk_root
fi
