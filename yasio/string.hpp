#pragma once

#include <utility>
#include <memory>
#include <iterator>
#include <limits>
#include <algorithm>
#include "yasio/buffer_alloc.hpp"
#include "yasio/string_view.hpp"

namespace yasio
{
template <typename _Elem, enable_if_t<std::is_integral<_Elem>::value && sizeof(_Elem) <= sizeof(char32_t), int> = 0>
using default_string_allocater = default_buffer_allocator<_Elem>;
template <typename _Elem, typename _SizeT = uint, typename _Alloc = default_string_allocater<_Elem>>
class basic_string {
public:
  using pointer            = _Elem*;
  using const_pointer      = const _Elem*;
  using reference          = _Elem&;
  using const_reference    = const _Elem&;
  using size_type          = _SizeT;
  using value_type         = _Elem;
  using iterator           = _Elem*; // transparent iterator
  using const_iterator     = const _Elem*;
  using allocator_type     = _Alloc;
  using traits_type        = std::char_traits<_Elem>;
  using view_type          = cxx17::basic_string_view<_Elem>;
  using my_type            = basic_string<_Elem, _SizeT, _Alloc>;
  static const size_t npos = -1;
  basic_string() {}
  basic_string(nullptr_t) = delete;
  explicit basic_string(size_type count) { resize(static_cast<size_type>(count)); }
  basic_string(size_type count, const_reference val) { resize(static_cast<size_type>(count), val); }
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  basic_string(_Iter first, _Iter last)
  {
    assign(first, last);
  }
  basic_string(const basic_string& rhs) { assign(rhs); };
  basic_string(basic_string&& rhs) YASIO__NOEXCEPT { assign(std::move(rhs)); }
  basic_string(view_type rhs) { assign(rhs); }
  basic_string(const_pointer ntcs) { assign(ntcs); }
  basic_string(const_pointer ntcs, size_type count) { assign(ntcs, ntcs + count); }
  /*basic_string(std::initializer_list<value_type> rhs) { _Assign_range(rhs.begin(), rhs.end()); }*/
  ~basic_string() { _Tidy(); }
  operator view_type() const YASIO__NOEXCEPT { return this->view(); }
  view_type view() const YASIO__NOEXCEPT { return view_type(this->c_str(), this->size()); }
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
  template <typename _Cont>
  basic_string& operator+=(const _Cont& rhs)
  {
    return this->append(std::begin(rhs), std::end(rhs));
  }
  basic_string& operator+=(const_reference rhs)
  {
    this->push_back(rhs);
    return *this;
  }
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  void assign(_Iter first, _Iter last)
  {
    _Assign_range(first, last);
  }
  void assign(view_type rhs) { _Assign_range(rhs.begin(), rhs.end()); }
  void assign(const_pointer ntcs) { this->assign(ntcs, static_cast<size_type>(traits_type::length(ntcs))); }
  void assign(const_pointer ntcs, size_type count) { _Assign_range(ntcs, ntcs + count); }
  void assign(const basic_string& rhs) { _Assign_range(rhs.begin(), rhs.end()); }
  void assign(basic_string&& rhs) { _Assign_rv(std::move(rhs)); }
  void swap(basic_string& rhs) YASIO__NOEXCEPT
  {
    std::swap(_Myfirst, rhs._Myfirst);
    std::swap(_Mysize, rhs._Mysize);
    std::swap(_Myres, rhs._Myres);
  }
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  iterator insert(iterator _Where, _Iter first, _Iter last)
  {
    auto _Mylast = _Myfirst + _Mysize;
    _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where <= _Mylast && first <= last, "basic_string: out of range!");
    if (first != last)
    {
      auto insertion_pos = static_cast<size_type>(std::distance(_Myfirst, _Where));
      if (_Where == _Mylast)
        append(first, last);
      else
      {
        auto ifirst = std::addressof(*first);
        static_assert(sizeof(*ifirst) == sizeof(value_type), "basic_string: iterator type incompatible!");
        auto count = static_cast<size_type>(std::distance(first, last));
        if (insertion_pos >= 0)
        {
          auto old_size = _Mylast - _Myfirst;
          expand(count);
          _Where       = _Myfirst + insertion_pos;
          _Mylast      = _Myfirst + _Mysize;
          auto move_to = _Where + count;
          std::copy_n(_Where, _Mylast - move_to, move_to);
          std::copy_n((iterator)ifirst, count, _Where);
        }
      }
      return _Myfirst + insertion_pos;
    }
    return _Where;
  }
  iterator insert(iterator _Where, size_type count, const_reference val)
  {
    auto _Mylast = _Myfirst + _Mysize;
    _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where <= _Mylast, "basic_string: out of range!");
    if (count)
    {
      auto insertion_pos = std::distance(_Myfirst, _Where);
      if (_Where == _Mylast)
        append(count, val);
      else
      {
        if (insertion_pos >= 0)
        {
          const auto old_size = _Mysize;
          expand(count);
          _Where       = _Myfirst + insertion_pos;
          _Mylast      = _Myfirst + _Mysize;
          auto move_to = _Where + count;
          std::copy_n(_Where, _Mylast - move_to, move_to);
          std::fill_n(_Where, count, val);
        }
      }
      return _Myfirst + insertion_pos;
    }
    return _Where;
  }
  basic_string& append(view_type value) { return this->append(value.begin(), value.end()); }
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  basic_string& append(_Iter first, const _Iter last)
  {
    if (first != last)
    {
      auto ifirst = std::addressof(*first);
      static_assert(sizeof(*ifirst) == sizeof(value_type), "basic_string: iterator type incompatible!");
      auto count = static_cast<size_type>(std::distance(first, last));
      if (count > 1)
      {
        const auto old_size = _Mysize;
        expand(count);
        std::copy_n((iterator)ifirst, count, _Myfirst + old_size);
      }
      else if (count == 1)
        push_back(static_cast<value_type>(*(iterator)ifirst));
    }
    return *this;
  }
  basic_string& append(size_type count, const_reference val)
  {
    expand(count, val);
    return *this;
  }
  void push_back(value_type&& v) { push_back(v); }
  void push_back(const value_type& v)
  {
    expand(1);
    back() = v;
  }
  iterator erase(iterator _Where)
  {
    const auto _Mylast = _Myfirst + _Mysize;
    _YASIO_VERIFY_RANGE(_Where >= _Myfirst && _Where < _Mylast, "basic_string: out of range!");
    _Mysize = static_cast<size_type>(std::move(_Where + 1, _Mylast, _Where) - _Myfirst);
    return _Where;
  }
  iterator erase(iterator first, iterator last)
  {
    const auto _Mylast = _Myfirst + _Mysize;
    _YASIO_VERIFY_RANGE((first <= last) && first >= _Myfirst && last <= _Mylast, "basic_string: out of range!");
    _Mysize = static_cast<size_type>(std::move(last, _Mylast, first) - _Myfirst);
    return first;
  }
  value_type& front()
  {
    _YASIO_VERIFY_RANGE(!empty(), "basic_string: out of range!");
    return *_Myfirst;
  }
  value_type& back()
  {
    _YASIO_VERIFY_RANGE(!empty(), "basic_string: out of range!");
    return _Myfirst[_Mysize - 1];
  }
  static YASIO__CONSTEXPR size_type max_size() YASIO__NOEXCEPT { return (std::numeric_limits<size_type>::max)() / sizeof(value_type); }

