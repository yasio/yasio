//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2025 HALX99

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

Version: 5.0.0

The vector aka array_buffer concepts:
   a. The memory model is similar to std::vector
   b. The resize behavior different from STL, can allocate exactly
   c. By default resize without fill (uninitialized for overwrite on POD path),
      use insert/append instead if you want fill memory inside container
   d. Support release internal buffer ownership with `release_pointer`
   e. Transparent iterator
   f. expand/append/insert/push_back will trigger memory allocate growth strategy
   g. resize_and_overwrite (c++23)
*/
#ifndef YASIO__VECTOR_HPP
#define YASIO__VECTOR_HPP

#include <utility>
#include <memory>
#include <iterator>
#include <limits>
#include <algorithm>
#include "yasio/type_traits.hpp"
#include "yasio/compiler/feature_test.hpp"
#include "yasio/memory.hpp" // for yasio::compressed_pair and ::yasio::construct_at

namespace yasio
{
template <typename _Ty, typename _Alloc = std::allocator<_Ty>>
class vector {
public:
  using value_type      = _Ty;
  using allocator_type  = _Alloc;
  using _Alloc_traits   = std::allocator_traits<_Alloc>;
  using pointer         = typename _Alloc_traits::pointer;
  using const_pointer   = typename _Alloc_traits::const_pointer;
  using reference       = value_type&;
  using const_reference = const value_type&;
  using size_type       = typename _Alloc_traits::size_type;
  using iterator        = pointer;
  using const_iterator  = const_pointer;

  // storage holder with three pointers: begin, last, end-cap
  struct _Vec_storage {
    pointer _Myfirst = nullptr; // begin
    pointer _Mylast  = nullptr; // one past last element
    pointer _Myend   = nullptr; // one past end of storage
  };

  vector() {}
  explicit vector(const allocator_type& alloc) : _Mypair(alloc, _Vec_storage{}) {}
  explicit vector(size_type count) { resize(static_cast<size_type>(count)); }
  vector(size_type count, const_reference val) { resize(static_cast<size_type>(count), val); }

  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  vector(_Iter first, _Iter last)
  {
    assign(first, last);
  }

  vector(const vector& rhs) { assign(rhs); }
  vector(vector&& rhs) YASIO__NOEXCEPT { _Assign_rv(std::move(rhs)); }

  ~vector() { _Tidy(); }

  vector& operator=(const vector& rhs)
  {
    assign(rhs);
    return *this;
  }
  vector& operator=(vector&& rhs) YASIO__NOEXCEPT
  {
    this->swap(rhs);
    return *this;
  }

  template <typename _Cont>
  vector& operator+=(const _Cont& rhs)
  {
    return this->append(std::begin(rhs), std::end(rhs));
  }

  vector& operator+=(const_reference rhs)
  {
    this->push_back(rhs);
    return *this;
  }

  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  void assign(_Iter first, _Iter last)
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();

    // if source is our own buffer, avoid self-assignment corruption
    auto ifirst = std::addressof(*first);
    static_assert(sizeof(*ifirst) == sizeof(value_type), "vector: iterator type incompatible!");
    if (ifirst != st._Myfirst)
    {
      clear();
      const auto count = static_cast<size_type>(std::distance(first, last));
      reserve(count);
      if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
        std::copy_n((iterator)ifirst, count, st._Myfirst);
      else
        std::uninitialized_copy((iterator)ifirst, (iterator)ifirst + count, st._Myfirst);
      st._Mylast = st._Myfirst + count;
    }
  }

  void assign(const vector& rhs) { assign(rhs.begin(), rhs.end()); }

  void assign(vector&& rhs) { _Assign_rv(std::move(rhs)); }

  void swap(vector& rhs) YASIO__NOEXCEPT
  {
    auto& a = _Mypair.second();
    auto& b = rhs._Mypair.second();
    std::swap(a._Myfirst, b._Myfirst);
    std::swap(a._Mylast, b._Mylast);
    std::swap(a._Myend, b._Myend);
  }

