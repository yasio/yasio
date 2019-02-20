# yasio - yet another socket io service
A lightweight & stable cross-platform support library with a focus on asynchronous socket I/O, support win32  &amp; linux  &amp; apple &amp; android &amp; wp8 &amp; wp8.1-universal &amp; win10-universal  
The core design is reference from https://github.com/chriskohlhoff/asio but very small.  
**This lib has been used by project http://hongjing.qq.com/ and run at millions of devices.  
support IPv6-only network.  
support multi-connections at one thread.  
support deadline timer.  
processing tcp sticky internal, user do not need to care it.**
  
## simple usage:  
1. Only compile src\xxsocket.cpp with your project; For gcc, you must add --std=c++11 compile flag<br />
2. demo code:
```
#include "xxsocket.h"
using namespace purelib::inet;
void test_connect() 
{
   xxsocket clientsock;
   // The interface xpconnect_n will detect whether localhost is IPV6 only network automatically
   // and connect the ipv4 server by V4MAPPED address.
   if(0 == clientsock.xpconnect_n("www.baidu.com", 443, std::chrono::seconds(5)/* connect timeout 5 seconds */))
   {
       printf("connect succeed\n");
   }
}
```

## asio usage:  
### cpp intergation: 
Compile src\xxsocket.cpp src\yasio.cpp with your project  
cpptest: https://github.com/halx99/yasio/blob/master/msvc/cpptest/cpptest.cpp  
  
    
### Lua intergation: 
1. Compile ```src\xxsocket.cpp``` ```src\yasio.cpp``` ```src\ibinarystream.cpp``` ```src\obinarystream.cpp``` ```src\lyasio.cpp``` with your project  
2. call luaopen_yasio after LUA VM initialized.  
luatest(windows user): open https://github.com/halx99/yasio/blob/master/msvc/luatest/luatest.xsxproj by x-studio365 IDE  
  

### JSB integration  
#### cocos2d-x jsb integration:
1. Copy all sources to your Classes\yasio and compile obinarystream.cpp, ibinarystream.cpp, xxsocket.cpp, yasio.cpp, yasio_jsb.cpp with your native project  
2. Add register code to your AppDelegate.cpp: ```sc->addRegisterCallback(register_yasio_bindings);```, please include yasio_jsb.h firstly.
  
#### CocosCreator jsb2.0 integration:
1. Copy all sources to your Classes\yasio and compile obinarystream.cpp, ibinarystream.cpp, xxsocket.cpp, yasio.cpp, yasio_jsb20.cpp with your native project  
2. Add register code to your jsb_module_register.cpp: ```se->addRegisterCallback(register_all_yasio);``` , please include yasio_jsb20.h firstly. 
  
#### JS demo:
see: https://github.com/halx99/yasio/blob/master/msvc/jstest/example.js  
showcase:  
![image](https://github.com/halx99/yasio/raw/master/showcasejsb.jpg)  
  
  
## Quick test at linux platform with gcc compiler:  
```g++ src/xxsocket.cpp src/yasio.cpp src/ibinarystream.cpp src/obinarystream.cpp msvc/cpptest/cpptest.cpp --std=c++11 -lpthread -I./src -o cpptest && ./cpptest```  
  
## pitfall: 
1. For Microsoft Visual Studio, if your project has specific precompiled header, you should include it at head of xxsocket.cpp or specific the compile option: ```C/C++ --> Advance -->'Forced Include File'``` to it(such as pch.h).  
2. For MSVC C++17, you must set compile option: ```C/C++ --> Language --> Conformance mode``` to **No**, otherwise, you will got compiling error: ```sol.hpp(8060): error C3779: 'sol::stack::get': a function that returns 'decltype(auto)' cannot be used before it is defined```
  
## Unity tolua integration:  
see: https://github.com/halx99/LuaFramework_UGUI  
showcase:  
![image](https://github.com/halx99/yasio/raw/master/showcaseunity.png)  

