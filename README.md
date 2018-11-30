# mini-asio
A lightweight & stable cross platform posix socket API wrapper, support win32  &amp; linux  &amp; apple &amp; android &amp; wp8 &amp; wp8.1-universal &amp; win10-universal  
Used by http://hongjing.qq.com/  
**support IPv6-only network.  
support multi-connections at one thread.  
support deadline timer.**
  
Usage(less v4.0, since v4.0 c++17 required):

1. Only compile src\xxsocket.cpp src\deadline_timer.cpp src\async_socket_io.cpp with your project; For gcc, you must add --std=c++11 compile flag<br />
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

ASIO Usage(Client Program):
```
cpptest: https://github.com/halx99/mini-asio/blob/master/test/test/simple_test.cpp
luatest: open https://github.com/halx99/mini-asio/blob/master/test/test/luatest.xsxproj by x-studui365 ide
```

#pitfall: For Microsoft Visual Studio, if your project has specific precompiled header, you should include it at head of xxsocket.cpp or specific the compile option: C/C++ --> Advance -->'Forced Include File' to it(such as pch.h).
