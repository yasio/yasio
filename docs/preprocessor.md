# yasio 宏定义

以下宏定义可以控制 `yasio` 库的某些行为，可以在 [yasio/detail/config.hpp](https://github.com/yasio/yasio/blob/dev/yasio/detail/config.hpp) 定义或者在编译器预处理器定义

|Name|Description|
|----------|-----------------|
|*YASIO_HAVE_KCP*|是否启用kcp传输支持，需要kcp已经在软件编译系统中，默认关闭。|
|*YASIO_HEADER_ONLY*|是否以仅头文件的方式使用yasio核心组件，默认关闭。|
|*YASIO_SSL_BACKEND*|选择SSL库以支持SSL客户端，需要软件编译系统包含OpenSSL/MbedTLS库，<br/>3.36.0新增(同时移除YASIO_HAVE_SSL)，此宏只能取值 `1`(使用OpenSSL) 或者 `2` (使用mbedtls)。|
|*YASIO_ENABLE_UDS*|是否启用unix domain socket支持，目前仅类unix系统和win10 RS5+支持，默认关闭。|
|*YASIO_HAVE_CARES*|是否启用c-ares异步域名解析库，<br/>当编译系统包含c-ares时可启用，有效避免每次解析域名都新开线程。<br/>yasio有DNS缓存机制，超时时间默认10分钟，<br/>因此无c-ares也不会造成太大的性能损耗。|
|*YASIO_VERBOSE_LOG*|是否打印详细日志，默认关闭。|
|*YASIO_NT_COMPAT_GAI*|是否启用Windows XP系统下使用 `getaddrinfo` API支持。|
|*YASIO_USE_SPSC_QUEUE*|是否使用SPSC(单生产者单消费者)队列，<br/>仅当只有一个线程调用io_service::write时放可启用，默认关闭。|
|*YASIO_USE_SHARED_PACKET*|是否使用 `std::shared_ptr` 包装网络包，使其能在多线程之间共享，默认关闭。|
|*YASIO_HAVE_HALF_FLOAT*|是否启用半精度浮点数支持，依赖 [half.hpp](https://github.com/yasio/thirdparty/blob/master/half/half.hpp)。|
|*YASIO_DISABLE_OBJECT_POOL*|是否禁用对象池的使用，默认启用。|
|*YASIO_DISABLE_CONCURRENT_SINGLETON*|是否禁用并发单利类模板。|
|*YASIO_ENABLE_PASSIVE_EVENT*|是否启用服务端信道open/close事件产生，默认关闭。|
|*YASIO_DISABLE_POLL*|是否禁用`poll`，默认启用。自3.39.6，底层多路io复用模型使用`poll`，如需继续使用`select`模型，定义此预处理器即可|