  // insert range by iterator
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  iterator insert(iterator pos, _Iter first, _Iter last)
  {
    auto& st = _Mypair.second();
    _YASIO_VERIFY_RANGE(pos >= st._Myfirst && pos <= st._Mylast && first <= last, "vector: out of range!");
    if (first == last)
      return pos;

    auto insertion_off = static_cast<size_type>(std::distance(st._Myfirst, pos));
    if (pos == st._Mylast)
    {
      append(first, last);
    }
    else
    {
      auto ifirst = std::addressof(*first);
      static_assert(sizeof(*ifirst) == sizeof(value_type), "vector: iterator type incompatible!");
      auto count = static_cast<size_type>(std::distance(first, last));

      const auto old_size = size();
      expand(count); // may reallocate, updates _Mylast/size
      // refresh pointers after possible reallocate
      pos        = _Mypair.second()._Myfirst + insertion_off;
      auto mlast = _Mypair.second()._Mylast;

      auto move_to    = pos + count;
      auto tail_count = static_cast<size_type>(mlast - move_to);

      if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      {
        // POD path: shift tail right into gap
        std::copy_n(pos, tail_count, move_to);
        // fill gap with incoming values
        std::copy_n((iterator)ifirst, count, pos);
      }
      else
      {
        // non-POD path:
        // 1) construct tail into new uninitialized slots
        std::uninitialized_move(pos, mlast - count, move_to);
        // 2) destroy original moved tail
        auto& alloc = _Mypair.first();
        for (iterator it = pos; it != mlast - count; ++it)
          _Alloc_traits::destroy(alloc, it);
        // 3) construct inserted elements into the gap
        std::uninitialized_copy((iterator)ifirst, (iterator)ifirst + count, pos);
      }
      // size already increased by expand(count)
    }
    return _Mypair.second()._Myfirst + insertion_off;
  }

  // insert count copies of val
  iterator insert(iterator pos, size_type count, const_reference val)
  {
    auto& st = _Mypair.second();
    _YASIO_VERIFY_RANGE(pos >= st._Myfirst && pos <= st._Mylast, "vector: out of range!");
    if (!count)
      return pos;

    auto insertion_off = static_cast<size_type>(std::distance(st._Myfirst, pos));
    if (pos == st._Mylast)
    {
      append(count, val);
    }
    else
    {
      const auto old_size = size();
      expand(count); // may reallocate
      pos             = _Mypair.second()._Myfirst + insertion_off;
      auto mlast      = _Mypair.second()._Mylast;
      auto move_to    = pos + count;
      auto tail_count = static_cast<size_type>(mlast - move_to);

      if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      {
        std::copy_n(pos, tail_count, move_to);
        std::fill_n(pos, count, val);
      }
      else
      {
        // move tail into new uninitialized slots
        std::uninitialized_move(pos, mlast - count, move_to);
        // destroy original moved tail
        auto& alloc = _Mypair.first();
        for (iterator it = pos; it != mlast - count; ++it)
          _Alloc_traits::destroy(alloc, it);
        // construct 'count' elements in gap
        std::uninitialized_fill_n(pos, count, val);
      }
      // size already increased by expand(count)
    }
    return _Mypair.second()._Myfirst + insertion_off;
  }

  // single-element insert
  iterator insert(iterator pos, const value_type& val) { return emplace(pos, val); }
  iterator insert(iterator pos, value_type&& val) { return emplace(pos, std::move(val)); }

  // emplace at position
  template <typename... _Valty>
  iterator emplace(iterator pos, _Valty&&... val)
  {
    auto& st           = _Mypair.second();
    auto insertion_off = static_cast<size_type>(std::distance(st._Myfirst, pos));
    _YASIO_VERIFY_RANGE(insertion_off <= size(), "vector: out of range!");
#if YASIO__HAS_CXX20
    emplace_back(std::forward<_Valty>(val)...);
    std::rotate(begin() + insertion_off, end() - 1, end());
    return (begin() + insertion_off);
#else
    // expand by one, shift tail, and construct at pos
    if (pos == st._Mylast)
    {
      emplace_back(std::forward<_Valty>(val)...);
    }
    else
    {
      expand(1);
      // refresh pos after expand
      pos        = _Mypair.second()._Myfirst + insertion_off;
      auto mlast = _Mypair.second()._Mylast;

      if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      {
        std::copy_n(pos, static_cast<size_type>(mlast - (pos + 1)), pos + 1);
        ::yasio::construct_at(pos, std::forward<_Valty>(val)...);
      }
      else
      {
        // non-POD: construct tail into new slot
        std::uninitialized_move(pos, mlast - 1, pos + 1);
        // destroy original moved tail
        auto& alloc = _Mypair.first();
        for (iterator it = pos; it != mlast - 1; ++it)
          _Alloc_traits::destroy(alloc, it);
        // construct the new element at pos
        _Alloc_traits::construct(_Mypair.first(), pos, std::forward<_Valty>(val)...);
      }
    }
    return _Mypair.second()._Myfirst + insertion_off;
#endif
  }

