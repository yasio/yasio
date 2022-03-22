<p align="center"><a href="https://yasio.github.io/yasio" target="_blank" rel="noopener noreferrer"><img width="160" src="docs/assets/images/logo.png" alt="yasio logo"></a></p>

# *YASIO* - *Y*et *A*nother *S*ocket *I*/*O* library.

[![Android Build Status](https://github.com/yasio/yasio/workflows/android/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aandroid)
[![iOS Build Status](https://github.com/yasio/yasio/workflows/ios/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aios)
[![Windows Build Status](https://github.com/yasio/yasio/workflows/windows/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Awindows)
[![MinGW Build Status](https://github.com/yasio/yasio/workflows/mingw/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Amingw)
[![Linux Build Status](https://github.com/yasio/yasio/workflows/linux/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Alinux)
[![macOS Build Status](https://github.com/yasio/yasio/workflows/osx/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aosx)
[![FreeBSD Build Status](https://github.com/yasio/yasio/workflows/freebsd/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Afreebsd)  

[![Latest Release](https://img.shields.io/badge/dynamic/json.svg?label=release&url=https%3A%2F%2Fapi.github.com%2Frepos%2Fyasio%2Fyasio%2Freleases%2Flatest&query=%24.name&colorB=blue)](../../releases/latest)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/yasio/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yasio/yasio.svg?label=Stars)](https://github.com/yasio/yasio)
[![GitHub forks](https://img.shields.io/github/forks/yasio/yasio.svg?label=Forks)](https://github.com/yasio/yasio)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/alerts/)
  
[![Supported Platforms](https://img.shields.io/badge/platform-ios%20%7C%20android%20%7C%20osx%20%7C%20windows%20%7C%20linux%20%7C%20freebsd-green.svg?style=flat-square)](https://github.com/yasio/yasio)
  
[![Powered](https://img.shields.io/badge/Powered%20by-C4games%20%7C%20Bytedance-blue.svg)](https://www.bytedance.com/)  
  
**[简体中文](README.md)**
  
**yasio** is a multi-platform support and lightweight library with focus on asynchronous socket I/O for any client application, support windows, macos, ios, android, linux, freebsd and other unix-like systems.

## Showcase
* [Place Girls](http://hcsj.c4connect.co.jp/): A popular mobile game published on jp.
* [RAOL Mobile Game Project](https://hongjing.qq.com/): Since the game is published on Tencent Games at 2018.10.17, it's run at millions of devices.
* [x-studio IDE Project](https://en.x-studio.net/): The local LAN upgrade system is based on yasio.
* [QttAudio](https://www.qttaudio.com/): Integrated Audio solution.

## Integration Demos
* Unity
  - [yasio_unity](https://github.com/yasio/yasio_unity): The unity c# wrapper of yasio, open scene `SampleScene` and run it.
  - [xlua](https://github.com/yasio/xLua): Integrate yasio to xlua, open scene `U3DScripting` and run it.
* [xlua](https://github.com/yasio/xLua): 
* UnrealEngine
  - [yasio_unreal](https://github.com/yasio/yasio_unreal): The yasio UnrealEngine plugin
  - [sluaunreal](https://github.com/yasio/sluaunreal): Integrate to Tencent's sluaunreal(A lua bindings solution for UE4)
  - [UnLua](https://github.com/yasio/UnLua): Integrate to Tencent's sluaunreal(Yet another lua bindings solution for UE4)
* [adxe](https://github.com/adxeproject/adxe): Use as tcp/udp asynchronous socket solution of game engine `adxe`

## Docomentation
* [https://yasio.github.io/en](https://yasio.github.io/en)

## Simple run tcptest with g++
```sh
g++ tests/tcp/main.cpp --std=c++11 -DYASIO_HEADER_ONLY -lpthread -I./ -o tcptest && ./tcptest
```

## Build more examples with cmake
```sh
git clone --recursive https://github.com/yasio/yasio
cd yasio
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
* Support [Unity3D](https://github.com/yasio/yasio_unity)
* Support [Unreal Engine](https://github.com/yasio/yasio_unreal)
* Support ssl client with OpenSSL/MbedTLS
* Support async resolve with c-ares
* Support header only with ```YASIO_HEAD_ONLY=1``` set at config.hpp or compiler flags
* Support Unix Domain Socket
* Support BinaryStram by **obstream/ibstream**, easy to use
* Support **7Bit Encoded Int/Int64** compatible with dotnet

## About C++17
yasio provide follow C++17 standard components compatible with C++11 compiler, please see: [cxx17](https://github.com/yasio/yasio/tree/master/yasio/cxx17)
- cxx17::string_view
- cxx17::shared_mutex
- cxx20::starts_with
- cxx20::ends_with
  
## Core framework
![image](docs/assets/images/framework_en.png)  

