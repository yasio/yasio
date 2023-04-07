---
title: "yasio::inet::io_event Class"
date: "1/5/2021"
f1_keywords: ["io_event", "yasio/io_event", ]
helpviewer_keywords: []
---

# io_event Class

网络事件由 io_service 线程产生。


## 语法

```cpp
namespace yasio { inline namespace inet { class io_event; } }
```

## 成员

### 公共方法

|Name|Description|
|----------|-----------------|
|[io_event::kind](#kind)|获取事件类型|
|[io_event::status](#status)|获取事件状态|
|[io_event::passive](#passive)|检查是否是被动事件|
|[io_event::packet](#packet)|获取事件消息包|
|[io_event::packet_view](#packet_view)|获取事件消息包view|
|[io_event::timestamp](#timestamp)|获取事件时间戳|
|[io_event::transport](#transport)|获取事件传输会话|
|[io_event::transport_id](#transport_id)|获取事件传输会话ID|
|[io_event::transport_ud](#transport_ud)|设置或获取事件传输会话用户数据|


## <a name="kind"></a> io_event::kind

获取事件类型。

```cpp
int kind() const;
```

### 返回值

事件类型，可以是以下值:

* `YEK_ON_PACKET`: 消息事件
* `YEK_ON_OPEN`: 打开事件，对于客户端信道，代表连接响应
* `YEK_ON_CLOSE`: 关闭事件，对于客户端信道，代表连接丢失

## <a name="status"></a> io_event::status

获取事件状态。

```cpp
int status() const;
```

### 返回值

- 0: 正常
- 非0: 出错, 用户只需要简单打印即可。

## <a name="passive"></a> io_event::passive

检查是否是被动事件

```cpp
int passive() const
```

### 返回值

- 0: 非被动事件，包括客户端信道的连接打开、关闭事件和传输会话的消息事件。
- 1: open或close服务器信道时产生，仅当打开编译配置`YASIO_ENABLE_PASSIVE_EVENT`才会产生。  
且为保持兼容，此行为默认是关闭的。

## <a name="packet"></a> io_event::packet

获取事件携带的消息包

```cpp
yasio::sbyte_buffer& packet()
```

## 返回值

消息数据的view。

## <a name="packet_view"></a> io_event::packet_view

获取事件携带的消息数据view，仅当io_service设置`YOPT_S_FORWARD_PACKET`时有效。此数据在事件回调结束后会失效，业务应及时保存。

```cpp
packet_view_t packet_view()
```

## 返回值

消息包的引用, 用户可以使用`std::move`无GC方式从事件取走消息包。

## <a name="timestamp"></a> io_event::timestamp

获取事件产生的微秒级时间戳。

```cpp
highp_time_t timestamp() const;
```

### 返回值

和系统时间无关的微秒级时间戳。

## <a name="transport"></a> io_event::transport

获取事件的传输会话句柄。

```cpp
transport_handle_t transport() const;
```

### 返回值

返回句柄，当收到断开事件时，传输会话句柄已失效，仅可用作地址值比较。

## <a name="transport_id"></a> io_event::transport_id

获取事件的传输会话ID。

```cpp
unsigned int transport_id() const;
```

### 返回值

32位无符号整数范围内的唯一ID。

## <a name="transport_ud"></a> io_event::transport_ud

设置或获取传输会话用户数据。

```cpp
template<typename _Uty>
_Uty io_event::transport_ud();

template<typename _Uty>
void io_event::transport_ud(_Uty uservalue);
```

### 注意

用户需要自己管理 userdata 的内存, 例如:  

* 收到连接建立成功事件时存储userdata
* 收到连接丢失事件时清理userdata

## 请参阅

[io_service Class](./io_service-class.md)

[io_channel Class](./io_channel-class.md)