  // append range
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  vector& append(_Iter first, const _Iter last)
  {
    if (first == last)
      return *this;

    auto ifirst = std::addressof(*first);
    static_assert(sizeof(*ifirst) == sizeof(value_type), "vector: iterator type incompatible!");
    auto count          = static_cast<size_type>(std::distance(first, last));
    const auto old_size = size();
    expand(count);

    auto dst = _Mypair.second()._Myfirst + old_size;
    if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      std::copy_n((iterator)ifirst, count, dst);
    else
      std::uninitialized_copy((iterator)ifirst, (iterator)ifirst + count, dst);
    return *this;
  }

  // append count copies
  vector& append(size_type count, const_reference val)
  {
    if (!count)
      return *this;
    const auto old_size = size();
    expand(count);
    auto dst = _Mypair.second()._Myfirst + old_size;
    if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      std::fill_n(dst, count, val);
    else
      std::uninitialized_fill_n(dst, count, val);
    return *this;
  }

  // push_back
  void push_back(value_type&& val) { push_back(val); }
  void push_back(const value_type& val) { emplace_back(val); }

  // pop_back
  void pop_back()
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    if (!empty())
    {
      if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
        _Alloc_traits::destroy(alloc, st._Mylast - 1);
      st._Mylast = st._Mylast - 1;
    }
  }

  // emplace_back
  template <typename... _Valty>
  inline value_type& emplace_back(_Valty&&... val)
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    if (st._Mylast != st._Myend)
    {
      _Alloc_traits::construct(alloc, st._Mylast, std::forward<_Valty>(val)...);
      return *st._Mylast++;
    }
    return *_Emplace_back_reallocate(std::forward<_Valty>(val)...);
  }

  // erase single element by const_iterator
  iterator erase(const_iterator pos) { return erase(const_cast<iterator>(pos)); }

  // erase range by const_iterator
  iterator erase(const_iterator first, const_iterator last) { return erase(const_cast<iterator>(first), const_cast<iterator>(last)); }

  // erase single
  iterator erase(iterator pos)
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    _YASIO_VERIFY_RANGE(pos >= st._Myfirst && pos < st._Mylast, "vector: out of range!");
    iterator next = pos + 1;

    if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
    {
      _Alloc_traits::destroy(alloc, pos);
      std::move(next, st._Mylast, pos);
      --st._Mylast;
      _Alloc_traits::destroy(alloc, st._Mylast);
    }
    else
    {
      std::move(next, st._Mylast, pos);
      --st._Mylast;
    }
    return pos;
  }

  // erase range
  iterator erase(iterator first, iterator last)
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    _YASIO_VERIFY_RANGE((first <= last) && first >= st._Myfirst && last <= st._Mylast, "vector: out of range!");
    size_type count = static_cast<size_type>(last - first);

    if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
    {
      for (iterator it = first; it != last; ++it)
        _Alloc_traits::destroy(alloc, it);

      iterator new_end = std::move(last, st._Mylast, first);
      for (iterator it = new_end; it != st._Mylast; ++it)
        _Alloc_traits::destroy(alloc, it);
    }
    else
    {
      std::move(last, st._Mylast, first);
    }
    st._Mylast -= count;
    return first;
  }

  // front/back
  value_type& front()
  {
    _YASIO_VERIFY_RANGE(!empty(), "vector: out of range!");
    return *_Mypair.second()._Myfirst;
  }
  const value_type& front() const
  {
    _YASIO_VERIFY_RANGE(!empty(), "vector: out of range!");
    return *_Mypair.second()._Myfirst;
  }
  value_type& back()
  {
    _YASIO_VERIFY_RANGE(!empty(), "vector: out of range!");
    return *(_Mypair.second()._Mylast - 1);
  }
  const value_type& back() const
  {
    _YASIO_VERIFY_RANGE(!empty(), "vector: out of range!");
    return *(_Mypair.second()._Mylast - 1);
  }

  // iterators and data
  YASIO__CONSTEXPR size_type max_size() YASIO__NOEXCEPT { return _Alloc_traits::max_size(_Mypair.first()); }
  iterator begin() YASIO__NOEXCEPT { return _Mypair.second()._Myfirst; }
  iterator end() YASIO__NOEXCEPT { return _Mypair.second()._Mylast; }
  const_iterator begin() const YASIO__NOEXCEPT { return _Mypair.second()._Myfirst; }
  const_iterator end() const YASIO__NOEXCEPT { return _Mypair.second()._Mylast; }
  pointer data() YASIO__NOEXCEPT { return _Mypair.second()._Myfirst; }
  const_pointer data() const YASIO__NOEXCEPT { return _Mypair.second()._Myfirst; }
  size_type capacity() const YASIO__NOEXCEPT { return static_cast<size_type>(_Mypair.second()._Myend - _Mypair.second()._Myfirst); }
  size_type size() const YASIO__NOEXCEPT { return static_cast<size_type>(_Mypair.second()._Mylast - _Mypair.second()._Myfirst); }
  size_type length() const YASIO__NOEXCEPT { return size(); }

  // clear
  void clear() YASIO__NOEXCEPT
  {
    auto& st    = _Mypair.second();
    if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
    {
	  auto& alloc = _Mypair.first();
      for (pointer p = st._Myfirst; p != st._Mylast; ++p)
        _Alloc_traits::destroy(alloc, p);
    }
    st._Mylast = st._Myfirst;
  }

  bool empty() const YASIO__NOEXCEPT { return _Mypair.second()._Mylast == _Mypair.second()._Myfirst; }

  // operator[] and at
  const_reference operator[](size_type index) const { return this->at(index); }
  reference operator[](size_type index) { return this->at(index); }

  const_reference at(size_type index) const
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "vector: out of range!");
    return _Mypair.second()._Myfirst[index];
  }
  reference at(size_type index)
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "vector: out of range!");
    return _Mypair.second()._Myfirst[index];
  }

