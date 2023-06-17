<p align="center"><a href="https://yasio.github.io/yasio" target="_blank" rel="noopener noreferrer"><img width="160" src="docs/assets/images/logo.png" alt="yasio logo"></a></p>

# *YASIO* - *Y*et *A*nother *S*ocket *I*/*O* library.

[![Latest Release](https://img.shields.io/github/v/release/yasio/yasio?include_prereleases&label=release)](../../releases/latest)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/yasio/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yasio/yasio.svg?label=Stars)](https://github.com/yasio/yasio)
[![GitHub forks](https://img.shields.io/github/forks/yasio/yasio.svg?label=Forks)](https://github.com/yasio/yasio)
[![codeql](https://github.com/yasio/yasio/workflows/codeql/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Acodeql)  
  
[![Powered](https://img.shields.io/badge/Powered%20by-C4games%20%7C%20Bytedance-blue.svg)](https://www.bytedance.com/)  
  
**[English](README_EN.md)**
  
**yasio** 是一个轻量级跨平台的异步socket库，专注于客户端和基于各种游戏引擎的游戏客户端网络服务， 支持windows、macos、ios、android、linux、freebsd以及其他类unix操作系统。  

## 支持平台

| Build | Status (github) |
|-------|-----------------|
| Windows(msvc,clang,mingw)|[![Windows Build Status](https://github.com/yasio/yasio/workflows/windows/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Awindows)|
| Windows(vs2013)|[![Windows VS2013 status](https://ci.appveyor.com/api/projects/status/xdmad4v3917n7rct?svg=true)](https://ci.appveyor.com/project/halx99/yasio)|
| Android|[![Android Build Status](https://github.com/yasio/yasio/workflows/android/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aandroid)|
| iOS/tvOS/watchOS|[![iOS Build Status](https://github.com/yasio/yasio/workflows/ios/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aios)|
| Linux |[![Linux Build Status](https://github.com/yasio/yasio/workflows/linux/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Alinux)|
| macOS |[![macOS Build Status](https://github.com/yasio/yasio/workflows/osx/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Aosx)|
| FreeBSD |[![FreeBSD Build Status](https://github.com/yasio/yasio/workflows/freebsd/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Afreebsd)|
| Solaris |[![Solaris Build Status](https://github.com/yasio/yasio/workflows/solaris/badge.svg)](https://github.com/yasio/yasio/actions?query=workflow%3Asolaris)|

## 应用案例
* [放置少女（HD）](http://hcsj.c4connect.co.jp/)：用于cocos和unity重制版客户端网络传输。
* [红警OL手游项目](https://hongjing.qq.com/)：用于客户端网络传输，并且随着该项目于2018年10月17日由腾讯游戏发行正式上线后稳定运行于上千万移动设备上。
* [x-studio软件项目](https://x-studio.net/)：网络解决方案。
* [QttAudio](https://www.qttaudio.com/)：语音连麦聊天集成方案。

## 集成案例
* Unity
  - [yasio_unity](https://github.com/yasio/yasio_unity)：Unity 纯C#封装，打开场景`SampleScene`运行即可。
  - [xlua](https://github.com/yasio/xLua)：将yasio集成到xlua, 打开场景`U3DScripting`运行即可。
* UnrealEngine
  - [yasio_unreal](https://github.com/yasio/yasio_unreal)：yasio的UnrealEngine插件。
  - [sluaunreal](https://github.com/yasio/sluaunreal)：集成到Tencent的sluaunreal。
  - [UnLua](https://github.com/yasio/UnLua)：集成到Tencent的UnLua。
* [axmol](https://github.com/axmolengine/axmol)：作为`axmol`游戏引擎的网络解决方案。

## 文档
* [https://yasio.github.io/yasio](https://yasio.github.io/yasio)

## 使用g++快速运行tcptest测试程序
```sh
g++ tests/tcp/main.cpp --std=c++11 -DYASIO_HEADER_ONLY -lpthread -I./ -o tcptest && ./tcptest
```

## 使用CMake编译yasio的测试程序和示例程序
```sh
git clone --recursive https://github.com/yasio/yasio
cd yasio

# 如果是macOS Xcode, 这里命令应该换成：cmake -B build -GXcode
cmake -B build

# 使用CMake命令行编译, 如果需要调试，则使用相应平台IDE打开即可:
# a. Windows：使用VisualStudio打开build/yasio.sln
# b. macOS：使用Xcode打开build/yasio.xcodeproj
cmake --build build --config Debug

# # 者直接用VS打开 
```

## 特性：
* 支持TCP，UDP，KCP传输，且API是统一的。
* 支持TCP粘包处理，业务完全不必关心。ds
* 支持组播。
* 支持IPv4/IPv6或者苹果IPv6_only网络。
* 支持处理多个连接的所有网络事件。
* 支持微秒级定时器。
* 支持Lua绑定。
* 支持Cocos2d-x jsb绑定。
* 支持[CocosCreator jsb2.0绑定](https://github.com/yasio/inettester)。
* 支持[Unity3D](https://github.com/yasio/yasio_unity)。
* 支持[虚幻引擎](https://github.com/yasio/yasio_unreal)。
* 支持SSL客户端/服务端, 基于OpenSSL/MbedTLS。
* 支持非阻塞域名解析，基于c-ares。
* 支持Header Only集成方式，只需要定义编译预处理器宏```YASIO_HEAD_ONLY=1```即可。
* 支持Unix Domain Socket。
* 支持二进制读写，两个工具类**obstream/ibstream**非常方便使用。
* 支持和.net兼容的整数压缩编码方式：**7Bit Encoded Int/Int64**。

## 关于C++17
yasio提供了如下可在C++11编译器下使用的C++17标准库组件，请查看 [yasio/stl](https://github.com/yasio/yasio/tree/dev/yasio/stl)
- cxx17::string_view
- cxx17::shared_mutex
- cxx20::starts_with
- cxx20::ends_with

## 框架图
![image](docs/assets/images/framework.png)  

## QQ交流群
点击加入：[829884294](https://jq.qq.com/?_wv=1027&k=5LDEiNv)

