# *YASIO* - *A*in't *S*ocket *IO*, *Y*et *A*nother asynchronous *S*ocket *I*/*O*.
[![Build Status](https://travis-ci.com/yasio/yasio.svg?branch=master)](https://travis-ci.com/yasio/yasio)
[![Windows Build Status](https://ci.appveyor.com/api/projects/status/d6qjfygtw2ewt9pf/branch/master?svg=true)](https://ci.appveyor.com/project/halx99/yasio)
[![Release](https://img.shields.io/badge/release-v3.33.0-blue.svg)](https://github.com/yasio/yasio/releases)
[![996.icu](https://img.shields.io/badge/link-996.icu-red.svg)](https://996.icu)
[![LICENSE](https://img.shields.io/badge/license-Anti%20996-blue.svg)](https://github.com/yasio/yasio/blob/master/LICENSE)
[![GitHub stars](https://img.shields.io/github/stars/yasio/yasio.svg?label=Stars)](https://github.com/yasio/yasio)
[![GitHub forks](https://img.shields.io/github/forks/yasio/yasio.svg?label=Fork)](https://github.com/yasio/yasio)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/context:cpp)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/yasio/yasio.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/yasio/yasio/alerts/)  
  
[![Powered](https://img.shields.io/badge/Powered%20by-c4games-blue.svg)](http://c4games.com)  
  
**[English](README_EN.md)**

**yasio** 是一个轻量级跨平台的异步socket库，专注于客户端和基于各种游戏引擎的游戏客户端网络服务, 支持windows & linux & apple & android & win10-universal。  

## 应用案例
* [红警OL手游项目](https://hongjing.qq.com/): 用于客户端网络传输，并且随着该项目于2018年10月17日由腾讯游戏发行正式上线后稳定运行于上千万移动设备上。
* [x-studio软件项目](https://x-studio.net/): 用于实现局域网UDP+TCP发现更新机制。
* [xlua集成](https://github.com/c4games/xlua): 将yasio集成到xlua, 使基于xlua的unity3d可以直接使用yasio的lua绑定接口。
    
## 用法
### C++
```cpp
#include "yasio/yasio.hpp"
#include "yasio/obstream.hpp"
using namespace yasio;
using namespace yasio::inet;

int main()
{
  io_service service({"www.ip138.com", 80});
  service.set_option(YOPT_S_DEFERRED_EVENT, 0); // 直接在网络线程分派网络事件
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
local ip138 = "www.ip138.com"
local service = yasio.io_service.new({host=ip138, port=80})
local respdata = ""
-- 传入网络事件处理函数启动网络服务线程，网络事件有: 消息包，连接响应，连接丢失
service.start_service(function(ev)
        local k = ev.kind()
        if (k == yasio.YEK_PACKET) then
            respdata = respdata .. ev:packet():to_string()
        elseif k == yasio.YEK_CONNECT_RESPONSE then
            if ev:status() == 0 then -- status为0表示连接建立成功
                local transport = ev:transport()
                local obs = yasio.obstream.new()
                obs.write_bytes("GET / HTTP/1.1\r\n")

                obs.write_bytes("Host: " .. ip138 .. "\r\n")

                obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/79.0.3945.117 Safari/537.36\r\n")
                obs.write_bytes("Accept: */*;q=0.8\r\n")
                obs.write_bytes("Connection: Close\r\n\r\n")

                service.write(transport, obs)
            end
        elseif k == yasio.YEK_CONNECTION_LOST then
            print("request finish, respdata: " ..  respdata)
        end
    end)
-- 将信道0作为TCP客户端打开，并向服务器发起异步连接，进行TCP三次握手
service.open(0, yasio.YCM_TCP_CLIENT)

-- 由于lua_State和渲染对象，不支持在其他线程操作，因此分派网络事件封装为全局Lua函数，并且以下函数应该在主线程或者游戏引擎渲染线程调用
function gDispatchNetworkEvent(...)
    service.dispatch(128) -- 每帧最多处理128个网络事件
end

_G.yservice = service -- Store service to global table as a singleton instance
```

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
* 支持计时器。  
* 支持TCP粘包处理，业务完全不必关心。  
* 支持Lua绑定。  
* 支持Cocos2d-x jsb绑定。  
* 支持CocosCreator jsb2.0绑定。  
* 支持Unity3D C#绑定。  
* 支持组播。  
* 支持SSL客户端，基于OpenSSL。  
* 支持非阻塞域名解析，基于c-ares。  
  
## yasio核心设计，充分利用了多路io复用模型(服务器高并发的基石)，以下是框架图
![image](https://github.com/yasio/yasio/blob/master/framework.png)  

## 集成指南
[https://github.com/yasio/yasio/wiki/Integrate-Guides](https://github.com/yasio/yasio/wiki/Integrate-Guides)

## 更多详细用法，请查看 [文档](https://github.com/yasio/yasio/tree/master/docs)
