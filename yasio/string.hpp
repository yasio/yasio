//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2023 HALX99

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
// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

/**
 * A alternative stl compatible string implementation, incomplete
 * refer to: https://github.com/llvm/llvm-project/blob/main/libcxx/include/string
 * Features:
 *   - LLVM libcxx string SSO layout
 *   - resize_and_overwrite
 *   - transparent iterator
 *   - resize exactly
 *   - expand for growth strategy
 * TODO:
 *   - find stubs
 *   - unit tests or fuzz tests
 */

#ifndef YASIO__STRING_HPP
#define YASIO__STRING_HPP
#include <utility>
#include <memory>
#include <iterator>
#include <limits>
#include <algorithm>
#include <initializer_list>
#include "yasio/buffer_alloc.hpp"
#include "yasio/string_view.hpp"

#if !defined(_NOEXCEPT)
#  define _NOEXCEPT YASIO__NOEXCEPT
#endif

namespace yasio
{
template <typename _Elem, enable_if_t<std::is_integral<_Elem>::value && sizeof(_Elem) <= sizeof(char32_t), int> = 0>
using default_string_allocater = default_buffer_allocator<_Elem>;

template <typename _Elem, typename _Alloc = default_string_allocater<_Elem>>
class basic_string {
public:
  using pointer         = _Elem*;
  using const_pointer   = const _Elem*;
  using reference       = _Elem&;
  using const_reference = const _Elem&;
  using size_type       = size_t;
  using value_type      = _Elem;
  using iterator        = _Elem*; // byte_buffer only needs transparent iterator
  using const_iterator  = const _Elem*;
  using allocator_type  = _Alloc;
  using view_type       = cxx17::basic_string_view<_Elem>;
  using _Traits         = std::char_traits<_Elem>;
  basic_string() { __zero(); }
  explicit basic_string(size_type count) { resize(count); }
  explicit basic_string(view_type rhs) { assign(rhs); }
  basic_string(const _Elem* ntcs) { assign(ntcs); }
  basic_string(size_type count, const_reference val) { resize(count, val); }
  template <typename _Iter, ::yasio::is_iterator<_Iter>::value>
  basic_string(_Iter first, _Iter last)
  {
    assign(first, last);
  }
  basic_string(const basic_string& rhs) { assign(rhs); };
  basic_string(basic_string&& rhs) _NOEXCEPT { assign(std::move(rhs)); }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  basic_string(std::initializer_list<_Ty> rhs)
  {
    assign(rhs);
  }
  ~basic_string() { _Tidy(); }
  basic_string& operator=(const basic_string& rhs)
  {
    assign(rhs);
    return *this;
  }
  basic_string& operator=(basic_string&& rhs) _NOEXCEPT
  {
    this->swap(rhs);
    return *this;
  }

  template <size_t _N>
  basic_string& operator+=(const _Elem (&rhs)[_N])
  {
    return append(rhs, rhs + _N - 1);
  }
  basic_string& operator+=(const_reference rhs)
  {
    this->push_back(rhs);
    return *this;
  }
  operator view_type() const { return this->view(); }
  view_type view() const { return view_type{this->data(), this->size()}; }
  basic_string& substr(size_t offset, size_t count) { return basic_string{view().substr(offset, count)}; }
  template <typename _Iter>
  void assign(const _Iter first, const _Iter last)
  {
    _Assign_range(first, last);
  }
  void assign(const basic_string& rhs) { _Assign_range(rhs.begin(), rhs.end()); }
  void assign(basic_string&& rhs) { _Assign_rv(std::move(rhs)); }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  void assign(std::initializer_list<_Ty> rhs)
  {
    _Assign_range((iterator)rhs.begin(), (iterator)rhs.end());
  }
  void assign(view_type rhs) { this->assign(rhs.begin(), rhs.end()); }
  void assign(const _Elem* ntcs) { this->assign(ntcs, ntcs + _Traits::length(ntcs)); }
  void swap(basic_string& rhs) _NOEXCEPT
  {
    std::swap(__r.__words[0], rhs.__r.__words[0]);
    std::swap(__r.__words[1], rhs.__r.__words[1]);
    std::swap(__r.__words[2], rhs.__r.__words[2]);
  }
  basic_string& insert(size_t offset, view_type value)
  {
    insert(__get_pointer() + offset, std::begin(value), std::end(value));
    return *this;
  }
  template <typename _Iter>
  basic_string& append(_Iter first, const _Iter last)
  {
    insert(end(), first, last);
    return *this;
  }
  basic_string& operator+=(view_type rhs) { return append(rhs.begin(), rhs.end()); }
  size_type find(view_type what) { return this->view().find(what); }
  void push_back(value_type v)
  {
    resize(this->size() + 1);
    back() = v;
  }
  iterator erase(iterator _Where)
  {
    _YASIO_VERIFY_RANGE(_Where >= begin() && _Where < end(), "string: out of range!");
    std::move(_Where + 1, end(), _Where);
    __set_size(size() - 1);
    return _Where;
  }
  iterator erase(iterator first, iterator last)
  {
    _YASIO_VERIFY_RANGE((first <= last) && first >= begin() && last <= end(), "string: out of range!");

    auto nremove = std::distance(first, last);
    std::move(last, end(), first);
    __set_size(size() - nremove);

    return first;
  }
  value_type& front()
  {
    _YASIO_VERIFY_RANGE(!empty(), "string: out of range!");
    return *__get_pointer();
  }
  value_type& back()
  {
    _YASIO_VERIFY_RANGE(!empty(), "string: out of range!");
    return end()[-1];
  }
  static YASIO__CONSTEXPR size_type max_size() _NOEXCEPT { return (std::numeric_limits<ptrdiff_t>::max)(); }
  iterator begin() _NOEXCEPT { return __get_pointer(); }
  iterator end() _NOEXCEPT { return __get_pointer() + size(); }
  const_iterator begin() const _NOEXCEPT { return __get_pointer(); }
  const_iterator end() const _NOEXCEPT { return __get_pointer() + size(); }
  pointer data() _NOEXCEPT { return __get_pointer(); }
  const_pointer data() const _NOEXCEPT { return __get_pointer(); }
  const_pointer c_str() const { return __get_pointer(); }
  size_type capacity() const _NOEXCEPT { return __is_long() ? __get_long_cap() : static_cast<size_type>(__min_cap) - 1; }
  size_type length() const _NOEXCEPT { return size(); }
  size_type size() const _NOEXCEPT { return __is_long() ? __get_long_size() : __get_short_size(); }
  void clear() _NOEXCEPT { __set_size(0); }
  bool empty() const _NOEXCEPT { return size() == 0; }