#pragma region modify size and capacity
  // resize without fill
  void resize(size_type new_size)
  {
    auto& st            = _Mypair.second();
    auto& alloc         = _Mypair.first();
    const auto old_size = size();
    if (capacity() < new_size)
      _Resize_reallocate<_Reallocation_policy::_Exactly>(new_size);

    if (new_size < old_size)
    {
      if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
      {
        for (pointer p = st._Myfirst + new_size; p != st._Mylast; ++p)
          _Alloc_traits::destroy(alloc, p);
      }
      st._Mylast = st._Myfirst + new_size;
    }
    else if (new_size > old_size)
    {
      if YASIO__CONSTEXPR (std::is_trivially_default_constructible_v<value_type>)
      {
        // POD path: leave as uninitialized for overwrite
        st._Mylast = st._Myfirst + new_size;
      }
      else
      {
        std::uninitialized_default_construct(st._Myfirst + old_size, st._Myfirst + new_size);
        st._Mylast = st._Myfirst + new_size;
      }
    }
  }

  // expand by count without fill
  void expand(size_type count)
  {
    auto& st            = _Mypair.second();
    const auto new_size = this->size() + count;
    if (this->capacity() < new_size)
      _Resize_reallocate<_Reallocation_policy::_At_least>(new_size);
    // initialize newly added region for non-POD
    if YASIO__CONSTEXPR (std::is_trivially_default_constructible_v<value_type>)
    {
      st._Mylast = st._Myfirst + new_size;
    }
    else
    {
      std::uninitialized_default_construct(st._Mylast, st._Myfirst + new_size);
      st._Mylast = st._Myfirst + new_size;
    }
  }

  // shrink_to_fit
  void shrink_to_fit()
  { // reduce capacity to size, provide strong guarantee
    auto& st = _Mypair.second();
    if (st._Mylast != st._Myend)
    {
      if (st._Mylast == st._Myfirst)
        _Tidy();
      else
        _Reallocate<_Reallocation_policy::_Exactly>(size());
    }
  }

  // reserve capacity
  void reserve(size_type new_cap)
  {
    if (this->capacity() < new_cap)
      _Reallocate<_Reallocation_policy::_Exactly>(new_cap);
  }

  // resize_and_overwrite (c++23-like)
  template <typename _Operation>
  void resize_and_overwrite(const size_type new_size, _Operation op)
  {
    _Reallocate<_Reallocation_policy::_Exactly>(new_size);
    auto& st   = _Mypair.second();
    st._Mylast = st._Myfirst + std::move(op)(st._Myfirst, new_size);
  }
