---
title: "yasio::obstream Class"
date: "16/10/2020"
f1_keywords: ["obstream", "yasio/obstream", ]
helpviewer_keywords: []
---
# obstream Class

提供二进制序列化功能。

!!! attention "注意"

    - 自3.35.0起，优化为类模板basic_obstream实现，体现了C++模板强大的代码复用能力。

    - `obstream` 当写入int16~int64和float/double类型时, 会自动将主机字节序转换为网络字节序。

    - `fast_obstream` 不会转换任何字节序。

## 语法

```cpp
namespace yasio { 
using obstream = basic_obstream<endian::network_convert_tag>;
using fast_obstream = basic_obstream<endian::host_convert_tag>;
}
```

## 成员

### 公共构造函数

|Name|Description|
|----------|-----------------|
|[obstream::obstream](#obstream)|构造1个`obstream` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[obstream::write](#write)|函数模板，写入数值|
|[obstream::write_ix](#write_ix)|函数模板，写入(**7bit Encoded Int/Int64**)数值|
|[obstream::write_v](#write_v)|写入带长度域(**7bit Encoded Int**)的二进制数据|
|[obstream::write_byte](#write_byte)|写入1个字节|
|[obstream::write_bytes](#write_bytes)|写入指定长度二进制数据|
|[obstream::empty](#empty)|检查流是否为空|
|[obstream::data](#data)|获取流数据指针|
|[obstream::length](#length)|获取流数据大小|
|[obstream::buffer](#buffer)|获取流内部缓冲区|
|[obstream::clear](#clear)|清理流，以便复用|
|[obstream::shrink_to_fit](#shrink_to_fit)|释放流内部缓冲区多余内存|
|[obstream::save](#save)|保存流二进制数据到文件系统|

## 要求

**头文件:** obstream.hpp

## <a name="obstream"></a> obstream::obstream

构造 `obstream` 对象。

```cpp
obstream(size_t capacity = 128);

obstream(const obstream& rhs);

obstream(obstream&& rhs);
```

## <a name="write"></a> obstream::write

写入数值类型

```cpp
template<typename _Nty>
void obstream::write(_Nty value);
```

### 参数

*value*<br/>
要写入的值

### 注意

*_Nty* 实际类型可以是任意1~8字节整数类型或浮点类型。

## <a name="write_ix"></a> obstream::write_ix

将32/64位整数值以7Bit Encoded Int方式压缩后写入流。

```cpp
template<typename _Intty>
void obstream::write_ix(_Intty value);
```

### 参数

*value*<br/>
要写入的值。

### 注意

*_Intty* 类型只能是如下类型

- int32_t
- int64_t

此函数压缩编码方式兼容微软dotnet如下函数

- [BinaryWriter.Write7BitEncodedInt](https://docs.microsoft.com/en-us/dotnet/api/system.io.binarywriter.write7bitencodedint?view=net-5.0#System_IO_BinaryWriter_Write7BitEncodedInt_System_Int32_)
- [BinaryWriter.Write7BitEncodedInt64](https://docs.microsoft.com/en-us/dotnet/api/system.io.binarywriter.write7bitencodedint64?view=net-5.0#System_IO_BinaryWriter_Write7BitEncodedInt64_System_Int64_)


## <a name="write_v"></a> obstream::write_v

写入二进制数据，包含长度字段(7Bit Encoded Int).

```cpp
void write_v(cxx17::string_view sv);
```

### 参数

*sv*<br/>
要写入的数据。

### 注意

此函数先以7Bit Encoded编码方式写入数据长度, 再调用 [write_bytes](#write_bytes) 写入字节数据.


## <a name="write_byte"></a> obstream::write_byte

写入1个字节。

```cpp
void write_byte(uint8_t value);
```

### 参数

*value*<br/>
要写入的值。

### 注意

此函数功能等价于 [obstream::write<uint8_t>](#write)


## <a name="write_bytes"></a> obstream::write_bytes

写入字节数组。

```cpp
void write_bytes(cxx17::string_view sv);

void write_bytes(const void* data, int length);

void write_bytes(std::streamoff offset, const void* data, int length);
```

### 参数

*sv*<br/>
写入string_view包装的字节数组.

*data*<br/>
要写入字节数组的首地址.

*length*<br/>
要写入字节数组的长度.

*offset*<br/>
要写入字节数组的目标偏移.

### 注意

`offset + length` 的值必须小于 [`obstream::length`](#length)

## <a name="empty"></a> obstream::empty

判断流是否为空。

```cpp
bool empty() const;
```

### 返回值

`true` 空; `false` 非空。

### 注意

此函数等价于 [length](#length) == 0。


## <a name="data"></a> obstream::data

获取数据指针。

```cpp
const char* data() const;

char* data();
```

### 返回值

字节流数据首地址。

## <a name="length"></a> obstream::length

获取流长度。

```cpp
size_t length() const;
```

### 返回值

返回流中包含的总字节数。

## <a name="buffer"></a> obstream::buffer

获取流内部缓冲区。

```cpp
const std::vector<char>& buffer() const;

std::vector<char>& buffer();
```

### 返回值

流内部缓冲区引用，可以使用 `std::move` 取走。

### 示例

```cpp
// obstream_buffer.cpp
// compile with: /EHsc
#include "yasio/obstream.hpp"

int main( )
{
   using namespace yasio;
   using namespace cxx17;
   
   obstream obs;
   obs.write_v("hello world!");
   
   const auto& const_buffer = obs.buffer();

   // after this line, the obs will be empty
   auto move_buffer = std::move(obs.buffer());

   return 0;
}
```

## <a name="clear"></a> obstream::clear

清理流，以便复用。

```cpp
void clear();
```

### 注意

此函数不会释放buffer内存，对于高效地复用序列化器非常有用。

## <a name="shrink_to_fit"></a> obstream::shrink_to_fit

释放流内部缓冲区多余内存。

```cpp
void shrink_to_fit();
```

## <a name="save"></a> obstream::save

将流中的二进制字节数据保存到文件。

```cpp
void save(const char* filename) const;
```

### 示例

```cpp
// obstream_save.cpp
// compile with: /EHsc
#include "yasio/obstream.hpp"
#include "yasio/ibstream.hpp"

int main( )
{
   using namespace yasio;
   using namespace cxx17;
   
   obstream obs;
   obs.write_v("hello world!");
   obs.save("obstream_save.bin");

   ibstream ibs;
   if(ibs.load("obstream_save.bin")) {
       // output should be: hello world!
       try {
           std::count << ibs.read_v() << "\n";
       }
       catch(const std::exception& ex) {
           std::count << "read_v fail: " <<
               << ex.message() << "\n";
       }
   }

   return 0;
}
```

## 请参阅

[ibstream_view Class](./ibstream-class.md)

[ibstream Class](./ibstream-class.md#ibstream-class)