#pragma region Iterators
  iterator begin() YASIO__NOEXCEPT { return this->data(); }
  iterator end() YASIO__NOEXCEPT { return begin() + _Mysize; }
  const_iterator begin() const YASIO__NOEXCEPT { return this->data(); }
  const_iterator end() const YASIO__NOEXCEPT { return begin() + _Mysize; }
#pragma endregion

  pointer data() YASIO__NOEXCEPT { return _Myfirst ? _Myfirst : reinterpret_cast<const_pointer>(&_Myfirst); }
  const_pointer data() const YASIO__NOEXCEPT { return _Myfirst ? _Myfirst : reinterpret_cast<const_pointer>(&_Myfirst); }
  const_pointer c_str() const YASIO__NOEXCEPT { return this->data(); }
  const_reference operator[](size_type index) const { return this->at(index); }
  reference operator[](size_type index) { return this->at(index); }
  const_reference at(size_type index) const
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "basic_string: out of range!");
    return _Myfirst[index];
  }
  reference at(size_type index)
  {
    _YASIO_VERIFY_RANGE(index < this->size(), "basic_string: out of range!");
    return _Myfirst[index];
  }

#pragma region Capacity
  size_type capacity() const YASIO__NOEXCEPT { return _Myres; }
  size_type size() const YASIO__NOEXCEPT { return _Mysize; }
  size_type length() const YASIO__NOEXCEPT { return _Mysize; }
  void clear() YASIO__NOEXCEPT { _Mysize = 0; }
  bool empty() const YASIO__NOEXCEPT { return _Mysize == 0; }
  void resize(size_type new_size)
  {
    if (this->capacity() < new_size)
      _Resize_reallocate<_Reallocation_policy::_Exactly>(new_size);
    else
      _Eos(new_size);
  }
  void expand(size_type count)
  {
    const auto new_size = this->size() + count;
    if (this->capacity() < new_size)
      _Resize_reallocate<_Reallocation_policy::_At_least>(new_size);
    else
      _Eos(new_size);
  }
  void shrink_to_fit()
  { // reduce capacity to size, provide strong guarantee
    if (_Mysize != _Myres)
    { // something to do
      if (!_Mysize)
        _Tidy();
      else
        _Reallocate<_Reallocation_policy::_Exactly>(_Mysize);
    }
  }
  void reserve(size_type new_cap)
  {
    if (this->capacity() < new_cap)
      _Reallocate<_Reallocation_policy::_Exactly>(new_cap);
  }
  template <typename _Operation>
  void resize_and_overwrite(const size_type _New_size, _Operation _Op)
  {
    _Reallocate<_Reallocation_policy::_Exactly>(_New_size);
    _Eos(std::move(_Op)(_Myfirst, _New_size));
  }
