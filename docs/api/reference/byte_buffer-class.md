# byte_buffer 类

顾名思义，二进制字节buffer。

!!! attention "概念"

    - 对象内存模型和std::vector<char>类似
    - 支持精确resize
    - resize时默认不填充0(uninitialized and for overwrite)
    - 支持释放内存管理权 `release_pointer`
    - 支持两种allocator
       - yasio::default_byte_allocator (new/delete)
       - yasio::crt_byte_allocator (malloc/free)
    - 可存储一切连续内存，例如: 
      ```cpp
      std:vector<int> vints{1,2,3};
      yasio::byte_buffer vints_bytes{vints.begin(), vints.end(), std::true_type{}}'
      ```

## 语法

```cpp
namespace yasio { 
template <typename _Elem, typename _Alloc = default_byte_allocator<_Elem>,
        enable_if_t<is_byte_type<_Elem>::value, int> = 0>
class basic_byte_buffer;
using sbyte_buffer = basic_byte_buffer<char>;
using byte_buffer  = basic_byte_buffer<unsigned char>;
}
```


## 成员

### 公共构造函数

|Name|Description|
|----------|-----------------|
|[basic_byte_buffer::basic_byte_buffer](#basic_byte_buffer)|构造1个`basic_byte_buffer` 对象|

### 公共方法

|Name|Description|
|----------|-----------------|
|[basic_byte_buffer::assign](#assign)|销毁对象并回收到对象池|
|[basic_byte_buffer::swap](#swap)|销毁对象并回收到对象池|
|[basic_byte_buffer::insert](#insert)|销毁对象并回收到对象池|
|[basic_byte_buffer::append](#append)|销毁对象并回收到对象池|
|[basic_byte_buffer::push_back](#push_back)|销毁对象并回收到对象池|
|[basic_byte_buffer::erase](#erase)|销毁对象并回收到对象池|
|[basic_byte_buffer::front](#front)|销毁对象并回收到对象池|
|[basic_byte_buffer::back](#back)|销毁对象并回收到对象池|
|[basic_byte_buffer::begin](#begin)|销毁对象并回收到对象池|
|[basic_byte_buffer::end](#end)|销毁对象并回收到对象池|
|[basic_byte_buffer::data](#data)|销毁对象并回收到对象池|
|[basic_byte_buffer::capacity](#capacity)|销毁对象并回收到对象池|
|[basic_byte_buffer::size](#size)|销毁对象并回收到对象池|
|[basic_byte_buffer::clear](#clear)|销毁对象并回收到对象池|
|[basic_byte_buffer::empty](#empty)|销毁对象并回收到对象池|
|[basic_byte_buffer::at](#at)|销毁对象并回收到对象池|
|[basic_byte_buffer::resize](#resize)|销毁对象并回收到对象池|
|[basic_byte_buffer::resize_fit](#resize_fit)|销毁对象并回收到对象池|
|[basic_byte_buffer::reserve](#reserve)|销毁对象并回收到对象池|
|[basic_byte_buffer::shrink_to_fit](#shrink_to_fit)|销毁对象并回收到对象池|
|[basic_byte_buffer::release_pointer](#release_pointer)|销毁对象并回收到对象池|

### 操作符

|Name|Description|
|----------|-----------------|
|[basic_byte_buffer::operator[]](#index_op)|销毁对象并回收到对象池|
|[basic_byte_buffer::operator=](#assign_op)|从对象池创建1个对象|

## 要求

**头文件:** yasio/core/byte_buffer.hpp

## <a name="basic_byte_buffer"></a> basic_byte_buffer::basic_byte_buffer

构造 `byte_buffer` 对象。

```cpp
basic_byte_buffer() {}
  explicit basic_byte_buffer(size_type count);
  basic_byte_buffer(size_type count, std::true_type /*fit*/);
  basic_byte_buffer(size_type count, const_reference val);
  basic_byte_buffer(size_type count, const_reference val, std::true_type /*fit*/);
  template <typename _Iter>
  basic_byte_buffer(_Iter first, _Iter last);
  template <typename _Iter>
  basic_byte_buffer(_Iter first, _Iter last, std::true_type /*fit*/);
  basic_byte_buffer(const basic_byte_buffer& rhs);
  basic_byte_buffer(const basic_byte_buffer& rhs, std::true_type /*fit*/);
  basic_byte_buffer(basic_byte_buffer&& rhs) YASIO__NOEXCEPT { assign(std::move(rhs)); }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  basic_byte_buffer(std::initializer_list<_Ty> rhs);
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  basic_byte_buffer(std::initializer_list<_Ty> rhs, std::true_type /*fit*/);
```

### 参数

*count*<br/>
初始字节数。
  
*fit*<br/>
分配合适大小标记。
  
*first*
要复制的元素范围内的第一个元素的位置。
  
*last*<br/>
超出要复制的元素范围的第一个元素的位置。

rhs <br />
要成为副本的构造的源。

## <a name="assign"></a> basic_byte_buffer::assign

清除byte_buffer并将指定的元素复制到该空byte_buffer。

```cpp
template <typename _Iter>
void assign(const _Iter first, const _Iter last);
template <typename _Iter>
void assign(const _Iter first, const _Iter last, std::true_type /*fit*/);
void assign(const basic_byte_buffer& rhs) { _Assign_range(rhs.begin(), rhs.end()); }
void assign(const basic_byte_buffer& rhs, std::true_type);
void assign(basic_byte_buffer&& rhs) { _Assign_rv(std::move(rhs)); }
template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
void assign(std::initializer_list<_Ty> rhs);
template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
void assign(std::initializer_list<_Ty> rhs, std::true_type /*fit*/);
```

### 参数

*first*<br/>
要复制的元素范围内的第一个元素的位置。
  
*last<br/>
超出要复制的元素范围的第一个元素的位置。

*rhs*<br/>
要成为副本的构造的源。

## <a name="swap"></a> basic_byte_buffer::swap

交换两个byte_buffer的元素。

```cpp
void swap(basic_byte_buffer& rhs);
```

### 参数

*rhs*<br/>
一个提供要交换的元素的byte_buffer。

## <a name="insert"></a> basic_byte_buffer::insert

在指定位置插入字节数据。

```cpp
template <typename _Iter>
iterator insert(iterator _Where, _Iter first, const _Iter last);
```

### 参数

*_Where*<br/>
要插入的位置。
  
*first*<br/>
要复制的元素范围内的第一个元素的位置。
  
*last*<br/>
超出要复制的元素范围的第一个元素的位置。


### 返回值
返回插入元素位置迭代器。

## <a name="append"></a> basic_byte_buffer::append

向byte_buffer追加字节数据。

```cpp
template <typename _Iter>
basic_byte_buffer& append(_Iter first, const _Iter last);
```

### 参数

*first*<br/>
要复制的元素范围内的第一个元素的位置。
  
*last*<br/>
超出要复制的元素范围的第一个元素的位置。

### 返回值
当前byte_buffer对象。


## <a name="push_back"></a> basic_byte_buffer::push_back

向byte_buffer尾部追加1个字节。

```cpp
void push_back(value_type v);
```

### 参数

*v*<br/>
要追加的字节值。

## <a name="push_back"></a> basic_byte_buffer::erase

删除指定迭代器元素。

```cpp
iterator erase(iterator _Where);
```

### 参数

*_Where*<br/>
要删除元素迭代器。

### 返回值
返回删除元素的下一个迭代器。

## <a name="front"></a> basic_byte_buffer::front

获取byte_buffer第一个字节。

```cpp
value_type& front();
```

### 返回值
返回byte_buffer第一个字节。

## <a name="front"></a> basic_byte_buffer::back

获取byte_buffer最后一个字节。

```cpp
value_type& front();
```

### 返回值
byte_buffer最后一个字节。

## <a name="begin"></a> `begin`

Returns a random-access iterator to the first element in the byte_buffer.

```cpp
const_iterator begin() const;

iterator begin();
```

### Return value

A random-access iterator addressing the first element in the `basic_byte_buffer` or to the location succeeding an empty `basic_byte_buffer`. Always compare the value returned with [`basic_byte_buffer::end`](#end) to ensure it's valid.

## <a name="end"></a> `end`

Returns a past-the-end iterator that points to the element following the last element of the vector.

```cpp
iterator end();

const_iterator end() const;
```

### Return value

A past-the-end iterator for the vector. It points to the element following the last element of the vector. That element is a placeholder and shouldn't be dereferenced. Only use it for comparisons. If the vector is empty, then `basic_byte_buffer::end() == basic_byte_buffer::begin()`.