#pragma endregion

  // resize with fill
  void resize(size_type new_size, const_reference val)
  {
    auto& st            = _Mypair.second();
    const auto old_size = this->size();
    if (old_size == new_size)
      return;

    resize(new_size);
    if (old_size < new_size)
    {
      auto dst        = st._Myfirst + old_size;
      auto fill_count = new_size - old_size;
      if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
        std::fill_n(dst, fill_count, val);
      else
        std::uninitialized_fill_n(dst, fill_count, val);
    }
  }

  // expand with fill
  void expand(size_type count, const_reference val)
  {
    if (!count)
      return;
    auto& st            = _Mypair.second();
    const auto old_size = this->size();
    expand(count);
    auto dst = st._Myfirst + old_size;
    if YASIO__CONSTEXPR (std::is_trivially_copy_constructible_v<value_type>)
      std::fill_n(dst, count, val);
    else
      std::uninitialized_fill_n(dst, count, val);
  }

  // helpers
  ptrdiff_t index_of(const_reference val) const YASIO__NOEXCEPT
  {
    auto it = std::find(begin(), end(), val);
    if (it != this->end())
      return std::distance(begin(), it);
    return -1;
  }

  void reset(size_type new_size)
  {
    resize(new_size);
    std::memset(_Mypair.second()._Myfirst, 0x0, size_bytes());
  }

  size_t size_bytes() const YASIO__NOEXCEPT { return static_cast<size_t>(this->size()) * sizeof(value_type); }

  template <typename _Intty>
  pointer detach_abi(_Intty& len) YASIO__NOEXCEPT
  {
    auto& st    = _Mypair.second();
    len         = static_cast<_Intty>(this->size());
    auto ptr    = st._Myfirst;
    st._Myfirst = st._Mylast = st._Myend = nullptr;
    return ptr;
  }

  pointer detach_abi() YASIO__NOEXCEPT
  {
    size_type ignored_len;
    return this->detach_abi(ignored_len);
  }

  void attach_abi(pointer ptr, size_type len)
  {
    _Tidy();
    auto& st    = _Mypair.second();
    st._Myfirst = ptr;
    st._Mylast  = ptr + len;
    st._Myend   = ptr + len;
  }

  pointer release_pointer() YASIO__NOEXCEPT { return detach_abi(); }

