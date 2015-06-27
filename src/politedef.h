/****************************************************************************
*                                                                           *
* politedef.h -- Basic politeness library Definitions                       *
*                                                                           *
* Copyright (c) X.D. Guo. All rights reserved.                              *
*                                                                           *
****************************************************************************/

#ifndef _POLITEDEF_H_
#define _POLITEDEF_H_

#ifndef _POLITE_VERSION
#define _POLITE_VERSION "1.0.1"
#endif

#if defined(_MSC_VER)
#    if _MSC_VER >= 1600
#        define __cpp0x 1
#    endif
#else // For __GNUC__
#    if __cplusplus >= 201103L
#        define __cpp0x 1
#    endif
#endif

#ifdef __cpp0x
#    define __cpp11       __cpp0x
#    define __cxx0x       __cpp11
#    define __cxx11       __cxx0x
#endif

#ifndef __cxx11
#  define nullptr         0
#endif

/*
 * _POLAPI_ Definitions
 */
#ifndef POLITEAPI
#define POLITEAPI __stdcall
#endif

/*
 * _POLITE_BASETYPES_ Definitions
 */

#ifndef _POLITE_BASETYPES
#define _POLITE_BASETYPES
#ifndef INTTYPES_DEFINED
#define INTTYPES_DEFINED
#define u_int uint32_t
typedef unsigned long u_long;
typedef long long llong;
typedef unsigned long long u_llong;
#if /*defined(__cxx11) || */defined(__GNUC__)
#include <stdint.h>
#else
typedef signed __int8 int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#endif
typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef uint32_t dword_t;
typedef uint64_t qword_t;
#endif
#endif  /* _POLITE_BASETYPES */

typedef int _initialization_t;

/*
 * _POL_CONSTANTS_ Definitions
 */
#ifndef _POLITE_CONSTANTS
#define _POLITE_CONSTANTS
static const uint64_t __B__ =  (1);
static const uint64_t __KB__ = (1024 * __B__) ;
static const uint64_t __MB__ = (1024 * __KB__);
static const uint64_t __GB__ = (1024 * __MB__);
static const uint64_t __TB__ = (1024 * __GB__);
static const uint64_t __PB__ = (1024 * __TB__);
static const uint64_t __EB__ = (1024 * __PB__);
#endif  /* _POL_CONSTANTS */
#include <memory>
/*
 * TEMPLATE TYPE structs Definition
 */
#ifndef _POLITE_BOOL
#define _POLITE_BOOL
template<typename _Int>
struct Bool
{
    typedef _Int type;
    static const type True = ~0;
    static const type False = 0;
} ;
#endif /* _POLITE_BOOL */

/*
 * size in bytes constant: SZ Definition
 */
#ifndef _POLITE_SZ
#define _POLITE_SZ
#define __b__ __B__
#define __k__ __K__
#define __m__ __M__
#define __g__ __G__
#define __t__ __T__
#define __e__ __E__
#define __K__ __KB__
#define __M__ __MB__
#define __G__ __GB__
#define __T__ __TB__
#define __P__ __PB__
#define __E__ __EB__
#define SZ(n, u) ( (n) * __##u##__ )
static const uint8_t __MAX_UINT8 = SZ(256, B) - 1;
static const uint16_t __MAX_UINT16 = SZ(64, KB) - 1;
static const uint32_t __MAX_UINT32 = SZ(4, GB) - 1;
static const uint64_t __MAX_UINT64 = SZ(15, EB) + (SZ(1, EB) - 1);
#endif   /* _POLITE_SZ */

/*
* Polite Micro _towcb Definition
*/
#ifndef _towcb
#define __towcb(quote) L##quote
#define _towcb(quote) __towcb(quote)
#endif /* _towcb */

