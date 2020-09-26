## Devices:
  - Windows 10 & WSL2 & Ubuntu 20.04 On VMware 15.5: Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz / Windows 10(10.0.19041.264)
  - Linux: Intel(R) Xeon(R) Platinum 8163 CPU @ 2.50GHz / Ubuntu 20.04 (Single Core CPU)
  - macOS: Intel(R) Core(TM) i7-8850H CPU @ 2.60GHz / macOS 10.15.4
  - Android: XIAOMI MIX2S (cocos2d-x game engine cpp-tests)

## Test Code: [speedtest](https://github.com/yasio/yasio/blob/master/tests/speed/main.cpp)

## Architecture: 
  - PC: X64
  - Android: armv7a

## Compiling:
  - Windows: VS2019 MSVC 14.25.28610
    Optimize Flag: /O2
    Commands:
      - mkdir build/build_w64
      - cd build/build_w64
      - cmake ../../
      - cmake --build . --config Release --target speedtest
  - Linux: gcc version 9.3.0 (Ubuntu 9.3.0-10ubuntu2)
    Optimize Flag: /O3
    Commands:
      - mkdir -p build/build_linux
      - cd build/build_linux
      - cmake ../../ -DCMAKE_BUILD_TYPE=Release
      - cmake --build . --config Release --target speedtest
  - macOS 10.15.4: Apple Clang 11.0.0 Release build

## Send Parameters:
  - Total Time: 10(s)
  - KCP Send Interval: 10(us)
  - Send Bytes Per Time: 62KB

## Results:
  - TCP speed: 
    - Windows: 22.4Gbits/s+
      - libuv: 22.4Gbits/s+
    - macOS: 29.9Gbits/s
    - Ubuntu 20.04 On Aliyun: 18.4Gbits/s
    - Android 10(MI MIX2S): 480Mbits/s 
  - UDP speed: 
    - Windows: 22.4Gbits/s
    - macOS: send=35.5Gbits/s, recv=25.8Gbits/s, lost=9.7Gbits/s
    - Ubuntu 20.04 On Aliyun: 20.8Gbits/s
    - Android 10(MI MIX2S): 488Mbits/s
  - KCP speed: 
    - Windows: 5.0Gbits/s
    - macOS: 16.6Gbits/s
    - Ubuntu 20.04 On Aliyun: 2.3~5.3Gbits/s, because it's Single Core CPU, so speed not stable
    - Android 10(MI MIX2S): 184Mbits/s (kcp.send.internval=1ms)
## About WSL2:
  - Microsoft docs: https://docs.microsoft.com/zh-cn/windows/wsl/wsl2-kernel
  - Microsoft repo: https://github.com/microsoft/WSL2-Linux-Kernel
  - How to install: https://github.com/mikemaccana/developing-with-wsl2-on-windows

## 分析结果：
  - ~~唯独阿里云Ubuntu测试结果异常，10s后，还在断断续续有接收，怀疑是安全规则问题~~、
  - 更新: 关于阿里云测试异常结果根本原因分析: 测试阿里云服务器是**单核CPU**，而原测试用例，**将kcp的interval设置成了0**，导致yasio的service占用100% CPU, 因此反而降低性能，保持**kcp.interval=10ms**，同时将发送kcp包间隔降低为100us，在阿里云服单核CPU本机传输性能可已达到**1.9Gbit/s**
  - Android CPU相对PC比较弱，因此UDP/TCP传输速率均在480Mbits/s左右
