---
title: "yasio::inet::io_channel Class"
date: "1/5/2021"
f1_keywords: ["io_channel", "yasio/io_channel", ]
helpviewer_keywords: []
---

# io_channel Class

负责管理 TCP/SSL/UDP/KCP 连接和传输会话。


## 语法

```cpp
namespace yasio { inline namespace inet { class io_channel; } }
```

## 成员

### 公共方法

|Name|Description|
|----------|-----------------|
|[io_channel::get_service](#get_service)|获取管理信道的io_service|
|[io_channel::index](#index)|获取信道索引|
|[io_channel::remote_port](#remote_port)|获取信道远程端口|
|[io_channel::bytes_transferred](#bytes_transferred)|获取传输字节数|
|[io_channel::connect_id](#bytes_transferred)|获取连接ID|

## 注意

当io_service对象构造后，最大信道数量不可改变， <br/>
信道句柄可通过 `io_service::channel_at` 获取。


## <a name="get_service"></a> io_channel::get_service

获取管理信道的io_service对象。

```cpp
io_service& get_service()
```

## <a name="index"></a> io_channel::index

获取信道索引。

```cpp
int index() const
```

## <a name="remote_port"></a> io_channel::remote_port

获取信道远程端口.

```cpp
u_short remote_port() const;
```

### 返回值

返回信道远程端口号

- 对于客户端信道表示通信的远端端口 
- 对于服务端信道表示监听端口

## <a name="bytes_transferred"></a> io_channel::bytes_transferred

获取客户端接收总字节数

```cpp
long long bytes_transferred() const;
```

### 返回值

从连接建立开始到当前，接收总字节数，用于统计客户端接收流量

## <a name="connect_id"></a> io_channel::connect_id

获取客户端信道递增的实时连接id

```cpp
unsigned int connect_id() const;
```

### 返回值

客户端信道当前连接id


## 请参阅

[io_service Class](./io_service-class.md)

[io_event Class](./io_event-class.md)