  const_reference operator[](size_type index) const { return this->at(index); }
  reference operator[](size_type index) { return this->at(index); }
  const_reference at(size_type index) const
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "string: out of range!");
    return __get_pointer()[index];
  }
  reference at(size_type index)
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "string: out of range!");
    return __get_pointer()[index];
  }
  template <typename _Operation>
  void resize_and_overwrite(const size_type _New_size, _Operation _Op)
  {
    _Reallocate<_Reallocation_policy::_Exactly>(_New_size);
    const auto _Result_size = std::move(_Op)(__get_pointer(), _New_size);
    _Eos(_Result_size);
  }
  void resize(size_type new_size, const_reference val)
  {
    auto old_size = this->size();
    resize(new_size);
    if (old_size < new_size)
      ::memset(__get_pointer() + old_size, val, new_size - old_size);
  }
  void resize(size_type new_size)
  {
    if (this->capacity() < new_size)
      _Resize_reallocate<_Reallocation_policy::_Exactly>(new_size);
    else
      __set_size(new_size);
  }
  void expand(size_type count, const_reference val)
  {
    if (count)
    {
      const auto old_size = this->size();
      expand(count);
      ::memset(__get_pointer() + old_size, val, count);
    }
  }
  void expand(size_type count)
  {
    if (count)
    {
      const auto new_size = this->size() + count;
      if (this->capacity() < new_size)
        _Resize_reallocate<_Reallocation_policy::_At_least>(new_size);
      else
        _Eos(new_size);
    }
  }
  void reserve(size_type new_cap)
  {
    if (this->capacity() < new_cap)
      _Reallocate<_Reallocation_policy::_Exactly>(new_cap);
  }
  void shrink_to_fit()
  { // TODO
  }
  template <typename _Iter>
  iterator insert(iterator _Where, _Iter first, const _Iter last)
  {
    _YASIO_VERIFY_RANGE(_Where >= begin() && _Where <= end() && first <= last, "string: out of range!");
    if (first != last)
    {
      auto _Myfirst      = __get_pointer();
      auto count         = std::distance(first, last);
      auto insertion_pos = std::distance(_Myfirst, _Where);
      if (_Where == end())
      {
        if (count > 1)
        {
          auto old_size = this->size();
          expand(count);
          _Myfirst = __get_pointer();
          std::copy_n(first, count, _Myfirst + old_size);
        }
        else if (count == 1)
          push_back(static_cast<value_type>(*first));
      }
      else
      {
        if (insertion_pos >= 0)
        {
          const auto new_size = this->size() + count;
          expand(count);
          _Myfirst     = __get_pointer();
          _Where       = _Myfirst + insertion_pos;
          auto move_to = _Where + count;
          std::copy_n(_Where, end() - move_to, move_to);
          std::copy_n(first, count, _Where);

          _Eos(new_size);
        }
      }
      return _Myfirst + insertion_pos;
    }
    return _Where;
  }

private:
  template <typename _Iter>
  void _Assign_range(_Iter first, _Iter last)
  {
    auto ifirst = std::addressof(*first);
    static_assert(sizeof(*ifirst) == sizeof(value_type), "pod_vector: iterator type incompatible!");
    if (ifirst != __get_pointer())
    {
      __set_size(0);
      if (last > first)
      {
        const auto count = std::distance(first, last);
        resize(count);
        std::copy_n((iterator)ifirst, count, __get_pointer());
      }
    }
  }
  void _Assign_rv(basic_string&& rhs)
  {
    memcpy(this, &rhs, sizeof(rhs));
    memset(&rhs, 0, sizeof(rhs));
  }