#ifndef _POL_RDTSC
#define _POL_RDTSC
#if defined(__GNUC__)
#define _naked_mark
static __inline int64_t rdtsc(void)
{
    int64_t ts;
    uint32_t ts1, ts2;
    //__asm__ __volatile__("rdtsc\n\t":"=a"(ts1), "=d"(ts2));
    ts = ((uint64_t) ts2 << 32) | ((uint64_t) ts1);
    return ts;
}
#elif !defined(_WIN64) /* win32 */
#define _naked_mark __declspec(naked)
_naked_mark
static inline int64_t rdtsc(void)
{
    __asm
    {
        rdtsc;
        ret;
    }
}
#else
#define _naked_mark
#endif
#endif  /* _POL_RDTSC_ */

#ifndef _ISNULLPTR
#define _ISNULLPTR
#define _IsNull(ptr) ( nullptr == (ptr) )
#define _IsNotNull(ptr) ( (ptr) != nullptr )
#endif /* _ISNULLPTR */

#ifndef _POLITE_STRING
#define _POLITE_STRING
#include <string>
namespace purelib {
#ifdef _UNICODE
    typedef std::wstring string;
#else
    typedef std::string string;
#endif
};
#endif /* _POLITE_STRING */

#ifndef _POLITE_COMPARE
#define _POLITE_COMPARE
#define _EQU(a,b) ( (a) == (b) )
#define _NEQ(a,b) ( (a) != (b) )
#define _LSS(a,b) ( (a) < (b) )
#define _LEQ(a,b) ( (a) <= (b) )
#define _GTR(a,b) ( (a) > (b) )
#define _GEQ(a,b) ( (a) >= (b) )
#endif /* _POLITE_STRING */

#ifndef _POLITE_ALIAS
#define _POLITE_ALIAS
#define NS_PURELIB_BEGIN namespace purelib {
#define NS_PURELIB_END }
#define USING_NS_PURELIB using namespace purelib

namespace nx = ::purelib;
namespace thelib = ::purelib;
namespace polite = ::purelib;
#endif /* _POLITE_ALIAS */

#ifndef _POLITE_LOG
#define _POLITE_LOG
#define log_error(format,...) fprintf(stderr,(format),##__VA_ARGS__)
#define log_trace(format,...) fprintf(stdout,(format),##__VA_ARGS__)
#endif

#ifndef _POLITE_SZ_ALIGN
#define _POLITE_SZ_ALIGN
#define sz_align(d,a) (((d) + ((a) - 1)) & ~((a) - 1))  
// #define sz_align(x,n) ( (x) + ( (n) - (x) % (n) ) - ( ( (x) % (n) == 0 ) * (n) ) )
#endif

#define _FORMAT_CTL(width,delim) std::setw(width) << std::setfill(delim) << std::setiosflags(std::ios_base::left)
#define _FORMAT_STRING(width,tip,value,delim) _FORMAT_CTL(width, delim) << std::string(tip) + " " << " " << value
#define _FORMAT_STRING_L(width,tip,value,len,delim) _FORMAT_CTL(width, delim) << std::string(tip) + " " << " " << std::string(value,len) << "\n"
#define strfmt(tip,value,delim) _FORMAT_STRING(23, tip, value, delim)
#define strnfmt(tip,value,len,delim) _FORMAT_STRING_L(23, tip, value, len, delim)

#define enable_leak_check() _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF )

#ifndef _POLITE_UNUSED
#define _POLITE_UNUSED
#define PLIB_UNUSED(expression) (void)(expression)
#define PLIB_UNUSED_PARAM(param)
#endif

#ifndef _POLITE_UNREACHABLE
#define _POLITE_UNREACHABLE
#define PLIB_UNREACHABLE assert(false)
#endif

#define PLIB_STATICIZE_CLASS(className) private: \
	className(void); \
	~className(void); public:

#if _MSC_VER >= 1700
#define _HAS_STD_THREAD 1
#endif

#endif /* _POLITEDEF_H_ */
/*
* Copyright (c) 2012-2014 by X.D. Guo ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
***/

