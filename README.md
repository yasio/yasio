# mini-asio
A lightweight & stable cross platform posix socket API wrapper, support win32  &amp; linux  &amp; apple &amp; android &amp; wp8 &amp; wp8.1-universal &amp; win10-universal  
The core design is reference from https://github.com/chriskohlhoff/asio but very small.  
This lib is used by project http://hongjing.qq.com/ and run at thousands of devices.  
**support IPv6-only network.  
support multi-connections at one thread.  
support deadline timer.  
processing tcp sticky internal, user do not need to care it.**
  
#Simiple usage:  
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

#Asio usage:  
cpp intergation: Compile src\xxsocket.cpp src\masio.cpp with your project  
cpptest: https://github.com/halx99/mini-asio/blob/master/test/test/cpptest.cpp  
  
    
lua intergation: 
1. Compile src\xxsocket.cpp src\masio.cpp src\ibinarystream.cpp src\obinarystream.cpp [src\lmasio.cpp(for c++17) or src\lmasio11.cpp(for c++11)] with your project  
2. call lua_open_masio after LUA VM initialized.  
luatest(windows user): open https://github.com/halx99/mini-asio/blob/master/test/test/luatest.xsxproj by x-studui365 ide  

#Quick test at linux platform with gcc compiler:  
```g++ src/xxsocket.cpp src/masio.cpp test/test/cpptest.cpp --std=c++11 -lpthread -I./src -o cpptest && ./cpptest```  
  
#pitfall: For Microsoft Visual Studio, if your project has specific precompiled header, you should include it at head of xxsocket.cpp or specific the compile option: C/C++ --> Advance -->'Forced Include File' to it(such as pch.h).
