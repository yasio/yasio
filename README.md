<p align="center"><a href="https://yasio.org" target="_blank" rel="noopener noreferrer"><img width="160" src="https://yasio.org/images/logo.png" alt="yasio logo"></a></p>

# *YASIO* - *Y*et *A*nother asynchronous *S*ocket *I*/*O*.
[![Build Status](https://travis-ci.com/yasio/yasio.svg?branch=master)](https://travis-ci.com/yasio/yasio)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/d6qjfygtw2ewt9pf/branch/master?svg=true)](https://ci.appveyor.com/project/halx99/yasio)
[![Release](https://img.shields.io/badge/dev-v3.33.0-blue.svg)](https://github.com/yasio/yasio/releases)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/yasio/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yasio/yasio.svg?label=Stars)](https://github.com/yasio/yasio)
[![GitHub forks](https://img.shields.io/github/forks/yasio/yasio.svg?label=Fork)](https://github.com/yasio/yasio)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/alerts/)  
[![Last Commit](https://badgen.net/github/last-commit/yasio/yasio)](https://github.com/yasio/yasio)

  
[![Supported Platforms](https://img.shields.io/badge/platform-ios%20%7C%20osx%20%7C%20android%20%7C%20win32%20%7C%20linux-green.svg?style=flat-square)](https://github.com/yasio/yasio)
[![Powered](https://img.shields.io/badge/Powered%20by-c4games-blue.svg)](http://c4games.com)  
  
**[English](README_EN.md)**
  
**yasio** 是一个轻量级跨平台的异步socket库，专注于客户端和基于各种游戏引擎的游戏客户端网络服务, 支持windows & linux & apple & android & win10-universal。  

## 应用案例
* [红警OL手游项目](https://hongjing.qq.com/): 用于客户端网络传输，并且随着该项目于2018年10月17日由腾讯游戏发行正式上线后稳定运行于上千万移动设备上。
* [x-studio软件项目](https://x-studio.net/): 用于实现局域网UDP+TCP发现更新机制。
* [xlua集成](https://github.com/c4games/xlua): 将yasio集成到xlua, 使基于xlua的unity3d可以直接使用yasio的lua绑定接口。
    
## 文档
* 简体中文: [https://docs.yasio.org/](https://docs.yasio.org/)
* 英文: [https://docs.yasio.org/en/latest/](https://docs.yasio.org/en/latest/)


## 使用g++快速运行tcptest测试程序
```sh
g++ tests/tcp/main.cpp --std=c++11 -DYASIO_HEADER_ONLY -lpthread -I./ -o tcptest && ./tcptest
```

## 使用CMake编译yasio的测试程序和示例程序
```sh
git clone https://github.com/yasio/yasio
cd yasio
git submodule update --init --recursive
cd build
cmake ..

# 使用CMake命令行编译
# 或者直接用VS打开 yasio.sln 解决方案
cmake --build . --config Debug
```

## 特性: 
* 支持IPv6_only网络。  
* 支持处理多个连接的所有网络事件。  
* 支持微秒级定时器。  
* 支持TCP粘包处理，业务完全不必关心。  
* 支持Lua绑定。  
* 支持Cocos2d-x jsb绑定。  
* 支持CocosCreator jsb2.0绑定。  
* 支持Unity3D C#绑定。  
* 支持组播。  
* 支持SSL客户端，基于OpenSSL。  
* 支持非阻塞域名解析，基于c-ares。  
  
## 框架图
![image](https://yasio.org/images/framework.png)  
