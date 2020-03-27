//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

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
#ifndef YASIO__FEATURE_TEST_HPP
#define YASIO__FEATURE_TEST_HPP

// Tests whether compiler has fully c++11 support
#if defined(_MSC_VER)
#  if _MSC_VER < 1900
#    define YASIO__HAS_FULL_CXX11 0
#  else
#    define YASIO__HAS_FULL_CXX11 1
#    if _MSC_VER > 1900 // VS2017 or later
#      include <vcruntime.h>
#    endif
#  endif
#else
#  define YASIO__HAS_FULL_CXX11 1
#endif

// Tests whether compiler has fully c++17 support
#if (defined(__cplusplus) && __cplusplus == 201703L) ||                                            \
    (defined(_MSC_VER) && _MSC_VER > 1900 &&                                                       \
     ((defined(_HAS_CXX17) && _HAS_CXX17 == 1) ||                                                  \
      (defined(_MSVC_LANG) && (_MSVC_LANG > 201402L))))
#  ifndef YASIO_HAS_CXX17
#    define YASIO__HAS_CXX17 1
#  endif // C++17 features macro
#endif   // C++17 features check

#if !defined(YASIO__HAS_CXX17)
#  define YASIO__HAS_CXX17 0
#endif

// Feature: inline namespace
#if YASIO__HAS_FULL_CXX11
#  define YASIO__HAS_NS_INLINE 1
#  define YASIO__NS_INLINE inline
#else
#  define YASIO__HAS_NS_INLINE 0
#  define YASIO__NS_INLINE
#endif

#endif
