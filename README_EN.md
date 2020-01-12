# yasio - *Y*et *A*nother *S*ocket *IO* service
[![Build Status](https://travis-ci.com/simdsoft/yasio.svg?branch=master)](https://travis-ci.com/simdsoft/yasio)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/fnd3fji4dss7ppsd/branch/master?svg=true)](https://ci.appveyor.com/project/halx99/yasio)
[![Release](https://img.shields.io/badge/release-v3.31.2-blue.svg)](https://github.com/simdsoft/yasio/releases)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/simdsoft/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/simdsoft/yasio.svg?label=Stars)](https://github.com/simdsoft/yasio)
[![GitHub forks](https://img.shields.io/github/forks/simdsoft/yasio.svg?label=Fork)](https://github.com/simdsoft/yasio)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/simdsoft/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/simdsoft/yasio/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/simdsoft/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/simdsoft/yasio/alerts/)

**yasio** is a multi-platform support library with focus on asynchronous TCP socket I/O for any client application, support win32 & linux & apple & android & wp8 & wp8.1-universal & win10-universal.  
The core design is reference from [asio](https://github.com/chriskohlhoff/asio) but very small.  
**This lib has been used by project [RedAlert OL](https://hongjing.qq.com/) and run at millions of devices.**

## Usage
### C++
```cpp
#include "yasio/yasio.hpp"
using namespace yasio;
using namespace yasio::inet;

int main()
{
  io_service service({host="www.ip138.com", port=80});
  service.set_option(YOPT_S_DEFERRED_EVENT, 0); // dispatch event at netwrok thread
  service.start_service([&](event_ptr&& ev) {
    switch (ev->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(ev->packet());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fflush(stdout);
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (ev->status() == 0)
        {
          auto transport = ev->transport();
          if (ev->cindex() == 0)
          {
            obstream obs;
            obs.write_bytes("GET /index.htm HTTP/1.1\r\n");

            obs.write_bytes("Host: www.ip138.com\r\n");

            obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                            "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/79.0.3945.117 Safari/537.36\r\n");
            obs.write_bytes("Accept: */*;q=0.8\r\n");
            obs.write_bytes("Connection: Close\r\n\r\n");

            service.write(transport, std::move(obs.buffer()));
          }
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost.\n");
        break;
    }
  });
  // open channel 0 as tcp client
  service.open(0, YCM_TCP_CLIENT);
  getchar();
}
```

### Lua
```lua
local host = "www.ip138.com"
local service = yasio.io_service.new({host, 80})
local respdata = ""
service.start_service(function(ev)
        local k = ev.kind()
        if (k == yasio.YEK_PACKET) then
            respdata = respdata .. ev:packet():to_string()
        elseif k == yasio.YEK_CONNECT_RESPONSE then
            if ev:status() == 0 then
                local transport = ev:transport()
                local obs = yasio.obstream.new()
                obs.write_bytes("GET / HTTP/1.1\r\n")

                obs.write_bytes("Host: " .. host .. "\r\n")

                obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.117 Safari/537.36\r\n")
                obs.write_bytes("Accept: */*;q=0.8\r\n")
                obs.write_bytes("Connection: Close\r\n\r\n")

                service.write(transport, obs)
            end
        elseif k == yasio.YEK_CONNECTION_LOST then
            print("request finish, respdata: " ..  respdata)
        end
    end)
-- open channel 0 as tcp client
service.open(0, yasio.YCM_TCP_CLIENT)

-- should be call at the thread of lua_State, for game engine, it's should be renderer loop.
function gDispatchNetworkEvent(...)
    service.dispatch(128) -- dispatch max event is 128 per frame
end

_G.yservice = service -- Store service to global table as a singleton instance
```

## Simple run tcptest with g++
```sh
g++ tests/tcp/main.cpp --std=c++11 -DYASIO_HEADER_ONLY -lpthread -I./ -o tcptest && ./tcptest
```

## Build more examples with cmake
```sh
git clone https://github.com/simdsoft/yasio
cd yasio
git submodule update --init --recursive
cd build
cmake ..

# Use cmake command to build examples & tests, 
# or open yasio.sln with visual studio(2013~2019 supported) at win32 platform
cmake --build . --config Debug
```

## Features: 
* support IPv6-only network.  
* support multi-connections at one thread.  
* support deadline timer.  
* support unpack tcp packet internal, user do not need to care it.  
* support lua bindings  
* support cocos2d-x jsb  
* support CocosCreator jsb2.0  
* support Unity3D
* support multicast
* support ssl client with openssl
* support async resolve with c-ares
  
## Core framework
![image](https://github.com/simdsoft/yasio/blob/master/framework.png)  

## For more detail usage: see [wiki](https://github.com/simdsoft/yasio/wiki)

