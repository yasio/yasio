# endpoint Class

封装底层 `bsd socket` 地址，支持IPv4和IPv6，包含IP地址和端口信息，可直接用于底层 socket API: `bind/connect/sendto/recvfrom`。

## 语法

```cpp
namespace yasio { inline namespace inet { inline namespace ip { struct endpoint; } } }
```

## 成员

|Name|Description|
|----------|-----------------|
|[endpoint::endpoint](#endpoint)|构造一个 `endpoint` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[endpoint::operator bool](#operator-bool)|检查是否是一个有效地址|
|[endpoint::operator=](#operator-assign)|赋值运算符重载|
|[endpoint::operator&](#operator-addrof)|取地址运算符重载|
|[endpoint::as_is](#as_is)|从已知地址类型构造|
|[endpoint::as_in](#as_in)|从ip，端口等地址信息构造|
|[endpoint::as_un](#as_un)|构造为unix domain socket地址|
|[endpoint::as_is_raw](#as_is_raw)|从连续内存地址构造|
|[endpoint::zeroset](#zeroset)|清零|
|[endpoint::af](#af)|设置或获取地址类型(族)|
|[endpoint::ip](#ip)|设置或获取字符串形式ip|
|[endpoint::port](#port)|设置或获取端口|
|[endpoint::addr_v4](#addr_v4)|设置或获取IPv4地址|
|[endpoint::is_global](#is_global)|检查是否是全局地址|
|[endpoint::len](#len)|获取地址长度|
|[endpoint::to_string](#to_string)|将地址转换为字符串|
|[endpoint::format_to](#format_to)|格式化地址到字符串|

## <a name="endpoint"></a> endpoint::endpoint

构造 `endpoint` 对象。

```cpp
endpoint::endpoint();
endpoint::endpoint(const endpoint& rhs);
explicit endpoint::endpoint(const addrinfo* ai);
explicit endpoint::endpoint(const sockaddr* sa);
explicit endpoint::endpoint(const char* str_ep);
endpoint::endpoint(const char* ip, unsigned short port);
endpoint::endpoint(uint32_t addrv4, unsigned short port);
endpoint::endpoint(int family, const void* addr, unsigned short port);
```

### 参数

*rhs*<br/>
构造endpoint的右值`endpoint`对象

*ai*<br/>
构造endpoint的addrinfo信息

*sa*<br/>
构造endpoint的sockaddr信息

*str_ep*<br/>
字符串表示的IP和端口信息，格式为 `127.0.0.1:2022` 或 `[fe80::1]:2033`

*ip*<br/>
构造endpoint的IPv4或IPv6地址字符串

*port*<br/>
构造endpoint的端口

*addrv4*<br/>
构造endpoint的4字节无符号整数表示的IPv4地址值

*family*<br/>
构造endpoint的地址类型

*addr*<br/>
构造endpoint地址，地址类型由family决定

## <a name="operator-bool"></a> endpoint::operator bool

检查是否是一个有效地址。

```cpp
explicit operator bool() const;
```

### 返回值

`true`: 地址有效， `false`：地址无效


## <a name="operator-assign"></a> endpoint::operator =

赋值运算符重载。

```cpp
endpoint& operator=(const endpoint& rhs) const;
```

### 返回值

返回对象自身的引用。

### 示例

```cpp
#include "yasio/xxsocket.h"
int main() {
    yasio::endpoint ep("127.0.0.1", 2021);
    yasio::endpoint ep1("127.0.0.1", 2022);
    yasio::endpoint ep3{ep};
    ep3 = ep1;
    return 0;
}
```

## <a name="operator-addrof"></a> endpoint::operator &

取地址运算符重载。

```cpp
sockaddr* operator&();
const sockaddr* operator&() const;
```

### 返回值

返回sockaddr*类型，可直接用户底层bsd socket API。

### 注意

如果需要获取`ip::endpoint*`类型指针，请使用`std::addressof`。

### 示例

```cpp
#include "yasio/xxsocket.h"
int main() {
    yasio::endpoint ep("127.0.0.1", 2021);
    yasio::socket sock(AF_INET, SOCK_STREAM, 0);
    ::connect(sock.native_handle(), &ep, ep.len());
    return 0;
}
```


## <a name="as_is"></a> endpoint::as_is

由已知地址信息构造。

```cpp
endpoint& as_is(const endpoint& rhs);
endpoint& as_is(const addrinfo* info);
endpoint& as_is(const sockaddr* addr);
endpoint& as_is(const char* str_ep);
```

### 参数

*rhs*<br/>
已有`endpoint`对象

*info*<br/>
addrinfo*地址信息

*addr*<br/>
sockaddr*类型地址

*str_ep*<br/>
字符串表示的IP和端口信息，格式为 `127.0.0.1:2022` 或 `[fe80::1]:2033`

### 返回值

返回对象自身的引用。

## <a name="as_in"></a> endpoint::as_in

由地址类型、地址和端口参数构造。

```cpp
endpoint& as_in(int family, const void* addr_in, u_short port);
endpoint& as_in(const char* addr, unsigned short port);
endpoint& as_in(uint32_t addrv4, u_short port);
```

### 参数

*family*<br/>
地址类型

*addr_in*<br/>
in地址，类型由参数`family`决定

*addr*<br/>
字符串表示的IPv4或IPv4地址

*addrv4*<br/>
4字节无符号整数表示的IPv4地址

*port*<br/>
端口号

### 返回值

返回对象自身的引用。


## <a name="as_un"></a> endpoint::as_un

构造为Unix domain socket的endpoint，当编译器宏 `YASIO_ENABLE_UDS` 定义时可用。

```cpp
endpoint& as_un(const char* name);
```

### 参数

*name*<br/>
Unix domain socket本地磁盘路径，不能超过`sizeof(sockaddr::sun_path)`

### 返回值

返回对象自身的引用。


## <a name="as_is_raw"></a> endpoint::as_is_raw

由已知地址信息构造。

```cpp
endpoint& as_is_raw(const void* ai_addr, size_t ai_addrlen);
```

### 参数

*ai_addr*<br/>
地址指针

*ai_addrlen*<br/>
地址长度

### 返回值

返回对象自身的引用。

## <a name="zeroset"></a> endpoint::zeroset

清空地址。

```cpp
void zeroset();
```


## <a name="af"></a> endpoint::af

设置或获取endpoint的地址类型。

```cpp
void af(int v);
int af() const;
```

### 参数

*v*<br/>
地址类型： 取值 `AF_INET`、`AF_INET6`

### 返回值

返回地址类型。

## <a name="ip"></a> endpoint::ip

设置或获取ip字符串。

```cpp
void ip(const char* v);
std::string ip() const;
```

### 参数

*v*<br/>
字符串表示的IPv4或IPv6地址，例如: 127.0.0.1、fe80::1

### 返回值

返回字符串表示的IPv4或IPv6地址。

## <a name="port"></a> endpoint::port

设置或获取端口号。

```cpp
void port(u_short v);
u_short port() const;
```

### 参数

*v*<br/>
端口号

### 返回值

返回端口号。

## <a name="addr_v4"></a> endpoint::addr_v4

设置或获取4字节无符号整数表示的IPv4地址。

```cpp
void addr_v4(uint32_t addr);
uint32_t addr_v4() const;
```

### 参数
*addr*<br/>
`uint32_t` 表示的IPv4地址，会将 `endpoint` 的地址类型修改为 `AF_INET`。

### 返回值

返回4字节无符号整数表示的IPv4地址，如果地址类型不是IPv4则返回`0`


## <a name="len"></a> endpoint::len

设置或获取地址长度。

```cpp
void len(int n);
int len() const;
```

### 参数

*n*<br/>
地址长度

### 返回值

返回地址长度。

## <a name="is_global"></a> endpoint::is_global

判断地址是否为全局地址，非回环地址。

```cpp
bool is_global() const;
```

### 返回值

`true`: 全局地址，`false`: 本机回环地址。


## <a name="to_string"></a> endpoint::to_string

转换为字符串。

```cpp
std::string to_string(int flags = endpoint::fmt_default) const;
```

### 参数

*flags*<br/>
格式化标志位，详见: [endpoint::fmt_xxx](#fmt_xxx)

### 返回值

返回字符串表示的IPv4或IPv6地址，是否包含端口取决于参数`flags`。


## <a name="format_to"></a> endpoint::format_to

赋值运算符重载。

```cpp
size_t format_to(std::string& buf, int flags = 0) const;
size_t format_to(char* buf, size_t buf_len, int flags) const;
```

### 参数

*buf*<br/>
格式化目标缓冲区。

*buf_len*<br/>
格式化目标缓冲区大小，必须确保大于 `endpoint::max_fmt_len`。

*flags*<br/>
格式化标志位，详见: [endpoint::fmt_xxx](#fmt_xxx)

### 返回值

返回字符串表示的IPv4或IPv6地址，是否包含端口取决于参数`flags`。

## <a name="fmt_xxx"></a> endpoint::fmt_xxx

地址格式化标志位枚举值

```cpp
struct endpoint {
    enum
    {
        fmt_no_local   = 1,
        fmt_no_port    = 2,
        fmt_no_port_0  = 4,
        fmt_no_un_path = 8,
        fmt_default    = fmt_no_port_0 | fmt_no_un_path,
    };
}
```

- *fmt_no_local*: 忽略回环地址
- *fmt_no_port*: 忽略端口
- *fmt_no_port_0*: 忽略0端口
- *fmt_no_un_path*: 忽略unix path
- *fmt_default*: 忽略0端口和unix path
