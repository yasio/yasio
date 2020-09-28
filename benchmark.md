## Devices:
  - Windows 10: Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz / Windows 10(10.0.19041.264)
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
    - Windows: 11.0Gbits/s
    - macOS: 21.0Gbits/s
    - Ubuntu 20.04 On Aliyun: 2.3~5.3Gbits/s, because it's Single Core CPU, so speed not stable
    - Android 10(MI MIX2S): 184Mbits/s (kcp.send.internval=1ms)

## 注意事项
  - 多核CPU，当 ```io_service``` 任务饱和时可将 ```wait_duration``` 设置为**0**，以便事件循环在下次tick立刻执行任务
  - 单核CPU，当 ```io_service``` 任务饱和时至少要给一定的 ```wait_duration``` 到```socket.select```，详见[提交记录](https://github.com/yasio/yasio/commit/0a549fdd558a17b75da3923d36e63c3c77904041)，以防止占满CPU降低整体传输性能，例如阿里云**单核CPU**服务器，最早测试用例里将 ```kcp.interval``` 设置成了**0**，传输性能很低，只有**40Mbits/s**，后来保持 ```kcp.interval=10ms```，同时将发包间隔降低为**100us**，传输性能提高到**1.9Gbit/s**
  - Android CPU相对PC比较弱，因此UDP/TCP传输速率均在480Mbits/s左右
