---
title: "ibstream Class"
date: "1/16/2021"
f1_keywords: ["ibstream", "yasio/ibstream", ]
helpviewer_keywords: []
---
# ibstream_view Class

提供二进制反序列化功能。

!!! attention "注意"

    在反序列化过程中，当剩余数据不足时会抛出 `std::out_of_range` 异常。

## 语法

```cpp
namespace yasio { 
// 反序列化过程，会自动转换字节序，适用于网络传输
using ibstream_view = basic_ibstream_view<endian::network_convert_tag>; 

// 反序列化过程，无字节序转换，性能更快
using fast_ibstream_view = basic_ibstream_view<endian::host_convert_tag>;
}
```

## 成员

### 公共构造

|Name|Description|
|----------|-----------------|
|[ibstream_view::ibstream_view](#ibstream_view)|构造1个 `ibstream_view` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[ibstream_view::reset](#reset)|重置待反序列化数据|
|[ibstream_view::read](#read)|函数模板，读取数值|
|[ibstream_view:read_ix](#read_ix)|函数模板，读取(**7bit Encoded Int/Int64**)整数值|
|[ibstream_view:read_v](#read_v)|读取带长度域(**7bit Encoded Int/Int64**)的二进制数据|
|[ibstream_view:read_byte](#read_byte)|读取1个字节|
|[ibstream_view:read_bytes](#read_bytes)|读取指定长度二进制数据|
|[ibstream_view::empty](#empty)|检查流是否为空|
|[ibstream_view::data](#data)|获取流数据指针|
|[ibstream_view::length](#length)|获取流大小|
|[ibstream_view::advance](#advance)|向前移动流的读取游标|
|[ibstream_view::seek](#seek)|移动流的读取游标|

## 注意

`ibstream_view` 借鉴C++17标准的 `std::string_view`, 意味着在初始化和反序列化过程中不会产生任何GC。

## 要求

**头文件:** ibstream.hpp

## <a name="ibstream_view"></a> ibstream_view::ibstream_view

构造一个 `ibstream_view` 对象。

```cpp
ibstream_view();

ibstream_view(const void* data, size_t size);

ibstream_view(const obstream* obs);
```

### 参数

*data*<br/>
待反序列化二进制数据首地址。

*size*<br/>
待反序列化二进制数据大小。

*obs*<br/>
已序列化的流。


## <a name="reset"></a> ibstream_view::reset

重置 `ibstream_view` 缓冲视图。

```cpp
void ibstream_view::reset(const void* data, size_t size);
```

### 参数

*data*<br/>
待反序列化二进制数据首地址。

*size*<br/>
待反序列化二进制数据大小。

## <a name="read"></a> ibstream_view::read

从流中读取数值。

```cpp
template<typename _Nty>
_Nty ibstream_view::read();
```

### 返回值

返回读到的值

### 注意

*_Nty* 实际类型可以是任意1~8字节整数类型或浮点类型。<br />

## <a name="read_ix"></a> ibstream_view::read_ix

读取7Bit Encoded Int压缩编码的整数值。

```cpp
template<typename _Intty>
_Intty ibstream_view::read_ix();
```

### 返回值

32/64位整数值。

### 注意

*_Intty* 的必须是以下类型

- int32_t
- int64_t

本函数兼容于 **Microsoft dotnet** 如下函数

- [BinaryReader.Read7BitEncodedInt()](https://docs.microsoft.com/en-us/dotnet/api/system.io.binaryreader.read7bitencodedint?view=net-5.0#System_IO_BinaryReader_Read7BitEncodedInt)
- [BinaryReader.Read7BitEncodedInt64()](https://docs.microsoft.com/en-us/dotnet/api/system.io.binaryreader.read7bitencodedint64?view=net-5.0#System_IO_BinaryReader_Read7BitEncodedInt64)

## <a name="read_v"></a> ibstream_view::read_v

读取变长二进制数据。

```cpp
cxx17::string_view read_v();
```

### 返回值

返回读取到的二进制数据视图，无GC。

### 注意

本函数会先读取7bit Encoded Int压缩编码的长度值，再调用 [read_bytes](#read_bytes) 读取二进制字节数据。


## <a name="read_byte"></a> ibstream_view::read_byte

读取1个字节。

```cpp
uint8_t read_byte();
```

### 返回值

uint8_t值。

### 注意

本函数等价于 [`ibstream_view::read<uint8_t>`](#read)


## <a name="read_bytes"></a> ibstream_view::read_bytes

读取指定长度字节数据，无GC。

```cpp
cxx17::string_view read_bytes();
```

### 返回值

二进制数据的 `cxx17::string_view` 类型视图。

## <a name="empty"></a> ibstream_view::empty

判断流是否为空。

```cpp
bool empty() const;
```

### 返回值

`true` 空; `false` 流中至少包含1个字节。

### 注意

此方法等价于 [length](#length) == 0。

## <a name="data"></a> ibstream_view::data

返回流数据指针。

```cpp
const char* data() const;
```

### 返回值

返回指向流中第一个字节的指针。

## <a name="length"></a> ibstream_view::length

获取流长度。

```cpp
size_t length() const;
```

### 返回值

当前流长度。

## <a name="advance"></a> ibstream_view::advance

向前移动流读取游标。

```cpp
void advance(ptrdiff_t offset);
```

### 参数

*offset*<br/>
要向前移动的偏移量。

### 注意

若*offset*传负数，则反向可移动读取游标。

## <a name="seek"></a> ibstream_view::seek

移动读取游标。

```cpp
ptrdiff_t seek(ptrdiff_t offset, int whence);
```

### 参数

*offset*<br/>
和*whence*相关的偏移量。

*whence*<br/>
含义等同于C标准库枚举值： `SEEK_SET`,`SEEK_CUR`,`SEEK_END`。


### 返回值

返回移动后相对于流首字节偏移。


# ibstream Class

提供二进制数据加载和反序列化功能。

## 语法

```cpp
namespace yasio { 
using ibstream = basic_ibstream<endian::network_convert_tag>; 
// 反序列化过程，无字节序转换，性能更快
using fast_ibstream = basic_ibstream<endian::host_convert_tag>; 
}
```

## <a name="Members"></a> 成员

### <a name="public-constructor"></a> 公共构造函数

|Name|Description|
|----------|-----------------|
|[ibstream::ibstream](#ibstream)|构造1个 `ibstream` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[ibstream::load](#load)|从文件加载流|

### <a name="inheritance-hierarchy"></a> 继承层次结构
[ibstream_view](#ibstream_view)

`ibstream`

## <a name="ibstream"></a> ibstream::ibstream

构造一个 `ibstream` 对象。

```cpp
ibstream(yasio::sbyte_buffer blob);

ibstream(const obstream* obs);
```

### 参数

*blob*<br/>
输入二进制流。

*obs*<br/>
已序列化的 `obstream` 对象。

## <a name="load"></a> ibstream::load

从文件加载.

```cpp
bool load(const char* filename) const;
```

### 返回值

`true` 加载成功，`false` 加载失败。

### 示例

请查看 [obstream::save](obstream-class.md#save)

## 请参阅

[obstream Class](./obstream-class.md)

[io_service Class](./io_service-class.md)
