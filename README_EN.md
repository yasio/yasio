<p align="center"><a href="https://yasio.org" target="_blank" rel="noopener noreferrer"><img width="160" src="https://yasio.org/images/logo.png" alt="yasio logo"></a></p>

# *YASIO* - *Y*et another *A*synchronous *S*ocket *I*/*O*.

[![Android Build Status](https://github.com/yasio/yasio/workflows/android/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aandroid)
[![iOS Build Status](https://github.com/yasio/yasio/workflows/ios/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aios)
[![Windows Build Status](https://github.com/yasio/yasio/workflows/windows/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Awin32)
[![Linux Build Status](https://github.com/yasio/yasio/workflows/linux/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Alinux)
[![macOS Build Status](https://github.com/yasio/yasio/workflows/osx/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aosx)  

[![Release](https://img.shields.io/badge/release-v3.34.0-blue.svg)](https://github.com/yasio/yasio/releases)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/yasio/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yasio/yasio.svg?label=Stars)](https://github.com/yasio/yasio)
[![GitHub forks](https://img.shields.io/github/forks/yasio/yasio.svg?label=Fork)](https://github.com/yasio/yasio)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/alerts/)
  
[![Supported Platforms](https://img.shields.io/badge/platform-ios%20%7C%20android%20%7C%20osx%20%7C%20windows%20%7C%20linux%20%7C%20freebsd-green.svg?style=flat-square)](https://github.com/yasio/yasio)
  
[![Powered](https://img.shields.io/badge/Powered%20by-c4games-blue.svg)](http://c4games.com)  
  
**[简体中文](README.md)**
  
**yasio** is a multi-platform support and lightweight library with focus on asynchronous socket I/O for any client application, support windows & linux & apple & android & win10-universal.  

## Showcase
* [RAOL Mobile Game Project](https://hongjing.qq.com/): Since the game is published on Tencent Games at 2018.10.17, it's run at millions of devices.
* [x-studio IDE Project](https://en.x-studio.net/): The local LAN upgrade system is based on yasio.

## Integration Demos
* [xlua](https://github.com/yasio/DemoU3D): Integrate yasio to xlua, make the unity3d game project based on xlua can use yasio lua bindings APIs.
* Unreal Engine 4
  - [DemoUE4](https://github.com/yasio/DemoUE4): Demo for Unreal Engine 4
  - [sluaunreal](https://github.com/yasio/sluaunreal): Integrate to Tencent's sluaunreal(A lua bindings solution for UE4)
  - [UnLua](https://github.com/yasio/UnLua): Integrate to Tencent's sluaunreal(Yet another lua bindings solution for UE4)

## Docomentation
* Simplified Chinese: [https://docs.yasio.org/](https://docs.yasio.org/)
* English: [https://docs.yasio.org/en/latest/](https://docs.yasio.org/en/latest/) (Building)

## Simple run tcptest with g++
```sh
g++ tests/tcp/main.cpp --std=c++11 -DYASIO_HEADER_ONLY -lpthread -I./ -o tcptest && ./tcptest
```

## Build more examples with cmake
```sh
git clone https://github.com/yasio/yasio
cd yasio
git submodule update --init --recursive
# For macOS xcode, it's shoud be: cmake -B build -GXcode
cmake -B build

# Use cmake command to build examples & tests, or use platform specific IDE to open yasio project
# a. Windows: Use Visual Studio(2013~2019 supported) to open build/yasio.sln
# b. macOS: Use Xcode to open build/yasio.xcodeproj
cmake --build build --config Debug
```

## Features: 
* Support TCP/UDP/KCP, and manipulate APIs are same
* Support process ```sticking packets``` for TCP internal, user do not need to care it
* Support multicast
* Support IPv4/IPv6 network
* Support multi-connections at one thread
* Support high-resolution deadline timer
* Support lua bindings
* Support cocos2d-x jsb
* Support [CocosCreator jsb2.0](https://github.com/yasio/inettester)
* Support [Unity3D](https://github.com/yasio/DemoU3D)
* Support [Unreal Engine 4](https://github.com/yasio/DemoUE4)
* Support ssl client with openssl
* Support async resolve with c-ares
* Support header only with ```YASIO_HEAD_ONLY=1``` set at config.hpp or compiler flags
* Support Unix Domain Socket
* Support BinaryStram by **obstream/ibstream**, easy to use
* Support **7Bit Encoded Int/Int64** compatible with dotnet

## About C++17
yasio provide follow C++17 standard components compatible with C++11 compiler, please see: [cxx17](https://github.com/yasio/yasio/tree/master/yasio/cxx17)
- cxx17::string_view
- cxx17::shared_mutex
- cxx20::starts_with, cxx20::ends_with
  
## Core framework
![image](https://yasio.org/images/framework_en.png)  

