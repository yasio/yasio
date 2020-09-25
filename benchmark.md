## Devices:
  - Windows 10 & WSL2 & Ubuntu 20.04 On VMware 15.5: Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz / Windows 10(10.0.19041.264)
  - Linux: Intel(R) Xeon(R) Platinum 8163 CPU @ 2.50GHz / Ubuntu 20.04
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
  - WSL2: same compile with Linux
  - macOS 10.15.4: Apple Clang 11.0.0 Release build

## Send Parameters:
  - Total Time: 10(s)
  - KCP Send Interval: 1(ms)
  - Send Bytes Per Time: 62KB

## Results:
  - TCP speed: 
    - Windows: 22.4Gbits/s+
      - libuv: 22.4Gbits/s+
    - Ubuntu 20.04 On Aliyun: 18.4Gbits/s
    - Ubuntu 20.04 On VMware 15.5: 28Gbits/s
    - Android 10(MI MIX2S): 480Mbits/s 
  - UDP speed: 
    - Windows: 22.4Gbits/s
    - Ubuntu 20.04 On Aliyun: 20.8Gbits/s
    - Ubuntu 20.04 On VMware 15.5: 28Gbits/s
    - Android 10(MI MIX2S): 488Mbits/s
  - KCP speed: 
    - Windows: 232Mbits/s
    - macOS: 368Mbits/s
    - Ubuntu 20.04 On VMware 15.5: 240Mbits/s
    - Ubuntu 20.04 On WSL2: 2.03Gbits/s (send.interval=100us)
    - Ubuntu 20.04 On Aliyun: 426Mbits/s (aliyun server single core cpu), 1.9Gbits/s(send.interval=100us)
    - Android 10(MI MIX2S): 184Mbits/s
## About WSL2:
  - Microsoft docs: https://docs.microsoft.com/zh-cn/windows/wsl/wsl2-kernel
  - Microsoft repo: https://github.com/microsoft/WSL2-Linux-Kernel
  - How to install: https://github.com/mikemaccana/developing-with-wsl2-on-windows

## 分析结果：
  - ~~唯独阿里云Ubuntu测试结果异常，10s后，还在断断续续有接收，怀疑是安全规则问题~~、
  - 更新: 关于阿里云测试异常结果根本原因分析: 测试阿里云服务器是**单核CPU**，而原测试用例，**将kcp的interval设置成了0**，导致yasio的service占用100% CPU, 因此反而降低性能，保持**kcp.interval=10ms**，同时将发送kcp包间隔降低为100us，在阿里云服单核CPU本机传输性能可已达到**1.9Gbit/s**
  - Android CPU相对PC比较弱，因此UDP/TCP传输速率均在480Mbits/s左右

## 最新阿里云KCP本机传输性能测试结果:
```
[yasio][1601018268935][global] the yasio-3.33.4 is initialized, the size of per transport is 65952 when object_pool enabled.

Start trasnfer test via KCP after 170ms...
[yasio]xxsocket::getipsv: flags=1
[yasio]xxsocket::getipsv: flags=1
[yasio][1601018268936][index: 0] connecting server 127.0.0.1:3001...

[yasio][1601018268936][index: 0] the connection #1(0x7f66740017f0) [0.0.0.0:3002] --> [] is established.

The sender connected...
[yasio][1601018269106][index: 0] connecting server 127.0.0.1:3002...

[yasio][1601018269106][index: 0] the connection #2(0x7f667c0017f0) [0.0.0.0:3001] --> [] is established.

Speed: send=1.9Gbits/s recv=1.3Gbits/s, Total Time: 0.502221(s), Total Bytes: send=124753920 recv=122087424
Speed: send=1.9Gbits/s recv=1.6Gbits/s, Total Time: 1.00242(s), Total Bytes: send=251729920 recv=249952256
Speed: send=1.9Gbits/s recv=1.7Gbits/s, Total Time: 1.50254(s), Total Bytes: send=376102912 recv=375214080
Speed: send=1.9Gbits/s recv=1.7Gbits/s, Total Time: 2.00274(s), Total Bytes: send=500983808 recv=500158464
Speed: send=1.9Gbits/s recv=1.7Gbits/s, Total Time: 2.50292(s), Total Bytes: send=626690048 recv=625102848
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 3.003(s), Total Bytes: send=753031168 recv=751570944
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 3.50323(s), Total Bytes: send=878039040 recv=877531136
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 4.00331(s), Total Bytes: send=1003681792 recv=1003110400
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 4.50343(s), Total Bytes: send=1130340352 recv=1129641984
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 5.00347(s), Total Bytes: send=1258903552 recv=1257570304
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 5.5035(s), Total Bytes: send=1386070016 recv=1385498624
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 6.00361(s), Total Bytes: send=1514569728 recv=1513871360
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 6.5037(s), Total Bytes: send=1639069696 recv=1637038080
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 7.00381(s), Total Bytes: send=1767505920 recv=1766871040
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 7.50381(s), Total Bytes: send=1896069120 recv=1895434240
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 8.00388(s), Total Bytes: send=2024441856 recv=2023616512
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 8.50402(s), Total Bytes: send=2151925760 recv=2151100416
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 9.0042(s), Total Bytes: send=2276806656 recv=2276044800
Speed: send=1.9Gbits/s recv=1.8Gbits/s, Total Time: 9.50422(s), Total Bytes: send=2403592192 recv=2402766848
Speed: send=1.9Gbits/s recv=1.9Gbits/s, Total Time: 10.0119(s), Total Bytes: send=2531393536 recv=2529234944
Speed: send=1.9Gbits/s recv=1.9Gbits/s, Total Time: 10.5162(s), Total Bytes: send=2531393536 recv=2531393536
```