#pragma endregion
  void resize(size_type new_size, const_reference val)
  {
    auto old_size = this->size();
    if (old_size != new_size)
    {
      resize(new_size);
      if (old_size < new_size)
        std::fill_n(_Myfirst + old_size, new_size - old_size, val);
    }
  }
  void expand(size_type count, const_reference val)
  {
    if (count)
    {
      auto old_size = this->size();
      expand(count);
      if (count)
        std::fill_n(_Myfirst + old_size, count, val);
    }
  }
  void reset(size_type new_size)
  {
    resize(new_size);
    memset(_Myfirst, 0x0, size_bytes());
  }
  size_t size_bytes() const YASIO__NOEXCEPT { return this->size() * sizeof(value_type); }
  template <typename _Intty>
  pointer detach_abi(_Intty& len) YASIO__NOEXCEPT
  {
    len      = static_cast<_Intty>(this->size());
    auto ptr = _Myfirst;
    _Myfirst = nullptr;
    _Mysize = _Myres = 0;
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
    _Myfirst = ptr;
    _Mysize = _Myres = len;
  }
  pointer release_pointer() YASIO__NOEXCEPT { return detach_abi(); }

#pragma region find stubs, String operations
  size_t find(value_type c, size_t pos = 0) const YASIO__NOEXCEPT { return view().find(c, pos); }
  size_t find(view_type str, size_t pos = 0) const YASIO__NOEXCEPT { return view().find(str, pos); }

  size_t rfind(value_type c, size_t pos = npos) const YASIO__NOEXCEPT { return view().rfind(c, pos); }
  size_t rfind(view_type str, size_t pos = npos) const YASIO__NOEXCEPT { return view().rfind(str, pos); }

  size_t find_first_of(value_type c, size_t pos = 0) const YASIO__NOEXCEPT { return view().find_first_of(c, pos); }
  size_t find_first_of(view_type str, size_t pos = 0) const YASIO__NOEXCEPT { return view().find_first_of(str, pos); }

  size_t find_last_of(value_type c, size_t pos = npos) const YASIO__NOEXCEPT { return view().find_last_of(c, pos); }
  size_t find_last_of(view_type str, size_t pos = npos) const YASIO__NOEXCEPT { return view().find_last_of(str, pos); }

  size_t find_first_not_of(value_type c, size_t pos = 0) const YASIO__NOEXCEPT { return view().find_first_not_of(c, pos); }
  size_t find_first_not_of(view_type str, size_t pos = 0) const YASIO__NOEXCEPT { return view().find_first_not_of(str, pos); }

  int compare(view_type str) const YASIO__NOEXCEPT { return view().compare(str); }

  my_type substr(size_t pos = 0, size_t len = npos) const { return my_type{view().substr(pos, len)}; }
#pragma endregion
  
private:
  void _Eos(size_type size) YASIO__NOEXCEPT
  {
    _Mysize           = size;
    _Myfirst[_Mysize] = static_cast<value_type>(0);
  }
  template <typename _Iter, ::yasio::enable_if_t<::yasio::is_iterator<_Iter>::value, int> = 0>
  void _Assign_range(_Iter first, _Iter last)
  {
    auto ifirst = std::addressof(*first);
    static_assert(sizeof(*ifirst) == sizeof(value_type), "basic_string: iterator type incompatible!");
    if (ifirst != _Myfirst)
    {
      _Mysize = 0;
      if (last > first)
      {
        const auto count = static_cast<size_type>(std::distance(first, last));
        resize(count);
        std::copy_n((iterator)ifirst, count, _Myfirst);
      }
    }
  }
  void _Assign_rv(basic_string&& rhs)
  {
    memcpy(this, &rhs, sizeof(rhs));
    memset(&rhs, 0, sizeof(rhs));
  }
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
    if YASIO__CONSTEXPR (_Policy == _Reallocation_policy::_Exactly)
      new_cap = size + 1;
    else
      new_cap = (std::max)(_Calculate_growth(size), size + 1);
    auto _Newvec = _Alloc::reallocate(_Myfirst, _Myres, new_cap);
    if (_Newvec)
    {
      _Myfirst = _Newvec;
      _Myres   = new_cap;
    }
    else
      throw std::bad_alloc{};
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
  void _Tidy() YASIO__NOEXCEPT
  { // free all storage
    if (_Myfirst)
    {
      _Alloc::deallocate(_Myfirst, _Myres);
      _Myfirst = nullptr;
      _Mysize = _Myres = 0;
    }
  }

  pointer _Myfirst  = nullptr;
  size_type _Mysize = 0;
  size_type _Myres  = 0;
};
using string = basic_string<char>;
#if defined(__cpp_lib_char8_t)
using u8string = basic_string<char8_t>;
#endif
using wstring   = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;
} // namespace yasio
