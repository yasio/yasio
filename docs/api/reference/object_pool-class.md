# object_pool Class template

对象池类模板。


## 语法

```cpp
namespace yasio {
template <typename _Ty, typename _Mutex = ::yasio::null_mutex>
class object_pool;
}
```

## 成员

### 公共构造函数

|Name|Description|
|----------|-----------------|
|[object_pool::object_pool](#object_pool)|构造1个`object_pool` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[object_pool::create](#create)|从对象池创建1个对象|
|[object_pool::destroy](#destroy)|销毁对象并回收到对象池|

## 要求

**头文件:** yasio/yasio.hpp

## <a name="object_pool"></a> object_pool::object_pool

构造 `object_pool` 对象。

```cpp
object_pool(size_t _ElemCount = 128);
```

### 参数

*_ElemCount*<br/>
单个chunk允许分配最大对象数量。

## <a name="create"></a> object_pool::create

创建1个对象。

```cpp
template <typename... _Types>
_Ty* create(_Types&&... args);
```

### 参数

*args*<br/>
构造对象所需参数，必须和对象类型构造函数匹配，否则编译报错。

## <a name="destroy"></a> object_pool::destroy

销毁对象，并将对象的内存块放回对象池中。

```cpp
void destroy(void* _Ptr);
```

### 参数

*_Ptr*<br/>
要销毁的对象地址。

## 示例

```cpp
#include "yasio/object_pool.hpp"

int main() {
    // 非线程安全版本
    yasio::object_pool<int> pool(128);
    auto value1 = pool.create(2023);
    auto value2 = pool.create(2024);
    auto value3 = pool.create(2025);
    pool.destroy(value3);
    pool.destroy(value1);
    pool.destroy(value2);

    // 线程安全版本
    yasio::object_pool<int, std::mutex> tsf_pool(128);
    value1 = tsf_pool.create(2023);
    value2 = tsf_pool.create(2024);
    value3 = tsf_pool.create(2025);
    tsf_pool.destroy(value3);
    tsf_pool.destroy(value1);
    tsf_pool.destroy(value2);

    return 0;
}
```
