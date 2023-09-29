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

/**
 * A alternative stl compatible string implementation, incomplete
 * Features:
 *   - resize_and_overwrite
 *   - transparent iterator
 *   - resize_fit
 *   - release internal string ownership
 *   - plain struct memory model same with std::vector in msvc, no small buffer, no COW optimized
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
  // using _Myty           = basic_string<_Elem, _Alloc>;
  basic_string() {}
  explicit basic_string(size_type count) { resize(count); }
  explicit basic_string(view_type rhs) { assign(rhs); }
  basic_string(const _Elem* ntcs) { assign(ntcs); }
  basic_string(size_type count, std::true_type /*fit*/) { resize_fit(count); }
  basic_string(size_type count, const_reference val) { resize(count, val); }
  basic_string(size_type count, const_reference val, std::true_type /*fit*/) { resize_fit(count, val); }
  template <typename _Iter>
  basic_string(_Iter first, _Iter last)
  {
    assign(first, last);
  }
  template <typename _Iter>
  basic_string(_Iter first, _Iter last, std::true_type /*fit*/)
  {
    assign(first, last, std::true_type{});
  }
  basic_string(const basic_string& rhs) { assign(rhs); };
  basic_string(const basic_string& rhs, std::true_type /*fit*/) { assign(rhs, std::true_type{}); }
  basic_string(basic_string&& rhs) YASIO__NOEXCEPT { assign(std::move(rhs)); }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  basic_string(std::initializer_list<_Ty> rhs)
  {
    assign(rhs);
  }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  basic_string(std::initializer_list<_Ty> rhs, std::true_type /*fit*/)
  {
    assign(rhs, std::true_type{});
  }
  ~basic_string() { _Tidy(); }
  basic_string& operator=(const basic_string& rhs)
  {
    assign(rhs);
    return *this;
  }
  basic_string& operator=(basic_string&& rhs) YASIO__NOEXCEPT
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
  view_type view() const { return view_type{_Myfirst, static_cast<size_type>(_Mylast - _Myfirst)}; }
  basic_string& substr(size_t offset, size_t count) { return basic_string{view().substr(offset, count)}; }
  template <typename _Iter>
  void assign(const _Iter first, const _Iter last)
  {
    _Assign_range(first, last);
  }
  template <typename _Iter>
  void assign(const _Iter first, const _Iter last, std::true_type /*fit*/)
  {
    _Assign_range(first, last, std::true_type{});
  }
  void assign(const basic_string& rhs) { _Assign_range(rhs.begin(), rhs.end()); }
  void assign(const basic_string& rhs, std::true_type) { _Assign_range(rhs.begin(), rhs.end(), std::true_type{}); }
  void assign(basic_string&& rhs) { _Assign_rv(std::move(rhs)); }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  void assign(std::initializer_list<_Ty> rhs)
  {
    _Assign_range((iterator)rhs.begin(), (iterator)rhs.end());
  }
  template <typename _Ty, enable_if_t<std::is_integral<_Ty>::value, int> = 0>
  void assign(std::initializer_list<_Ty> rhs, std::true_type /*fit*/)
  {
    _Assign_range((iterator)rhs.begin(), (iterator)rhs.end(), std::true_type{});
  }
  void assign(view_type rhs) { this->assign(rhs.begin(), rhs.end()); }
  void assign(const _Elem* ntcs) { this->assign(ntcs, ntcs + _Traits::length(ntcs)); }
  void swap(basic_string& rhs) YASIO__NOEXCEPT
  {
    std::swap(_Myfirst, rhs._Myfirst);
    std::swap(_Mylast, rhs._Mylast);
    std::swap(_Myend, rhs._Myend);
  }
  basic_string& insert(size_t offset, view_type value)
  {
    insert(_Myfirst + offset, std::begin(value), std::end(value));
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
    *(_Mylast - 1) = v;
  }
  iterator erase(iterator _Where)
  {
    _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where < _Mylast, "string: out of range!");
    _Mylast = std::move(_Where + 1, _Mylast, _Where);
    return _Where;
  }
  iterator erase(iterator first, iterator last)
  {
    _YASIO_VERIFY_RANGE((first <= last) && first >= _Myfirst && last <= _Mylast, "string: out of range!");
    _Mylast = std::move(last, _Mylast, first);
    return first;
  }
  value_type& front()
  {
    _YASIO_VERIFY_RANGE(_Myfirst < _Mylast, "string: out of range!");
    return *_Myfirst;
  }
  value_type& back()
  {
    _YASIO_VERIFY_RANGE(_Myfirst < _Mylast, "string: out of range!");
    return *(_Mylast - 1);
  }
  static YASIO__CONSTEXPR size_type max_size() YASIO__NOEXCEPT { return (std::numeric_limits<ptrdiff_t>::max)(); }
  iterator begin() YASIO__NOEXCEPT { return _Myfirst; }
  iterator end() YASIO__NOEXCEPT { return _Mylast; }
  const_iterator begin() const YASIO__NOEXCEPT { return _Myfirst; }
  const_iterator end() const YASIO__NOEXCEPT { return _Mylast; }
  pointer data() YASIO__NOEXCEPT { return _Myfirst; }
  const_pointer data() const YASIO__NOEXCEPT { return _Myfirst ? _Myfirst : &_Myend; }
  const_pointer c_str() const { return this->data(); }
  size_type capacity() const YASIO__NOEXCEPT { return static_cast<size_type>(_Myend - _Myfirst); }
  size_type size() const YASIO__NOEXCEPT { return static_cast<size_type>(_Mylast - _Myfirst); }
  void clear() YASIO__NOEXCEPT { _Mylast = _Myfirst; }
  bool empty() const YASIO__NOEXCEPT { return _Mylast == _Myfirst; }

  const_reference operator[](size_type index) const { return this->at(index); }
  reference operator[](size_type index) { return this->at(index); }
  const_reference at(size_type index) const
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "string: out of range!");
    return _Myfirst[index];
  }
  reference at(size_type index)
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "string: out of range!");
    return _Myfirst[index];
  }
  template <typename _Operation>
  void resize_and_overwrite(const size_type _New_size, _Operation _Op)
  {
    _Resize_uninitialized(_New_size, _New_size);
    _Mylast = _Myfirst + std::move(_Op)(_Myfirst, _New_size);
    _Eos();
  }
  void resize(size_type new_size, const_reference val)
  {
    auto old_size = this->size();
    resize(new_size);
    if (old_size < new_size)
      memset(_Myfirst + old_size, val, new_size - old_size);
  }
  void resize(size_type new_size)
  {
    auto old_cap = this->capacity();
    if (old_cap < new_size)
      _Resize_uninitialized(new_size, _Calculate_growth(new_size));
    else
      _Mylast = _Myfirst + new_size;
  }
  void resize_fit(size_type new_size, const_reference val)
  {
    auto old_size = this->size();
    resize_fit(new_size);
    if (old_size < new_size)
      memset(_Myfirst + old_size, val, new_size - old_size);
  }
  void resize_fit(size_type new_size)
  {
    if (this->capacity() < new_size)
      _Resize_uninitialized(new_size, new_size);
    else
      _Mylast = _Myfirst + new_size;
  }
  void reserve(size_type new_cap)
  {
    if (this->capacity() < new_cap)
      _Resize_uninitialized(this->size(), new_cap);
  }
  void shrink_to_fit()
  { // reduce capacity to size, provide strong guarantee
    if (_Myfirst)
    {
      const auto _Count = this->size();
      _Resize_uninitialized(_Count, _Count);
    }
  }
  /** Release internal buffer ownership
   * Note: this is a unsafe operation, after take the internal buffer, you are responsible for
   * destroy it once you don't need it, i.e:
   *   yasio::string buf;
   *   buf.push_back('I');
   *   auto rawbufCapacity = buf.capacity();
   *   auto rawbufLen = buf.size();
   *   auto rawbuf = buf.release_pointer();
   *   // use rawbuf to do something
   *   // ...
   *   // done, destroy the memory
   *   yasio::string::allocator_type::deallocate(rawbuf, rawbufCapacity);
   *
   */
  pointer release_pointer() YASIO__NOEXCEPT
  {
    auto ptr = _Myfirst;
    _Myfirst = nullptr;
    _Mylast  = nullptr;
    _Myend   = nullptr;
    return ptr;
  }
  template <typename _Iter>
  iterator insert(iterator _Where, _Iter first, const _Iter last)
  {
    _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where <= _Mylast && first <= last, "string: out of range!");
    if (first != last)
    {
      auto count         = std::distance(first, last);
      auto insertion_pos = std::distance(_Myfirst, _Where);
      if (_Where == _Mylast)
      {
        if (count > 1)
        {
          auto old_size = _Mylast - _Myfirst;
          resize(old_size + count);
          std::copy_n(first, count, _Myfirst + old_size);
        }
        else if (count == 1)
          push_back(static_cast<value_type>(*first));
      }
      else
      {
        if (insertion_pos >= 0)
        {
          auto old_size = _Mylast - _Myfirst;
          resize(old_size + count);
          _Where       = _Myfirst + insertion_pos;
          auto move_to = _Where + count;
          std::copy_n(_Where, _Mylast - move_to, move_to);
          std::copy_n(first, count, _Where);

          _Eos();
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
    _Mylast = _Myfirst;
    if (last > first)
    {
      resize(std::distance(first, last));
      std::copy(first, last, _Myfirst);
    }
  }
  template <typename _Iter>
  void _Assign_range(_Iter first, _Iter last, std::true_type)
  {
    _Mylast = _Myfirst;
    if (last > first)
    {
      resize_fit(std::distance(first, last));
      std::copy(first, last, _Myfirst);
    }
  }
  void _Assign_rv(basic_string&& rhs)
  {
    memcpy(this, &rhs, sizeof(rhs));
    memset(&rhs, 0, sizeof(rhs));
  }
  void _Resize_uninitialized(size_type new_size, size_type new_cap)
  {
    _Myfirst = _Alloc::reallocate(_Myfirst, static_cast<size_type>(_Myend - _Myfirst), new_cap > new_size ? new_cap : new_size + 1);
    if (_Myfirst || !new_cap)
    {
      _Eos(new_size);
      _Myend = _Myfirst + new_cap;
    }
    else
      throw std::bad_alloc{};
  }
  void _Eos(size_t new_size)
  {
    _Mylast = _Myfirst + new_size;
    this->_Eos();
  }
  void _Eos() { *_Mylast = 0; }
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
  void _Tidy() YASIO__NOEXCEPT
  { // free all storage
    if (_Myfirst)
    {
      _Alloc::reallocate(_Myfirst, static_cast<size_type>(_Myend - _Myfirst), 0);
      _Myfirst = _Mylast = _Myend = nullptr;
    }
  }

  pointer _Myfirst = nullptr;
  pointer _Mylast  = nullptr;
  pointer _Myend   = nullptr;
};
using string    = basic_string<char>;
using wstring   = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
} // namespace yasio
#endif
