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
*/
#ifndef YASIO__MEMORY
#define YASIO__MEMORY
#include <memory>

#include "yasio/compiler/feature_test.hpp"

/// The make_unique workaround on c++11
#if !YASIO__HAS_CXX14
namespace cxx14
{
template <typename _Ty, typename... _Args>
std::unique_ptr<_Ty> make_unique(_Args&&... args)
{
  return std::unique_ptr<_Ty>(new _Ty(std::forward<_Args>(args)...));
}
} // namespace cxx14
#endif

namespace yasio
{
template <typename _Ty, bool = true>
struct construct_helper {
  template <typename... Args>
  static _Ty* construct_at(_Ty* p, Args&&... args)
  {
    return ::new (static_cast<void*>(p)) _Ty(std::forward<Args>(args)...);
  }
};
template <typename _Ty>
struct construct_helper<_Ty, false> {
  template <typename... Args>
  static _Ty* construct_at(_Ty* p, Args&&... args)
  {
    return ::new (static_cast<void*>(p)) _Ty{std::forward<Args>(args)...};
  }
};

template <typename _Ty, typename... Args>
inline _Ty* construct_at(_Ty* p, Args&&... args)
{
  return construct_helper<_Ty, std::is_constructible<_Ty, Args&&...>::value>::construct_at(p, std::forward<Args>(args)...);
}

/** compressed_pair
 * stores two objects, but applies Empty Base Optimization(EBO) when one of them is an empty type.
 * This reduces memory overhead.
 */
template <class _Ty1, class _Ty2, bool = std::is_empty_v<_Ty1> && !std::is_final_v<_Ty1>, bool = std::is_empty_v<_Ty2> && !std::is_final_v<_Ty2>>
class compressed_pair;

// Case 1: neither empty
template <class _Ty1, class _Ty2>
class compressed_pair<_Ty1, _Ty2, false, false> {
  _Ty1 _Myval1;
  _Ty2 _Myval2;

public:
  // constructors
  constexpr compressed_pair() = default;

  constexpr compressed_pair(const _Ty1& v1, const _Ty2& v2) noexcept(std::is_nothrow_copy_constructible_v<_Ty1> && std::is_nothrow_copy_constructible_v<_Ty2>)
      : _Myval1(v1), _Myval2(v2)
  {}

  constexpr compressed_pair(_Ty1&& v1, _Ty2&& v2) noexcept(std::is_nothrow_move_constructible_v<_Ty1> && std::is_nothrow_move_constructible_v<_Ty2>)
      : _Myval1(std::move(v1)), _Myval2(std::move(v2))
  {}

  template <class U1, class U2>
  constexpr compressed_pair(U1&& v1, U2&& v2) noexcept(std::is_nothrow_constructible_v<_Ty1, U1&&> && std::is_nothrow_constructible_v<_Ty2, U2&&>)
      : _Myval1(std::forward<U1>(v1)), _Myval2(std::forward<U2>(v2))
  {}

  // accessors
  constexpr _Ty1& first() noexcept { return _Myval1; }
  constexpr const _Ty1& first() const noexcept { return _Myval1; }
  constexpr _Ty2& second() noexcept { return _Myval2; }
  constexpr const _Ty2& second() const noexcept { return _Myval2; }
};

// Case 2: First empty
template <class _Ty1, class _Ty2>
class compressed_pair<_Ty1, _Ty2, true, false> : private _Ty1 {
  _Ty2 _Myval2;

public:
  constexpr compressed_pair() = default;

  constexpr compressed_pair(const _Ty1& v1, const _Ty2& v2) noexcept(std::is_nothrow_copy_constructible_v<_Ty2>) : _Ty1(v1), _Myval2(v2) {}

  constexpr compressed_pair(_Ty1&& v1, _Ty2&& v2) noexcept(std::is_nothrow_move_constructible_v<_Ty2>) : _Ty1(std::move(v1)), _Myval2(std::move(v2)) {}

  template <class U1, class U2>
  constexpr compressed_pair(U1&& v1, U2&& v2) noexcept(std::is_nothrow_constructible_v<_Ty2, U2&&>) : _Ty1(std::forward<U1>(v1)), _Myval2(std::forward<U2>(v2))
  {}

  constexpr _Ty1& first() noexcept { return *this; }
  constexpr const _Ty1& first() const noexcept { return *this; }
  constexpr _Ty2& second() noexcept { return _Myval2; }
  constexpr const _Ty2& second() const noexcept { return _Myval2; }
};

// Case 3: Second empty
template <class _Ty1, class _Ty2>
class compressed_pair<_Ty1, _Ty2, false, true> : private _Ty2 {
  _Ty1 _Myval1;

public:
  constexpr compressed_pair() = default;

  constexpr compressed_pair(const _Ty1& v1, const _Ty2& v2) noexcept(std::is_nothrow_copy_constructible_v<_Ty1>) : _Myval1(v1), _Ty2(v2) {}

  constexpr compressed_pair(_Ty1&& v1, _Ty2&& v2) noexcept(std::is_nothrow_move_constructible_v<_Ty1>) : _Myval1(std::move(v1)), _Ty2(std::move(v2)) {}

  template <class U1, class U2>
  constexpr compressed_pair(U1&& v1, U2&& v2) noexcept(std::is_nothrow_constructible_v<_Ty1, U1&&>) : _Myval1(std::forward<U1>(v1)), _Ty2(std::forward<U2>(v2))
  {}

  constexpr _Ty1& first() noexcept { return _Myval1; }
  constexpr const _Ty1& first() const noexcept { return _Myval1; }
  constexpr _Ty2& second() noexcept { return *this; }
  constexpr const _Ty2& second() const noexcept { return *this; }
};

// Case 4: both empty
template <class _Ty1, class _Ty2>
class compressed_pair<_Ty1, _Ty2, true, true> : private _Ty1, private _Ty2 {
public:
  constexpr compressed_pair() = default;

  constexpr compressed_pair(const _Ty1& v1, const _Ty2& v2) : _Ty1(v1), _Ty2(v2) {}

  constexpr compressed_pair(_Ty1&& v1, _Ty2&& v2) : _Ty1(std::move(v1)), _Ty2(std::move(v2)) {}

  template <class U1, class U2>
  constexpr compressed_pair(U1&& v1, U2&& v2) : _Ty1(std::forward<U1>(v1)), _Ty2(std::forward<U2>(v2))
  {}

  constexpr _Ty1& first() noexcept { return *this; }
  constexpr const _Ty1& first() const noexcept { return *this; }
  constexpr _Ty2& second() noexcept { return *this; }
  constexpr const _Ty2& second() const noexcept { return *this; }
};
} // namespace yasio

#define _YASIO_VERIFY_RANGE(cond, mesg)                 \
  do                                                    \
  {                                                     \
    if (cond)                                           \
      ; /* contextually convertible to bool paranoia */ \
    else                                                \
    {                                                   \
      throw std::out_of_range(mesg);                    \
    }                                                   \
                                                        \
  } while (false)

#endif
