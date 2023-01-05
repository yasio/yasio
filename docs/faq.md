# FAQ

## 重点问题解答

??? question "do_select failed, ec=22, detail:Invalid argument"
    
    - 原因: 
        - macOS/ios/tvos平台，`socket.max_fd + 1`直大于`FD_SETSIZE(1024)`
        - Windows平台工作成长，限制的是`select`能够监听的文件描述符数量，但不受`socket.max_fd + 1`限制
        - linux平台，受到文件描述符`socket.max_fd + 1`限制，超过会立即返回1，但实际上无数据可读
        - freebsd平台，立即返回1，即使有数据可读，`is_set`判断始终返回false
    解决办法: 升级至3.39.6+版本，使用`poll`，不再受到任何限制

??? question "在iOS 14+设备上连接本地局域网主机任意端口号报错：ec=65, detail:No route to host"

    - 解决方案: 打开你的iPhone【设置】【隐私】【本地网络】找到你的应用，开启即可。

??? question "在iOS设备上连接服务器端口号为10161时失败，错误信息：ec=65, detail:No route to host"

    - 根本原因:
        - 当APP初次启动时，在ios14.1+设备上，会提示请求 `Local Network Permission` 权限，用户点拒绝。
        - 端口号 10161 是系统保留端口，用于 `SNMP via TLS` 协议。 
    - 解决方案: 使用其他端口号作为APP服务端口，建议使用动态/私密端口，范围： 49152~65535。
    - 其他参考: https://www.speedguide.net/port.php

??? question "yasio是否依赖其他网络库?"

    yasio的核心代码默认不依赖任何第三方库，从作者从业踏入通信行业开始就琢磨着编写一个轻量而好用的通用网络库，xxsocket就是yasio的起源，只有异步消息发送的软中断器提取于boost.asio。

??? question "为什么使用yasio?"

    1. 开源社区已知比较有名的网络库有asio,libevent,libev,libuv，他们提供的都是非常基础的非阻塞多路io复用模型，并且，各平台底层会使用iocp,kqueue,epoll,select等模型，拿来做客户端网络，如连接管理，TCP粘包处理等都需要程序员自己处理。
    2. yasio将连接管理，TCP拆包都封装到了底层。
    3. yasio将TCP,UDP,KCP统一抽象成Transport更加方便使用。
    4. yasio更轻量级，所有平台均使用select模型。

??? question "yasio是否支持非阻塞域名解析?"

    支持，但需要依赖c-ares库，这个库是完全实现DNS协议，并可以很好的和现有select模型结合使用，域名解析无需新开线程。如果不开启c-ares，yasio内部会为每次域名解析开线程。但会默认缓存10分钟，所以也不用担心开线程太频繁。

??? question "yasio是否支持SSL/TLS传输?"

    支持，但需要依赖OpenSSL或者MbedTLS。

??? question "如何从 io_event 中设置和获取 userdata"

    请参考：https://github.com/yasio/ftp_server/blob/master/ftp_server.cpp#L98

??? question "yasio是否处理 SIGPIPE 信号？"

    从 `3.30` 版本开始已处理。  
    如果不处理，实测过iOS在加载资源比较耗时会触发SIGPIPE直接导致APP闪退，处理后，仅仅触发TCP连接断开，错误码32，错误信息: `Broken pipe`  
    详见: https://github.com/yasio/yasio/issues/170

??? question "macOS 上发送UDP失败，报错误码40，错误信息 `Message too long` 怎么办？"

    - 原因: 数据包太大，macOS系统UDP发送缓冲区默认为 `9126` 字节。
    - 解决方案: 通过socket选项将UDP发送缓冲区设置大一点，例如:
        - xxsocket接口设置方式为: `sock_udp.set_optval(SOL_SOCKET, SO_SNDBUF, (int)65535);`
        - io_service设置方式请参见: https://github.com/yasio/yasio/blob/dev/tests/speed/main.cpp#L223

??? question "io_service schedule 可以多个任务吗？"

    可以。

??? question "std::thread 在某些嵌入式平台编译器闪退怎么办？"

    修改编译参数，详见: https://github.com/yasio/yasio/issues/244

??? question "xxsocket 函数命名后缀_n和_i的意思？"

    _n是nonblock的意思，_i是internal(新版本已去除)。
    
??? question "设置了YOPT_TCP_KEEPALIVE，还需要应用层心跳吗？"

    一般不需要，除非需要检测网络延时展示给用户，详见: https://github.com/yasio/yasio/issues/117

??? question "Lua绑定闪退怎么办？"

    * c++11:
        * 使用kaguya绑定库，但这个库有个问题：在绑定c++类的过程中，构造c++对象过程是先通过lua_newuserdata, 再通过[placement new](https://en.cppreference.com/w/cpp/language/new)构造对象，在xcode clang release优化编译下会直接闪退
        * 经过bing.com搜索，发现可通过定义```LUAI_USER_ALIGNMENT_T=max_align_t```来解决
        * 经过测试, max_align_t在xcode clang下定义为long double类型，长度为16个字节
        * 参考: http://lua-users.org/lists/lua-l/2019-07/msg00197.html
    * c++14:
        * 使用[sol2](https://github.com/ThePhD/sol2)绑定库，sol2可以成功解决kaguya的问题，经过查看sol2的源码，发现其在内部对lua_newuserdata返回的地址做了对齐处理，因此成功避免了clang release优化地址不对齐闪退问题
    * c++17:
        *  同样使用[sol2](https://github.com/ThePhD/sol2)库，但xcode会提示ios11以下不支持C++17默认```new```操作符的```Aligned allocation/deallocation```，通过添加编译选项-faligned-allocation可通过编译；
        * ios10以下不支持stl的shared_mutex(yasio最近做了兼容，对于Apple平台一律使用pthread实现)

    * 思考：
        * Lua虚拟机实现中默认最大对齐类型是double，而```max_align_t```类型是C11标准才引入的： https://en.cppreference.com/w/c/types/max_align_t ，因此Lua作为ANSI C89标准兼容实现并未完美处理各个编译器地址对齐问题，而是留给了用户定义: ```LUAI_USER_ALIGNMENT_T```
        * 在C++11编译系统下已经引入 ```std::max_align_t```类型: https://en.cppreference.com/w/cpp/types/max_align_t

    **重要: max_align_t各平台定义请查看llvm项目的 [__stddef_max_align_t.h](https://github.com/llvm/llvm-project/blob/main/clang/lib/Headers/__stddef_max_align_t.h)**

??? question "Can't load xxx.bundle on macOS?" 

    The file `xxx.bundle` needs change attr by command `sudo xattr -r -d com.apple.quarantine xxx.bundle`  

??? question "xxsocket的resolve系列函数socktype参数作用？"

    - winsock实现可以忽略
    - 其他操作系统如果设置为0会返回多个地址，即使直接传IP。

## 更多常见问题

请参阅 [项目帮助台](https://github.com/yasio/yasio/issues?q=is%3Aissue+label%3AHelpDesk+is%3Aclosed)
