# yasio - *Y*et *A*nother *S*ocket *IO* service
[![Build Status](https://travis-ci.org/halx99/yasio.svg?branch=master)](https://travis-ci.org/halx99/yasio)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/halx99/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/halx99/yasio.svg?label=Stars)](https://github.com/halx99/yasio)
[![GitHub forks](https://img.shields.io/github/forks/halx99/yasio.svg?label=Fork)](https://github.com/halx99/yasio)

A lightweight & stable cross-platform support library with a focus on asynchronous socket I/O, support win32  &amp; linux  &amp; apple &amp; android &amp; wp8 &amp; wp8.1-universal &amp; win10-universal  
The core design is reference from https://github.com/chriskohlhoff/asio but very small.  
**This lib has been used by project http://hongjing.qq.com/ and run at millions of devices.**
## Features: 
* support IPv6-only network.  
* support multi-connections at one thread.  
* support deadline timer.  
* processing tcp sticky internal, user do not need to care it.  
* support lua bindings  
* support cocos2d-x jsb  
* support CocosCreator jsb2.0  
* support Unity3D
* per io_service, per thread to process socket read,write,connect operations.  
  
## Intergation guides:  
### cpp intergation: 
Compile yasio\xxsocket.cpp yasio\yasio.cpp with your project  
cpptest: https://github.com/halx99/yasio/blob/master/msvc/cpptest/cpptest.cpp  
  
    
### Lua intergation: 
#### common lua application integration:
1. Compile ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibstream.cpp``` ```yasio\obstream.cpp``` ```yasio\impl\lyasio.cpp``` with your project  
2. call luaopen_yasio after LUA VM initialized.  
luatest(windows user): open https://github.com/halx99/yasio/blob/master/msvc/luatest/luatest.xsxproj by x-studio365 IDE  
  
#### cocos2d-x lua integration:
1. Copy the folder ```yasio``` to cocos2d-x engine's folder ```external```, ```yasio``` as sub directory of ```external```  

2. Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibstream.cpp``` ```yasio\obstream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_cclua.cpp``` to your compile system.
    + For Win32 & Apple platform:
Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibstream.cpp``` ```yasio\obstream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_cclua.cpp``` to **libluacocos2d** project of your Visual Studio or xcode solution.  
    + For Android:
Add yasio to your application Android.mk files, such as:  
```
LOCAL_STATIC_LIBRARIES += yasio_static
$(call import-module, external/yasio/build/android-lua)
```

3. Call luaopen_yasio_cclua(L) at your AppDelegate.cpp, please remember ```#include "yasio/yasio_cclua.h"``` firstly.  

### JSB integration  
#### cocos2d-x jsb integration:
1. Copy the folder ```yasio``` to cocos2d-x engine's folder ```external```, ```yasio``` as sub directory of ```external``` 
2. Add obstream.cpp, ibstream.cpp, xxsocket.cpp, yasio.cpp, impl/yasio_jsb.cpp to your compile system.
    + For Win32 & Apple platform:
Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibstream.cpp``` ```yasio\obstream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_jsb.cpp``` to **libjscocos2d** project of your Visual Studio or xcode solution. 
    + For Android: Add yasio to your application Android.mk files, such as: 
```
LOCAL_STATIC_LIBRARIES += yasio_static
$(call import-module, external/yasio/build/android-jsb)
```
3. Add register code to your AppDelegate.cpp: ```sc->addRegisterCallback(jsb_register_yasio);```, please include ```yasio_jsb.h``` firstly.
#### CocosCreator jsb2.0 integration:
1. Copy the folder ```yasio``` to cocos2d-x engine's folder ```external/sources```, ```yasio``` as sub directory of ```external/sources```  
2. Add obstream.cpp, ibstream.cpp, xxsocket.cpp, yasio.cpp, impl/yasio_jsb20.cpp to your compile system.
    + For Win32 & Apple platform: Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibstream.cpp``` ```yasio\obstream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_jsb.cpp``` to **libcocos2d** project of your Visual Studio or xcode solution.  
    + For Andorid: Add follow 5 lines to cocos/Android.mk file:
```
../external/sources/yasio/xxsocket.cpp \
../external/sources/yasio/yasio.cpp \
../external/sources/yasio/ibstream.cpp \
../external/sources/yasio/obstream.cpp \
../external/sources/yasio/impl/yasio_jsb20.cpp \
```
Just like:  
![image](https://github.com/halx99/yasio/raw/master/jsb20_android.png)  
3. Add register code to your jsb_module_register.cpp: ```se->addRegisterCallback(jsb_register_yasio);``` , please include ```yasio_jsb20.h``` firstly.  


#### JS demo:
see: https://github.com/halx99/yasio/blob/master/msvc/jstest/example.js  
showcase:  
![image](https://github.com/halx99/yasio/raw/master/showcasejsb.jpg)  
  
  
## Quick test at linux platform with gcc compiler:  
```g++ yasio/xxsocket.cpp yasio/yasio.cpp yasio/ibstream.cpp yasio/obstream.cpp msvc/cpptest/cpptest.cpp --std=c++11 -lpthread -I./ -o cpptest && ./cpptest```  
  
## pitfall: 
1. For Microsoft Visual Studio, if your project has specific precompiled header, you should include it at head of xxsocket.cpp or specific the compile option: ```C/C++ --> Advance -->'Forced Include File'``` to it(such as pch.h).  
2. For Microsoft Visual Studio 2019 MSVC C++17, you must set compile option: ```C/C++ --> Language --> Conformance mode``` to **No**, otherwise, you will got compiling error: ```sol.hpp(8060): error C3779: 'sol::stack::get': a function that returns 'decltype(auto)' cannot be used before it is defined```
  
## Unity3D integration guide
Tencent xLua integration, see: https://github.com/halx99/xLua

## For more detail usage: see example or wiki: https://github.com/halx99/yasio/wiki

