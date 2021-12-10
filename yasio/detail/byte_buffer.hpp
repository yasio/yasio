
//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2021 HALX99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/* The byte_buffer concepts
   a. The memory model same with std::string, std::vector<char>
   b. use c realloc/free to manage memory
   c. implemented operations:
      - resize(without fill)
      - detach(stl not support)
      - stl likes: insert, reserve, front, begin, end, push_back and etc.
*/

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <memory>
#include <vector>
#include <exception>

namespace yasio
{
template <typename _Elem>
class basic_byte_buffer final {
public:
  using pointer       = _Elem*;
  using const_pointer = const _Elem*;
  using size_type     = size_t;

  basic_byte_buffer() {}
  basic_byte_buffer(const _Elem* start, const _Elem* end) { assign(start, end); }
  basic_byte_buffer(size_t size, _Elem val) { resize(size, val); }
  basic_byte_buffer(const basic_byte_buffer& rhs) { assign(rhs.begin(), rhs.end()); };
  basic_byte_buffer(basic_byte_buffer&& rhs)
  {
    memcpy(this, &rhs, sizeof(rhs));
    memset(&rhs, 0, sizeof(rhs));
  }
  basic_byte_buffer(const std::vector<_Elem>& rhs) { assign(rhs.data(), rhs.data() + rhs.size()); }

  ~basic_byte_buffer() { _Tidy(); }

  basic_byte_buffer& operator=(const basic_byte_buffer& rhs) { return assign(rhs.begin(), rhs.end()); }
  basic_byte_buffer& operator=(basic_byte_buffer&& rhs) { return this->swap(rhs); }

  basic_byte_buffer& assign(const _Elem* start, const _Elem* end)
  {
    ptrdiff_t count = end - start;
    if (count > 0)
      memcpy(resize(count), start, count);
    else
      clear();
    return *this;
  }
  basic_byte_buffer& swap(basic_byte_buffer& rhs)
  {
    std::swap(_buf, rhs._buf);
    std::swap(_size, rhs._size);
    std::swap(_capacity, rhs._capacity);
    return *this;
  }
  void insert(_Elem* where, const void* start, const void* end)
  {
    size_t count = (const _Elem*)end - (const _Elem*)start;
    if (count > 0)
    {
      auto offset   = where - this->begin();
      auto old_size = size();
      resize(old_size + count);

      if (offset >= static_cast<ptrdiff_t>(old_size))
        memcpy(_buf + old_size, start, count);
      else if (offset >= 0)
      {
        auto ptr = this->begin() + offset;
        auto to  = ptr + count;
        memmove(to, ptr, this->end() - to);
        memcpy(ptr, start, count);
      }
    }
  }
  void push_back(_Elem v)
  {
    auto old_size                  = size();
    resize(old_size + 1)[old_size] = v;
  }
  _Elem& front()
  {
    if (!empty())
      return *_buf;
    else
      throw std::out_of_range("byte_buffer: out of range!");
  }
  _Elem* begin() { return _buf; }
  _Elem* end() { return _buf ? _buf + _size : nullptr; }
  const _Elem* begin() const { return _buf; }
  const _Elem* end() const { return _buf ? _buf + _size : nullptr; }
  pointer data() { return _buf; }
  const_pointer data() const { return _buf; }
  void reserve(size_t capacity)
  {
    if (_capacity < capacity)
      _Reset_cap(capacity);
  }
  _Elem* resize(size_t new_size, _Elem val)
  {
    auto old_size = size();
    auto new_buf  = resize(new_size);
    if (old_size < new_size)
      memset(new_buf + old_size, val, new_size - old_size);
    return new_buf;
  }
  _Elem* resize(size_t size)
  {
    if (_size > size)
      _size = size;
    else
    {
      if (_capacity < size)
        _Reset_cap(size * 3 / 2);

      _size = size;
    }
    return _buf;
  }
  size_t size() const { return _size; }
  void clear() { _size = 0; }
  bool empty() const { return _size == 0; }
  void shrink_to_fit() { _Reset_cap(size()); }
  template <typename _TSIZE>
  _Elem* detach(_TSIZE& size)
  {
    auto ptr = _buf;
    size     = static_cast<_TSIZE>(_size);
    memset(this, 0, sizeof(*this));
    return ptr;
  }

private:
  void _Tidy()
  {
    clear();
    shrink_to_fit();
  }
  void _Reset_cap(size_t capacity)
  {
    if (capacity > 0)
    {
      auto new_blk = (_Elem*)realloc(_buf, capacity);
      if (new_blk)
      {
        _buf      = new_blk;
        _capacity = capacity;
      }
      else
        throw std::bad_alloc();
    }
    else
    {
      if (_buf != nullptr)
      {
        free(_buf);
        _buf = nullptr;
      }
      _capacity = 0;
    }
  }

private:
  _Elem* _buf      = nullptr;
  size_t _size     = 0;
  size_t _capacity = 0;
};
using sbyte_buffer = basic_byte_buffer<char>;
using byte_buffer  = basic_byte_buffer<uint8_t>;
} // namespace yasio
