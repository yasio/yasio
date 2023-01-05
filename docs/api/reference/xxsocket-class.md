---
title: "yasio::inet::xxsocket Class"
date: "4/11/2021"
f1_keywords: ["xxsocket", "yasio/xxsocket", ]
helpviewer_keywords: []
---

# xxsocket Class

封装底层bsd socket常用API，屏蔽各操作系统平台差异，yasio的起源。

!!! attention "特别注意"

    xxsocket除了 `accept_n`以外的所有 `xxx_n` 接口均会将当前socket对象底层描述符设置为非阻塞模式，且不会恢复。

## 语法

```cpp
namespace yasio { inline namespace inet { class xxsocket; } }
```

## 成员

|Name|Description|
|----------|-----------------|
|[xxsocket::xxsocket](#xxsocket)|构造一个 `xxsocket` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[xxsocket::xpconnect](#xpconnect)|建立TCP连接|
|[xxsocket::xpconnect_n](#xpconnect_n)|非阻塞方式建立TCP连接|
|[xxsocket::pconnect](#pconnect)|建立TCP连接|
|[xxsocket::pconnect_n](#pconnect_n)|非阻塞方式建立TCP连接|
|[xxsocket::pserve](#pserve)|创建tcp服务端|
|[xxsocket::swap](#swap)|交换socket描述符句柄|
|[xxsocket::open](#open)|打开socket|
|[xxsocket::reopen](#reopen)|重新打开socket|
|[xxsocket::is_open](#is_open)|检查socket是否打开|
|[xxsocket::native_handle](#native_handle)|获取socket句柄|
|[xxsocket::release_handle](#release_handle)|释放socket句柄控制权|
|[xxsocket::set_nonblocking](#set_nonblocking)|将socket设置为非阻塞模式|
|[xxsocket::test_nonblocking](#test_nonblocking)|检测socket是否为非阻塞模式|
|[xxsocket::bind](#bind)|绑定指定本地指定网卡地址|
|[xxsocket::bind_any](#bind_any)|绑定本地任意地址|
|[xxsocket::listen](#listen)|开始TCP监听|
|[xxsocket::accept](#accept)|接受一个TCP客户端连接|
|[xxsocket::accept_n](#accept_n)|非阻塞方式接受TCP连接|
|[xxsocket::connect](#connect)|建立连接|
|[xxsocket::connect_n](#connect_n)|非阻塞方式建立连接|
|[xxsocket::disconnect](#disconnect)|断开UDP和远程地址的绑定|
|[xxsocket::send](#send)|发送数据|
|[xxsocket::send_n](#send_n)|非阻塞方式发送数据|
|[xxsocket::recv](#recv)|接受数据|
|[xxsocket::recv_n](#recv_n)|非阻塞方式接受数据|
|[xxsocket::sendto](#sendto)|发送DGRAM数据到指定地址|
|[xxsocket::recvfrom](#recvfrom)|接受DGRAM数据|
|[xxsocket::handle_write_ready](#handle_write_ready)|等待socket可写|
|[xxsocket::handle_read_ready](#handle_read_ready)|等待socket可读|
|[xxsocket::local_endpoint](#local_endpoint)|获取socket本地地址|
|[xxsocket::peer_endpoint](#peer_endpoint)|获取socket远端地址|
|[xxsocket::set_keepalive](#set_keepalive)|设置tcp keepalive|
|[xxsocket::reuse_address](#reuse_address)|设置socket是否重用地址|
|[xxsocket::exclusive_address](#exclusive_address)|设置socket是否阻止地址重用|
|[xxsocket::select](#select)|监听socket事件|
|[xxsocket::shutdown](#shutdown)|停止socket收发|
|[xxsocket::close](#close)|关闭socket|
|[xxsocket::tcp_rtt](#tcp_rtt)|获取tcp rtt.|
|[xxsocket::get_last_errno](#get_last_errno)|获取最近socket错误码|
|[xxsocket::set_last_errno](#set_last_errno)|设置最近socket错误码|
|[xxsocket::strerror](#strerror)|将socket错误码转换为字符串|
|[xxsocket::strerror_r](#strerror_r)|将socket错误码转换为字符串，线程安全|
|[xxsocket::gai_strerror](#gai_strerror)|将getaddrinfo返回值转换为字符串|
|[xxsocket::resolve](#resolve)|解析域名|
|[xxsocket::resolve_v4](#resolve_v4)|解析域名包含的ipv4地址|
|[xxsocket::resolve_v6](#resolve_v6)|解析域名包含的ipv6地址|
|[xxsocket::resolve_v4to6](#resolve_v4to6)|解析域名包含的ipv4地址并转换为ipv6的V4MAPPED格式|
|[xxsocket::resolve_tov6](#resolve_tov6)|解析域名包含的所有地址，ipv4地址会转换为ipv6的V4MAPPED格式|
|[xxsocket::getipsv](#getipsv)|获取本机支持的ip协议栈版本标志位|
|[xxsocket::traverse_local_address](#traverse_local_address)|枚举本机地址|


## <a name="xxsocket"></a> xxsocket::xxsocket

构造 `xxsocket` 对象。

```cpp
xxsocket::xxsocket();
xxsocket::xxsocket(socket_native_type handle);
xxsocket::xxsocket(xxsocket&& right);
xxsocket::xxsocket(int af, int type, int protocol);
```

### 参数

*handle*<br/>
通过已有socket句柄构造 `xxsocket` 对象。

*right*<br/>
move构造函数右值引用。

*af*<br/>
ip协议地址类型。

*protocol*<br/>
协议类型，对于TCP/UDP直接传 `0` 就可以。

## <a name="xpconnect"></a> xxsocket::xpconnect

和远程服务器建立TCP连接。

```cpp
int xxsocket::xpconnect(const char* hostname, u_short port, u_short local_port = 0);
```

### 参数

*hostname*<br/>
要连接服务器主机名，可以是 `IP地址` 或 `域名`。

*port*<br/>
要连接服务器的端口。

*local_port*<br/>
本地通信端口号，默认值 `0` 表示随机分配。

### 注意

会自动检测本机支持的ip协议栈版本。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="xpconnect_n"></a> xxsocket::xpconnect_n

和远程服务器建立TCP连接。

```cpp
int xxsocket::xpconnect_n(const char* hostname, u_short port, const std::chrono::microseconds& wtimeout, u_short local_port = 0);
```

### 参数

*hostname*<br/>
要连接服务器主机名，可以是 `IP地址` 或 `域名`。

*port*<br/>
要连接服务器的端口。

*local_port*<br/>
本地通信端口号，默认值 `0` 表示随机分配。

*wtimeout*<br/>
建立连接超时时间。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

会自动检测本机支持的ip协议栈。


## <a name="pconnect"></a> xxsocket::pconnect

和远程服务器建立TCP连接。

```cpp
int xxsocket::pconnect(const char* hostname, u_short port, u_short local_port = 0);
int xxsocket::pconnect(const endpoint& ep, u_short local_port = 0);
```

### 参数

*hostname*<br/>
要连接服务器主机名，可以是 `IP地址` 或 `域名`。

*ep*<br/>
要连接服务器的地址。

*port*<br/>
要连接服务器的端口。

*local_port*<br/>
本地通信端口号，默认值 `0` 表示随机分配。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。


### 注意

不会检测本机支持的ip协议栈。


## <a name="pconnect_n"></a> xxsocket::pconnect_n

和远程服务器建立TCP连接。

```cpp
int pconnect_n(const char* hostname, u_short port, const std::chrono::microseconds& wtimeout, u_short local_port = 0);
int pconnect_n(const char* hostname, u_short port, u_short local_port = 0);
int pconnect_n(const endpoint& ep, const std::chrono::microseconds& wtimeout, u_short local_port = 0);
int pconnect_n(const endpoint& ep, u_short local_port = 0);
```

### 参数

*hostname*<br/>
要连接服务器主机名，可以是 `IP地址` 或 `域名`。

*ep*<br/>
要连接服务器的地址。

*port*<br/>
要连接服务器的端口。

*local_port*<br/>
本地通信端口号，默认值 `0` 表示随机分配。

*wtimeout*<br/>
建立连接超时时间。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。


## <a name="pserve"></a> xxsocket::pserve

开启本地TCP服务监听。

```cpp
int pserve(const char* addr, u_short port);
int pserve(const endpoint& ep);
```

### 参数

*addr*<br/>
本机指定网卡 `IP地址`。

*ep*<br/>
本机地址。

*port*<br/>
TCP监听端口。


### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 示例

```cpp
// xxsocket-serve.cpp
#include <signal.h>
#include <vector>
#include "yasio/xxsocket.hpp"
using namespace yasio;

xxsocket g_server;
static bool g_stopped = false;
void process_exit(int sig)
{
  if (sig == SIGINT)
  {
    g_stopped = true;
    g_server.close();
  }
  printf("exit");
}
int main()
{
  signal(SIGINT, process_exit);

  if (g_server.pserve("0.0.0.0", 1219) != 0)
    return -1;
  const char reply_msg[] = "hi, I'm server\n";
  do
  {
    xxsocket cs = g_server.accept();
    if (cs.is_open())
    {
      cs.send(reply_msg, sizeof(reply_msg) - 1);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  } while (!g_stopped);

  return 0;
}
```


## <a name="swap"></a> xxsocket::swap

交换底层socket句柄。

```cpp
xxsocket& swap(xxsocket& who);
```

### 参数

*who*<br/>
交换对象。

### 返回值

`xxsocket` 左值对象的引用。


## <a name="open"></a> xxsocket::open

打开一个socket。

```cpp
bool open(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
```

### 参数

*af*<br/>
地址类型，例如 `AF_INET` (ipv4)，`AF_INET6` (ipv6)。

*type*<br/>
socket类型， `SOCK_STREAM` (TCP), `SOCK_DGRAM` (UDP)。

*protocol*<br/>
协议，对于TCP/UDP，传 `0` 即可。


### 返回值

`true`: 成功， `false`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="reopen"></a> xxsocket::reopen

打开一个socket。

```cpp
bool reopen(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
```

### 参数

*af*<br/>
地址类型，例如 `AF_INET` (ipv4)，`AF_INET6` (ipv6)。

*type*<br/>
socket类型， `SOCK_STREAM` (TCP), `SOCK_DGRAM` (UDP)。

*protocol*<br/>
协议，对于TCP/UDP，传 `0` 即可。


### 返回值

`true`: 成功， `false`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

如果socket已打开，此函数会先关闭，再重新打开。


## <a name="is_open"></a> xxsocket::is_open

判断socket是否已打开。

```cpp
bool is_open() const;
```

### 返回值

`true`: 已打开， `false`: 未打开。


## <a name="native_handle"></a> xxsocket::native_handle

获取socket文件描述符。

```cpp
socket_native_type native_handle() const;
```

### 返回值

socket文件描述符，`yasio::inet::invalid_socket` 表示无效socket。

## <a name="release_handle"></a> xxsocket::release_handle

释放底层socket描述符控制权。

```cpp
socket_native_type release_handle() const;
```

### 返回值

释放前的socket文件描述符

## <a name="set_nonblocking"></a> xxsocket::set_nonblocking

设置socket的非阻塞模式。

```cpp
int set_nonblocking(bool nonblocking) const;
```

### 参数

*nonblocking*<br/>
`true`: 非阻塞模式，`false`: 阻塞模式。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="test_nonblocking"></a> xxsocket::test_nonblocking

检测socket是否为非阻塞模式。

```cpp
int test_nonblocking() const;
```

### 返回值

`1`: 非阻塞模式， `0`: 阻塞模式。

### 注意

对于winsock2，未连接的 `SOCK_STREAM` 类型socket会返回 `-1`。

## <a name="bind"></a> xxsocket::bind

绑定socket本机地址。

```cpp
int bind(const char* addr, unsigned short port) const;
int bind(const endpoint& ep) const;
```

### 参数

*addr*<br/>
本机指定网卡ip地址。

*port*<br/>
要绑定的端口。

*ep*<br/>
要绑定的本机地址。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="bind_any"></a> xxsocket::bind_any

绑定socket本机任意地址。

```cpp
int bind_any(bool ipv6) const;
```

### 参数

*ipv6*<br/>
是否绑定本机任意IPv6地址

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。


## <a name="listen"></a> xxsocket::listen

开始监听来自TCP客户端的握手请求。

```cpp
int listen(int backlog = SOMAXCONN) const;
```

### 参数

*backlog*<br/>
最大监听数。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="accept"></a> xxsocket::accept

接受一个客户端连接。

```cpp
xxsocket accept() const;
```

### 返回值

和客户端通信的 `xxsocket` 对象。

## <a name="accept_n"></a> xxsocket::accept_n

非阻塞方式接受一个客户端连接。

```cpp
int accept_n(socket_native_type& new_sock) const;
```

### 参数

*new_sock*<br/>
输出参数，和客户端通信的底层socket句柄引用

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

如果此函数返回`0`，*new_sock* 会被设置为非阻塞模式。

调用此函数之前，请手动调用 `xxsocket::set_nonblocking` 将socket设置为非阻塞模式。


## <a name="connect"></a> xxsocket::connect

建立连接。

```cpp
int connect(const char* addr, u_short port);
int connect(const endpoint& ep);
```

### 参数

*addr*<br/>
远程主机ip地址。

*port*<br/>
远程主机端口。

*ep*<br/>
远程主机地址。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

TCP: 发起TCP三次握手  

UDP: 建立4元组绑定

## <a name="connect_n"></a> xxsocket::connect_n

建立连接。

```cpp
int connect_n(const char* addr, u_short port, const std::chrono::microseconds& wtimeout);
int connect_n(const endpoint& ep, const std::chrono::microseconds& wtimeout);
int connect_n(const endpoint& ep);
```

### 参数

*addr*<br/>
远程主机ip地址。

*port*<br/>
远程主机端口。

*ep*<br/>
远程主机地址。

*wtimeout*<br/>
建立连接的超时时间。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

TCP: 发起TCP三次握手  

UDP: 建立4元组绑定

## <a name="disconnect"></a> xxsocket::disconnect

解除socket和远程主机的4元组绑定。

```cpp
int disconnect() const;
```

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## 注意
仅支持UDP

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

只用于 `SOCK_DGRAM`(UDP) 类型socket。

## <a name="send"></a> xxsocket::send

向远端发送指定长度数据。

```cpp
int send(const void* buf, int len, int flags = 0);
```

### 参数

*buf*<br/>
要发送数据的起始字节地址。

*len*<br/>
要发送数据的长度。

*flags*<br/>
发送数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="send_n"></a> xxsocket::send_n

在超时时间内尽力向远端发送指定长度数据。

```cpp
int send_n(const void* buf, int len, const std::chrono::microseconds& wtimeout, int flags = 0);
```

### 参数

*buf*<br/>
要发送数据的起始字节地址。

*len*<br/>
要发送数据的长度。

*wtimeout*<br/>
发送超时时间。

*flags*<br/>
发送数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="recv"></a> xxsocket::recv

从内核去除远程主机发送过来的数据。

```cpp
int recv(void* buf, int len, int flags = 0) const;
```

### 参数

*buf*<br/>
接收数据缓冲区。

*len*<br/>
接收数据缓冲区长度。

*flags*<br/>
接收数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

此函数是否立即返回，取决于socket本身是否是 `非阻塞模式`。

## <a name="recv_n"></a> xxsocket::recv_n

在超时时间内尽力从内核取出指定长度数据。

```cpp
int recv_n(void* buf, int len, const std::chrono::microseconds& wtimeout, int flags = 0) const;
```

### 参数

*buf*<br/>
接收数据缓冲区。

*len*<br/>
接收数据缓冲区长度。

*wtimeout*<br/>
接收超时时间。

*flags*<br/>
接收数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。


## <a name="sendto"></a> xxsocket::sendto

向远程主机发送 `DGRAM` （UDP）数据。

```cpp
int sendto(const void* buf, int len, const endpoint& to, int flags = 0) const;
```

### 参数

*buf*<br/>
待发送数据缓冲区。

*len*<br/>
待发送数据缓冲区长度。

*to*<br/>
发送目标地址。

*flags*<br/>
发送数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="recvfrom"></a> xxsocket::recvfrom

从内核去除远程主机发送过来的数据。

```cpp
int recvfrom(void* buf, int len, endpoint& peer, int flags = 0) const;
```

### 参数

*buf*<br/>
接收数据缓冲区。

*len*<br/>
接收数据缓冲区长度。

*peer*<br/>
接收数据来源，输出参数。

*flags*<br/>
接收数据底层标记。

### 返回值

`==len`: 成功， `< len`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

此函数是否立即返回，取决于socket本身是否是 `非阻塞模式`。


## <a name="handle_write_ready"></a> xxsocket::handle_write_ready

等待socket可写。

```cpp
int handle_write_ready(const std::chrono::microseconds& wtimeout) const;
```

### 参数

*wtimeout*<br/>
等待超时时间。


### 返回值

`0`: 超时， `1`: 成功， `< 0`: 失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

通常当内核发送缓冲区没满的情况下，此函数会立即返回。

## <a name="handle_read_ready"></a> xxsocket::handle_read_ready

等待socket可读。

```cpp
int handle_read_ready(const std::chrono::microseconds& wtimeout) const;
```

### 参数

*wtimeout*<br/>
等待超时时间。


### 返回值

`0`: 超时， `1`: 成功， `< 0`: 失败，通过 `xxsocket::get_last_errno` 获取错误码。


## <a name="local_endpoint"></a> xxsocket::local_endpoint

获取4元组通信的本地地址。

```cpp
endpoint local_endpoint() const;
```

### 返回值

返回本地地址。

### 注意

如果没有调用过 `xxsocket::connect` 或者TCP连接3次握手未完成，那么返回的地址是 `0.0.0.0`

## <a name="peer_endpoint"></a> xxsocket::peer_endpoint

获取4元组通信的对端地址。

```cpp
endpoint peer_endpoint() const;
```

### 返回值

返回本地地址。

### 注意

如果没有调用过 `xxsocket::connect` 或者TCP连接3次握手未完成，那么返回的地址是 `0.0.0.0`

## <a name="set_keepalive"></a> xxsocket::set_keepalive

设置TCP底层协议的心跳参数。

```cpp
int set_keepalive(int flag = 1, int idle = 7200, int interval = 75, int probes = 10);
```

### 参数

*flag*<br/>
`1`: 开启底层协议心跳，`0`: 关闭。

*idle*<br/>
当应用层没有任何消息交互后，启动底层协议心跳探测的最大超时时间，单位（秒）。

*interval*<br/>
当没有收到心跳回应时，重复发送心跳探测报时间间隔，单位（秒）。

*probes*<br/>
当没有收到心跳回应时，最大探测次数，超过探测次数后，会触发应用层连接断开。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 示例

```cpp
// xxsocket-keepalive.cpp
#include "yasio/xxsocket.hpp"
using namespace yasio;
using namespace inet;

int main(){
    xxsocket client;
    if(0 == client.pconnect("192.168.1.19", 80)) {
        client.set_keepalive(1, 5, 10, 2);
    }
    return 0;
}

```

## <a name="reuse_address"></a> xxsocket::reuse_address

设置socket是否允许重用地址。

```cpp
void reuse_address(bool reuse);
```

### 参数

*reuse*<br/>
是否重用。

### 注意

此函数一般用于服务器或者组播监听端口。


## <a name="exclusive_address"></a> xxsocket::exclusive_address

是否明确不允许地址重用，以保护通信双方安全。

```cpp
void exclusive_address(bool exclusive);
```

### 参数

*exclusive*<br/>
`true`: 不允许，`false`: 允许

### 注意

[点击](https://docs.microsoft.com/en-us/windows/win32/winsock/using-so-reuseaddr-and-so-exclusiveaddruse) 查看 `winsock` 安全报告。

## <a name="set_optval"></a> xxsocket::set_optval

设置socket选项。

```cpp
template <typename _Ty> int set_optval(int level, int optname, const _Ty& optval);
```

### 参数

*level*<br/>
socket选项级别。

*optname*<br/>
选项类型。

*optval*<br/>
选项值。

### 返回值

`0`: 成功， `< 0`失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

此函数同bsd socket `setsockopt`功能相同，只是使用模板封装，更方便使用。


## <a name="get_optval"></a> xxsocket::get_optval

设置socket选项。

```cpp
template <typename _Ty> _Ty get_optval(int level, int optname) const
```

### 参数

*level*<br/>
socket选项级别。

*optname*<br/>
选项类型。

### 返回值

返回选项值。

### 注意

此函数同bsd socket `getsockopt`功能相同，只是使用模板封装，更方便使用。


## <a name="select"></a> xxsocket::select

监听socket内核事件。

```cpp
int select(fd_set* readfds, fd_set* writefds, fd_set* exceptfds, const std::chrono::microseconds& wtimeout)
```

### 参数

*readfds*<br/>
可读事件描述符数组。

*writefds*<br/>
可写事件描述符数组。

*exceptfds*<br/>
异常事件描述符数组。

*wtimeout*<br/>
等待事件超时事件。

### 返回值

`0`: 超时，`> 0`: 成功， `< 0`: 失败，通过 `xxsocket::get_last_errno` 获取错误码。


## <a name="shutdown"></a> xxsocket::shutdown

关闭TCP传输通道。

```cpp
int shutdown(int how = SD_BOTH) const;
```

### 参数

*how*<br/>
关闭通道类型，可传以下枚举值<br/>

- `SD_SEND`: 发送通道
- `SD_RECEIVE`: 接受通道
- `SD_BOTH`: 全部关闭

### 返回值

`0`: 成功， `< 0`: 失败，通过 `xxsocket::get_last_errno` 获取错误码。

## <a name="close"></a> xxsocket::close

关闭socket，释放系统资源。

```cpp
void close(int shut_how = SD_BOTH);
```

### 参数

*shut_how*<br/>
关闭通道类型，可以传以下枚举值<br/>

* `SD_SEND`: 发送通道
* `SD_RECEIVE`: 接受通道
* `SD_BOTH`: 全部关闭
* `SD_NONE`: 全部关闭

### 返回值

`0`: 成功， `< 0`: 失败，通过 `xxsocket::get_last_errno` 获取错误码。

### 注意

如果 *shut_how* != `SD_NONE`，此函数会先调用 `shutdown`，再调用底层 `close`。

## <a name="tcp_rtt"></a> xxsocket::tcp_rtt

获取TCP连接的RTT。

```cpp
uint32_t tcp_rtt() const;
```

### 返回值

返回TCP的RTT时间，单位: `微秒`。

## <a name="get_last_errno"></a> xxsocket::get_last_errno

获取最近一次socket操作错误码。

```cpp
 static int get_last_errno();
```

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。

### 注意

此函数是线程安全的。

## <a name="set_last_errno"></a> xxsocket::set_last_errno

设置socket操作错误码。

```cpp
static void set_last_errno(int error);
```

### 参数

*error*<br/>
错误码。

### 注意

此函数是线程安全的。

## <a name="not_send_error"></a> xxsocket::not_send_error

判断是否是发送时socket出现无法继续的错误。

```cpp
static bool not_send_error(int error);
```

### 参数

*error*<br/>
错误码。

### 返回值

`true`: socket正常，`false`: socket状态已经发生错误，应当关闭socket终止通讯。

### 注意

仅当发送操作返回值 `< 0`时，调用此函数。

## <a name="not_recv_error"></a> xxsocket::not_recv_error

判断是否是接收时socket出现无法继续的错误。

```cpp
static bool not_recv_error(int error);
```

### 参数

*error*<br/>
错误码。

### 返回值

`true`: socket正常，`false`: socket状态已经发生错误，应当关闭socket终止通讯。

### 注意

仅当接收操作返回值 `< 0`时，调用此函数。


## <a name="strerror"></a> xxsocket::strerror

将错误码转换为字符串。

```cpp
static const char* strerror(int error);
```

### 参数

*error*<br/>
错误码。

### 返回值

错误信息的字符串。

## <a name="strerror_r"></a> xxsocket::strerror_r

将错误码转换为字符串，功能和`xxsocket::strerror`一样，但此函数线程安全的。

```cpp
static const char* strerror_r(int error, char *buf, size_t buflen);
```

### 参数

*error*<br/>
错误码。

*buf*<br/>
接受错误信息字符串的缓冲区

*buflen*<br/>
接受错误信息字符串的缓冲区大小


### 返回值

错误信息的字符串。

## <a name="gai_strerror"></a> xxsocket::gai_strerror

将`getaddrinfo`错误码转换为字符串。

```cpp
static const char* gai_strerror(int error);
```

### 参数

*error*<br/>
错误码。

### 返回值

错误信息的字符串。

## <a name="resolve"></a> xxsocket::resolve

解析域名包含的所有地址。

```cpp
int resolve(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port = 0, int socktype = SOCK_STREAM);
```

### 参数

*endpoints*<br/>
输出参数。

*hostname*<br/>
域名。

*port*<br/>
端口。

*socktype*<br/>
socket类型。

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。


## <a name="resolve_v4"></a> xxsocket::resolve_v4

解析域名包含的IPv4地址。

```cpp
int resolve_v4(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port = 0, int socktype = SOCK_STREAM);
```

### 参数

*endpoints*<br/>
输出参数。

*hostname*<br/>
域名。

*port*<br/>
端口。

*socktype*<br/>
socket类型。

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。

## <a name="resolve_v6"></a> xxsocket::resolve_v6

解析域名包含的IPv6地址。

```cpp
int resolve_v6(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port = 0, int socktype = SOCK_STREAM);
```

### 参数

*endpoints*<br/>
输出参数。

*hostname*<br/>
域名。

*port*<br/>
端口。

*socktype*<br/>
socket类型。

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。

## <a name="resolve_v4to6"></a> xxsocket::resolve_v4to6

仅解析域名包含的IPv4地址并转换为IPv6 V4MAPPED格式。

```cpp
int resolve_v4to6(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port = 0, int socktype = SOCK_STREAM);
```

### 参数

*endpoints*<br/>
输出参数。

*hostname*<br/>
域名。

*port*<br/>
端口。

*socktype*<br/>
socket类型。

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。



## <a name="resolve_tov6"></a> xxsocket::resolve_tov6

解析域名包含的所有地址，IPv4地址会转换为IPv6 V4MAPPED格式。

```cpp
int resolve_tov6(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port = 0, int socktype = SOCK_STREAM);
```

### 参数

*endpoints*<br/>
输出参数。

*hostname*<br/>
域名。

*port*<br/>
端口。

*socktype*<br/>
socket类型。

### 返回值

`0`: 无错误， `> 0` 通过 `xxsocket::strerror` 转换为详细错误信息。


## <a name="getipsv"></a> xxsocket::getipsv

获取本机支持的IP协议栈标志位。

```cpp
static int getipsv();
```


### 返回值

- `ipsv_ipv4`: 本机只支持IPv4协议。
- `ipsv_ipv6`: 本机只支持IPv6协议。
- `ipsv_dual_stack`: 本机支持IPv4和IPv6双栈协议。

### 注意

当返回值支持双栈协议是，用户应当始终优先使用IPv4通信，<br/>
例如`智能手机设备`在同时开启`wifi`和`蜂窝网络`时，将会优先选择wifi，<br/>
而wifi通常是IPv4，详见: https://github.com/halx99/yasio/issues/130

### 示例

```cpp
// xxsocket-ipsv.cpp
#include <vector>
#include "yasio/xxsocket.hpp"
using namespace yasio;
int main(){
    const char* host = "github.com";
    std::vector<ip::endpoint> eps;
    int flags = xxsocket::get_ipsv();
    if(flags & ipsv_ipv4) {
        xxsocket::resolve_v4(eps, host, 80);
    }
    else if(flags & ipsv_ipv6) {
        xxsocket::resolve_tov6(eps, host, 80);
    }
    else {
        std::cerr << "Local network not available!\n";
    }
    return 0;
}
```

## <a name="traverse_local_address"></a> xxsocket::traverse_local_address

枚举本机地址。

```cpp
static void traverse_local_address(std::function<bool(const ip::endpoint&)> handler);
```

### 参数

*handler*<br/>
枚举地址回调。

### 示例

```cpp
// xxsocket-traverse.cpp
#include <vector>
#include "yasio/xxsocket.hpp"
using namespace yasio;
int main(){
    int flags = 0;
    xxsocket::traverse_local_address([&](const ip::endpoint& ep) -> bool {
        switch (ep.af())
        {
          case AF_INET:
            flags |= ipsv_ipv4;
            break;
          case AF_INET6:
            flags |= ipsv_ipv6;
            break;
        }
        return (flags == ipsv_dual_stack);
    });
    YASIO_LOG("Supported ip stack flags=%d", flags);
    return flags;
}
```

## 请参阅

[io_service Class](./io_service-class.md)
