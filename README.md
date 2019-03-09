# yasio - *Y*et *A*nother *S*ocket *IO* service
A lightweight & stable cross-platform support library with a focus on asynchronous socket I/O, support win32  &amp; linux  &amp; apple &amp; android &amp; wp8 &amp; wp8.1-universal &amp; win10-universal  
The core design is reference from https://github.com/chriskohlhoff/asio but very small.  
**This lib has been used by project http://hongjing.qq.com/ and run at millions of devices.**
## features: 
* support IPv6-only network.  
* support multi-connections at one thread.  
* support deadline timer.  
* processing tcp sticky internal, user do not need to care it.  
* support lua bindings  
* support cocos2d-x jsb  
* support CocosCreator jsb2.0  
* support Unity3D tolua
  
## simple usage:  
1. Only compile yasio\xxsocket.cpp with your project; For gcc, you must add --std=c++11 compile flag<br />
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
Compile yasio\xxsocket.cpp yasio\yasio.cpp with your project  
cpptest: https://github.com/halx99/yasio/blob/master/msvc/cpptest/cpptest.cpp  
  
    
### Lua intergation: 
#### common lua application integration:
1. Compile ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibinarystream.cpp``` ```yasio\obinarystream.cpp``` ```yasio\impl\lyasio.cpp``` with your project  
2. call luaopen_yasio after LUA VM initialized.  
luatest(windows user): open https://github.com/halx99/yasio/blob/master/msvc/luatest/luatest.xsxproj by x-studio365 IDE  
  
#### cocos2d-x lua integration:
1. Copy the folder ```yasio``` to cocos2d-x engine's folder ```external```, ```yasio``` as sub directory of ```external```  

2. Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibinarystream.cpp``` ```yasio\obinarystream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_cclua.cpp``` to compile with your solution.
##### For Android:
Add yasio to your application Android.mk files, such as:  
```
LOCAL_STATIC_LIBRARIES += yasio_static
$(call import-module, external/yasio/build/android-lua)
```
##### For Win32 & Apple platform:
Add ```yasio\xxsocket.cpp``` ```yasio\yasio.cpp``` ```yasio\ibinarystream.cpp``` ```yasio\obinarystream.cpp``` ```yasio\impl\lyasio.cpp```
```yasio\impl\yasio_cclua.cpp``` to libluacocos2d project of your Visual Studio or xcode solution.  

3. Call luaopen_yasio_cclua(L) at your AppDelegate.cpp, please remember ```#include "yasio/yasio_cclua.h"``` firstly.  

### JSB integration  
#### cocos2d-x jsb integration:
1. Copy all sources to your Classes\yasio and compile obinarystream.cpp, ibinarystream.cpp, xxsocket.cpp, yasio.cpp, impl/yasio_jsb.cpp with your native project  
2. Add register code to your AppDelegate.cpp: ```sc->addRegisterCallback(jsb_register_yasio);```, please include ```yasio_jsb.h``` firstly.
##### For Android:
Add yasio to your application Android.mk files, such as:  
```
LOCAL_STATIC_LIBRARIES += yasio_static
$(call import-module, external/yasio/build/android-jsb)
```
#### CocosCreator jsb2.0 integration:
1. Copy all sources to your Classes\yasio and compile obinarystream.cpp, ibinarystream.cpp, xxsocket.cpp, yasio.cpp, impl/yasio_jsb20.cpp with your native project  
2. Add register code to your jsb_module_register.cpp: ```se->addRegisterCallback(jsb_register_yasio);``` , please include ```yasio_jsb20.h``` firstly. 
  
#### JS demo:
see: https://github.com/halx99/yasio/blob/master/msvc/jstest/example.js  
showcase:  
![image](https://github.com/halx99/yasio/raw/master/showcasejsb.jpg)  
  
  
## Quick test at linux platform with gcc compiler:  
```g++ yasio/xxsocket.cpp yasio/yasio.cpp yasio/ibinarystream.cpp yasio/obinarystream.cpp msvc/cpptest/cpptest.cpp --std=c++11 -lpthread -I./ -o cpptest && ./cpptest```  
  
## pitfall: 
1. For Microsoft Visual Studio, if your project has specific precompiled header, you should include it at head of xxsocket.cpp or specific the compile option: ```C/C++ --> Advance -->'Forced Include File'``` to it(such as pch.h).  
2. For Microsoft Visual Studio 2019 preview3 MSVC C++17, you must set compile option: ```C/C++ --> Language --> Conformance mode``` to **No**, otherwise, you will got compiling error: ```sol.hpp(8060): error C3779: 'sol::stack::get': a function that returns 'decltype(auto)' cannot be used before it is defined```
  
## Unity tolua integration:  
see: https://github.com/halx99/LuaFramework_UGUI  
showcase:  
![image](https://github.com/halx99/yasio/raw/master/showcaseunity.png)  

## Simple proactor design mode:
The core objects: service, channel, transport  

service: event-loop, manage channel, transport, peek events  
channel: manage connection session  
transport: response for data transition  

client: per channel, per transport  
server: per channel, multi transports  

call service open:  
open a channel as client, the channel will try connect remote host    
open a channel as server, the channel will start listening  

events: connect response, connection lost, a packet received.  