#pragma region long string only
  enum class _Reallocation_policy
  {
    _At_least,
    _Exactly
  };
  template <_Reallocation_policy _Policy>
  void _Resize_reallocate(size_type size)
  {
    _Reallocate<_Policy>(size);
    _Eos(size);
  }
  template <_Reallocation_policy _Policy>
  void _Reallocate(size_type size)
  {
    size_type new_cap;
    if constexpr (_Policy == _Reallocation_policy::_Exactly)
      new_cap = size + 1;
    else
      new_cap = (std::max)(_Calculate_growth(size), size + 1);

    auto _Newvec = _Alloc::reallocate(__get_long_pointer(), __get_long_cap(), new_cap);
    if (_Newvec)
    {
      __set_long_pointer(_Newvec);
      __set_long_cap(new_cap - 1);
    }
    else
      throw std::bad_alloc{};
  }
#pragma endregion
  void _Eos(size_t new_size)
  {
    __set_size(new_size);
    __get_pointer()[new_size] = 0;
  }
  size_type _Calculate_growth(const size_type _Newsize) const
  {
    // given _Oldcapacity and _Newsize, calculate geometric growth
    const size_type _Oldcapacity = capacity();
    YASIO__CONSTEXPR auto _Max   = max_size();

    if (_Oldcapacity > _Max - _Oldcapacity / 2)
      return _Max; // geometric growth would overflow

    const size_type _Geometric = _Oldcapacity + (_Oldcapacity >> 1);

    if (_Geometric < _Newsize)
      return _Newsize; // geometric growth would be insufficient

    return _Geometric; // geometric growth is sufficient
  }
  void _Tidy() _NOEXCEPT
  { // free all storage
    if (__is_long())
      _Alloc::deallocate(__get_long_pointer(), __get_long_cap());
    __zero();
  }

  void __set_long_pointer(pointer __p) _NOEXCEPT { __l.__data_ = __p; }
  pointer __get_long_pointer() _NOEXCEPT { return __l.__data_; }
  const_pointer __get_long_pointer() const _NOEXCEPT { return __l.__data_; }
  pointer __get_short_pointer() _NOEXCEPT { return &__s.__data_[0]; }
  const_pointer __get_short_pointer() const _NOEXCEPT { return &__s.__data_[0]; }
  pointer __get_pointer() _NOEXCEPT { return __is_long() ? __get_long_pointer() : __get_short_pointer(); }
  const_pointer __get_pointer() const _NOEXCEPT { return __is_long() ? __get_long_pointer() : __get_short_pointer(); }

  void __set_size(size_type sz) _NOEXCEPT
  {
    if (__is_long())
      __set_long_size(sz);
    else
      __set_short_size(sz);
  }

  void __set_short_size(size_type sz) _NOEXCEPT
  {
    __s.__size_    = sz;
    __s.__is_long_ = false;
  }

  size_type __get_short_size() const _NOEXCEPT { return __s.__size_; }

  void __set_long_size(size_type sz) _NOEXCEPT { __l.__size_ = sz; }
  size_type __get_long_size() const _NOEXCEPT { return __l.__size_; }

  size_type __get_long_cap() const _NOEXCEPT { return __l.__cap_ * __endian_factor; }
  void __set_long_cap(size_type sz) _NOEXCEPT
  {
    __l.__cap_     = sz / __endian_factor;
    __l.__is_long_ = true;
  }

  bool __is_long() const _NOEXCEPT { return __s.__is_long_; }
  void __zero() { memset(&__r, 0, sizeof(__r)); }

  static const size_type __short_mask = 0x01;
  static const size_type __long_mask  = 0x1ul;

#ifdef _LIBCPP_BIG_ENDIAN
  static const size_type __endian_factor = 1;
#else
  static const size_type __endian_factor = 2;
#endif

  struct __long {
    __long() : __is_long_(0), __cap_(0), __size_(0), __data_(nullptr) {}
    struct {
      size_type __is_long_ : 1;
      size_type __cap_ : sizeof(size_type) * CHAR_BIT - 1;
    };
    size_type __size_;
    value_type* __data_;
  };

  enum
  {
    __min_cap = (sizeof(__long) - 1) / sizeof(value_type) > 2 ? (sizeof(__long) - 1) / sizeof(value_type) : 2
  };

  union {
    __long __l{};
    struct {
      union {
        uint8_t __is_long_ : 1;
        uint8_t __size_ : 7;
        value_type __lx;
      };
      value_type __data_[__min_cap];
    } __s;
    struct {
      size_type __words[3];
    } __r;
  };
};
using string = basic_string<char>;
#if defined(__cpp_lib_char8_t)
using u8string = basic_string<char8_t>;
#endif
using wstring   = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
} // namespace yasio
#endif