private:
  enum class _Reallocation_policy
  {
    _At_least,
    _Exactly
  };

  void _Eos(size_type new_size) YASIO__NOEXCEPT
  {
    auto& st   = _Mypair.second();
    st._Mylast = st._Myfirst + new_size;
  }

  // emplace_back with reallocate
  template <typename... _Valty>
  pointer _Emplace_back_reallocate(_Valty&&... val)
  {
    auto& st            = _Mypair.second();
    const auto old_size = size();

    if (old_size == max_size())
      throw std::length_error("vector too long");

    const size_type new_size = old_size + 1;
    _Resize_reallocate<_Reallocation_policy::_At_least>(new_size);
    auto& alloc = _Mypair.first();
    _Alloc_traits::construct(alloc, st._Myfirst + old_size, std::forward<_Valty>(val)...);
    st._Mylast = st._Myfirst + new_size;
    return st._Myfirst + old_size;
  }

  // growth calculation
  size_type _Calculate_growth(const size_type new_size) const
  {
    const size_type old_cap       = capacity();
    YASIO__CONSTEXPR auto max_cap = (std::numeric_limits<size_type>::max)();

    if (old_cap > max_cap - old_cap / 2)
      return max_cap; // geometric growth would overflow

    const size_type geometric = old_cap + (old_cap >> 1);

    if (geometric < new_size)
      return new_size; // geometric growth would be insufficient

    return geometric; // geometric growth is sufficient
  }

  // reallocate helpers
  template <_Reallocation_policy _Policy>
  void _Resize_reallocate(size_type size)
  {
    _Reallocate<_Policy>(size);
    _Eos(size);
  }

  template <_Reallocation_policy _Policy>
  void _Reallocate(size_type size)
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    size_type new_cap;
    if YASIO__CONSTEXPR (_Policy == _Reallocation_policy::_Exactly)
      new_cap = size;
    else
      new_cap = _Calculate_growth(size);

    pointer newbuf = _Alloc_traits::allocate(alloc, new_cap);
    // move-construct into new buffer
    if YASIO__CONSTEXPR (std::is_trivially_move_constructible_v<value_type>)
    {
      // POD path: copy bytes
      std::uninitialized_copy(st._Myfirst, st._Mylast, newbuf);
    }
    else
    {
      std::uninitialized_move(st._Myfirst, st._Mylast, newbuf);
    }

    // destroy old elements
    if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
    {
      for (pointer p = st._Myfirst; p != st._Mylast; ++p)
        _Alloc_traits::destroy(alloc, p);
    }
    // deallocate old storage
    if (st._Myfirst)
      _Alloc_traits::deallocate(alloc, st._Myfirst, static_cast<size_type>(st._Myend - st._Myfirst));

    // update storage pointers
    const auto old_size = static_cast<size_type>(st._Mylast - st._Myfirst);
    st._Myfirst         = newbuf;
    st._Mylast          = newbuf + old_size;
    st._Myend           = newbuf + new_cap;
  }

  // tidy release
  void _Tidy() YASIO__NOEXCEPT
  {
    auto& st    = _Mypair.second();
    auto& alloc = _Mypair.first();
    if (st._Myfirst)
    {
      if YASIO__CONSTEXPR (!std::is_trivially_destructible_v<value_type>)
      {
        for (pointer p = st._Myfirst; p != st._Mylast; ++p)
          _Alloc_traits::destroy(alloc, p);
      }
      _Alloc_traits::deallocate(alloc, st._Myfirst, static_cast<size_type>(st._Myend - st._Myfirst));
      st._Myfirst = st._Mylast = st._Myend = nullptr;
    }
  }

  void _Assign_rv(vector&& rhs)
  {
    // move allocator and storage pointers
    _Mypair.first()      = std::move(rhs._Mypair.first());
    _Mypair.second()     = rhs._Mypair.second();
    rhs._Mypair.second() = _Vec_storage{};
  }

  ::yasio::compressed_pair<allocator_type, _Vec_storage> _Mypair;
};

#pragma region c++20 like std::erase
template <typename _Ty, typename _Alloc>
void erase(vector<_Ty, _Alloc>& cont, const _Ty& val)
{
  cont.erase(std::remove(cont.begin(), cont.end(), val), cont.end());
}
template <typename _Ty, typename _Alloc, typename _Pr>
void erase_if(vector<_Ty, _Alloc>& cont, _Pr pred)
{
  cont.erase(std::remove_if(cont.begin(), cont.end(), pred), cont.end());
}
#pragma endregion

#pragma region ordered insert, for flat container emulating
template <typename _Cont>
inline typename _Cont::iterator ordered_insert(_Cont& vec, typename _Cont::value_type const& val)
{
  return vec.insert(std::upper_bound(vec.begin(), vec.end(), val), val);
}

template <typename _Cont, typename _Pred>
inline typename _Cont::iterator ordered_insert(_Cont& vec, typename _Cont::value_type const& val, _Pred pred)
{
  return vec.insert(std::upper_bound(vec.begin(), vec.end(), val, pred), val);
}
#pragma endregion

} // namespace yasio
#endif
