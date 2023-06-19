# singleton Class template

单例类模板。


## 语法

```cpp
namespace yasio {
template <typename _Ty>
class singleton;
}
```

## 成员

### 静态方法

|Name|Description|
|----------|-----------------|
|[singleton::instance](#instance)|获取单例指针|
|[singleton::instance1](#instance1)|获取单例指针|
|[singleton::destroy](#destroy)|销毁单例|

## 要求

**头文件:** yasio/yasio.hpp

## <a name="instance"></a> singleton::instance

创建或获取单例对象。

```cpp
template <typename... _Types>
static pointer instance(_Types&&... args);
```

### 参数

*args*<br/>
构造单例对象所需参数。

## <a name="instance1"></a> singleton::instance1

创建或获取单例对象，使用二段式构造模式。

```cpp
template <typename... _Types>
static pointer instance1(_Types&&... args);
```

### 参数

*args*<br/>
构造单例对象所需参数。

## <a name="destroy"></a> singleton::destroy

销毁单例对象。

```cpp
static void destroy(void);
```

## 示例

```cpp
#include "yasio/singleton.hpp"

class FruitManager {
public:
    FruitManager(int maxCount) : _maxCount(maxCount) {
    }

    void print() {
        printf("FruitManager: maxCount=%d\n", _maxCount);
    }

private:
    int _maxCount;
};

class FruitManager1 {
public:
    void init(int maxCount){
        _maxCount = maxCount;
    }

    void print() {
        printf("FruitManager1: maxCount=%d\n", _maxCount);
    }

private:
    int _maxCount;
};

int main() {
    yasio::singleton<FruitManager>::instance(100)->print();
    yasio::singleton<FruitManager1>::instance1(&FruitManager1::init, 100)->print();

    yasio::singleton<FruitManager>::destroy();
    yasio::singleton<FruitManager1>::destroy();
    return 0;
}

```
