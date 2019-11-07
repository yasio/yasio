//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

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

#ifndef YASIO__CONFIG_HPP
#define YASIO__CONFIG_HPP

/*
** Uncomment or add compiler flag -DYASIO_HEADER_ONLY to enable yasio core implementation header
** only
*/
// #define YASIO_HEADER_ONLY 1

/*
** Uncomment or add compiler flag -DYASIO_VERBOS_LOG to enable verbos log
*/
// #define YASIO_VERBOS_LOG 1

/*
** Uncomment or add compiler flag -DYASIO_DISABLE_SPSC_QUEUE to disable SPSC queue in io_service
** Remark: Most of time, this library may used in game engines:
**      1. send message: post send request at renderer thread, and perform send at `yasio_evloop`
**         thread.
**      2. receive message: unpack message at `yasio_evloop` thread, consume message at renderer
**         thread.
**      3. If you want use this library in other situation, you may need uncomment it.
*/
// #define YASIO_DISABLE_SPSC_QUEUE 1

/*
** Uncomment or add compiler flag -DYASIO_DISABLE_OBJECT_POOL to disable object_pool for allocating
** protocol data unit
*/
// #define YASIO_DISABLE_OBJECT_POOL 1

/*
** Uncomment or add compiler flag -DYASIO_HAVE_KCP for kcp support
** Remember, before thus, please ensure:
** 1. Execute: `git submodule update --init --recursive` to clone the kcp sources.
** 2. Add yasio/kcp/ikcp.c to your build system, even through the `YASIO_HEADER_ONLY` was defined.
** pitfall: yasio kcp support is experimental currently.
*/
// #define YASIO_HAVE_KCP 1

#if defined(YASIO_HEADER_ONLY)
#  define YASIO__DECL inline
#else
#  define YASIO__DECL
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800
#  define YASIO__HAVE_NS_INLINE 0
#  define YASIO__NS_INLINE
#else
#  define YASIO__HAVE_NS_INLINE 1
#  define YASIO__NS_INLINE inline
#endif

#if defined(_WIN32)
#  define YASIO_LOG(format, ...)                                                                   \
    OutputDebugStringA(::yasio::strfmt(127, ("%s" format "\n"), "[yasio]", ##__VA_ARGS__).c_str())
#elif defined(ANDROID) || defined(__ANDROID__)
#  include <android/log.h>
#  include <jni.h>
#  define YASIO_LOG(format, ...)                                                                   \
    __android_log_print(ANDROID_LOG_INFO, "yasio", ("%s" format), "[yasio]", ##__VA_ARGS__)
#else
#  define YASIO_LOG(format, ...) printf(("%s" format "\n"), "[yasio]", ##__VA_ARGS__)
#endif

#if !defined(YASIO_VERBOS_LOG)
#  define YASIO_LOGV(fmt, ...) (void)0
#else
#  define YASIO_LOGV YASIO_LOG
#endif

#define YASIO_INET_BUFFER_SIZE 65536

#define YASIO_ARRAYSIZE(A) (sizeof(A) / sizeof((A)[0]))

/*
** YASIO_OBSOLETE_DEPRECATE
*/
#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#  define YASIO_OBSOLETE_DEPRECATE(_Replacement) __attribute__((deprecated))
#elif _MSC_VER >= 1400 // vs 2005 or higher
#  define YASIO_OBSOLETE_DEPRECATE(_Replacement)                                                   \
    __declspec(deprecated(                                                                         \
        "This function will be removed in the future. Consider using " #_Replacement " instead."))
#else
#  define YASIO_OBSOLETE_DEPRECATE(_Replacement)
#endif

#include "strfmt.hpp"

#endif
