# 其他常用API

## 命名空间

```cpp
namespace yasio {}
```

## 公共方法

|Name|Description|
|----------|-----------------|
|[yasio::host_to_network](#host_to_network)|主机字节序转网络字节序|
|[yasio::network_to_host](#network_to_host)|网络字节序转主机字节序|
|[yasio::xhighp_clock](#xhighp_clock)|获取纳秒级时间戳|
|[yasio::highp_clock](#highp_clock)|获取微秒级时间戳|
|[yasio::clock](#clock)|获取毫秒级时间戳|
|[yasio::set_thread_name](#set_thread_name)|设置调用者线程名|
|[yasio::basic_strfmt](#basic_strfmt)|格式化字符串|


## <a name="host_to_network"></a> yasio::host_to_network

主机字节序转网络字节序。

### 头文件

`yasio/detail/endian_portable.hpp`

```cpp
template <typename _Ty>
inline _Ty host_to_network(_Ty value);
inline int host_to_network(int value, int size);
```

### 参数

*value*<br/>
要转换的数值，value类型 *_Ty* 可以是任意1~8字节数值类型。

*size*<br/>
要转换的数值有效字节数，只能是1~4字节。

### 返回值

网络字节序数值。

### 示例

```cpp
#include <stdio.h>
#include <inttypes.h>
#include "yasio/detail/endian_portable.hpp"
int main(){
    uint16_t v1 = 0x1122;
    uint32_t v2 = 0x11223344;
    uint64_t v3 = 0x1122334455667788;
    // output will be: net.v1=2211, net.v2=44332211, net.v3=8877665544332211
    printf("net.v1=%04" PRIx16 ", net.v2=%08" PRIx32 ", net.v3=%016" PRIx64 "\n",
        yasio::host_to_network(v1),
        yasio::host_to_network(v2),
        yasio::host_to_network(v3));
    return 0;
}
```

## <a name="network_to_host"></a> yasio::network_to_host

网络字节序转主机字节序。

### 头文件

`yasio/detail/endian_portable.hpp`

```cpp
template <typename _Ty>
inline _Ty network_to_host(_Ty value);
inline int network_to_host(int value, int size);
```

### 参数

*value*<br/>
要转换的数值，value类型 *_Ty* 可以是任意1~8字节数值类型。

*size*<br/>
要转换的数值有效字节数，只能是1~4字节。

### 返回值

主机字节序数值。

### 示例

```cpp
#include <stdio.h>
#include <inttypes.h>
#include "yasio/detail/endian_portable.hpp"
int main(){
    uint16_t v1 = 0x2211;
    uint32_t v2 = 0x44332211;
    uint64_t v3 = 0x8877665533442211;
    // output will be: net.v1=1122, net.v2=11223344, net.v3=1122334455667788
    printf("host.v1=%04" PRIx16 ", host.v2=%08" PRIx32 ", host.v3=%016" PRIx64 "\n",
        yasio::network_to_host(v1),
        yasio::network_to_host(v2),
        yasio::network_to_host(v3));
    return 0;
}
```

## <a name="xhighp_clock"></a> yasio::xhighp_clock

获取纳秒级时间戳。

### 头文件

`yasio/detail/utils.hpp`

```cpp
template <typename _Ty = steady_clock_t>
inline highp_time_t xhighp_clock();
```

### 模板参数

*_Ty*<br/>

- yasio::steady_clock_t: 返回当前系统时间无关的时间戳
- yasio::system_clock_t: 返回系统UTC时间戳

### 返回值

纳秒级时间戳。

## <a name="highp_clock"></a> yasio::highp_clock

获取微秒级时间戳。

### 头文件

`yasio/detail/utils.hpp`

```cpp
template <typename _Ty = steady_clock_t>
inline highp_time_t highp_clock();
```

### 模板参数

*_Ty*<br/>

- `yasio::steady_clock_t`: 返回当前系统时间无关的时间戳
- `yasio::system_clock_t`: 返回系统UTC时间戳

### 返回值

微秒级时间戳。

## <a name="clock"></a> yasio::clock

获取毫秒级时间戳。

### 头文件

`yasio/detail/utils.hpp`

```cpp
template <typename _Ty = steady_clock_t>
inline highp_time_t clock();
```

### 模板参数

*_Ty*<br/>

- `yasio::steady_clock_t`: 返回当前系统时间无关的时间戳
- `yasio::system_clock_t`: 返回系统UTC时间戳

### 返回值

毫秒级时间戳。

## <a name="set_thread_name"></a> yasio::set_thread_name

设置调用者线程名。

### 头文件

`yasio/detail/thread_name.hpp`

```cpp
inline void set_thread_name(const char* name);
```

### 参数

*name*<br/>
需要设置线程名称

### 注意

此函数用于诊断多线程程序非常有用。

## <a name="basic_strfmt"></a> yasio::basic_strfmt

函数模板，格式化字符串或者宽字符串。

### 头文件

`yasio/detail/strfmt.hpp`

```cpp
template <class _Elem, class _Traits = std::char_traits<_Elem>,
          class _Alloc = std::allocator<_Elem>>
inline std::basic_string<_Elem, _Traits, _Alloc> basic_strfmt(size_t n, const _Elem* format, ...);
```

### 参数

*n*<br/>
初始buffer大小

*format*<br/>
格式字符串，和标准库`printf`相同

### 返回值

返回格式化后的`std::string`类型字符串。


### 注意

实际使用，可直接使用已经定义好的别名

- `yasio::strfmt`: 格式化字符串
- `yasio::wcsfmt`: 格式化宽字符串

### 示例

```cpp
#include "yasio/detail/strfmt.hpp"
int main() {
    std::string str1 = yasio::strfmt(64, "My age is %d", 19);
    std::wstring str2 = yasio::wcsfmt(64, L"My age is %d", 19);
    return 0;
}
```
