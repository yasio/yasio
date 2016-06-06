#ifndef _XXENDIAN_H_
#define _XXENDIAN_H_
#include "simpleppdef.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <Mswsock.h>
#include <Windows.h>
#include <Mstcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace thelib {
namespace endian {

static const uint16_t __internal_for_comfirm_byte_endian = 0x0001;
static const bool is_little_endian = *((bool*)&__internal_for_comfirm_byte_endian);

#ifdef _WIN32
/*
*
__declspec(naked) uint16_t __cdecl __bswap16(uint16_t)
{
    __asm
    {
        // 保存寄存器（若没有寄存器需要存，以下3条指令代码可省略）
        push        ebp;  
        mov         ebp, esp; 
        push        esi

        mov         eax, dword ptr[ebp + 8]; // 这里如果没有寄存保存，可直接使用esp, 注： 16汇编不支持esp作为基地址寄存器
        ror         ax, 8;

        // 恢复寄存器（若没有寄存器需要存，以下3条指令代码可省略）
        pop         esi;
        mov         esp ebp
        pop         ebp;

        ret         ;
    }
}
* 
*/
static
__declspec(naked) uint16_t __cdecl __bswap16(uint16_t)
{
    __asm
    {
        mov         eax, dword ptr[esp + 4];
        mov         ecx, eax;
        shl         eax, 8; eax = 0x00112200;
        sar         ecx, 8; ecx = 0x00000011;
        or          eax, ecx;         
        ret         ;
    }
}

static
__declspec(naked) uint32_t __cdecl __bswap32(uint32_t)
{
    __asm
    {
        mov         eax, dword ptr[esp + 4];
        mov         ecx, eax;

        ror         ecx, 8;          ecx = 0x44112233
        and         ecx, 0xFF00FF00; ecx = 0x44002200

        rol         eax, 8;          eax = 0x22334411
        and         eax, 0x00FF00FF; eax = 0x00330011

        or          eax, ecx;        eax = 0x44332211

        ret         ;
    }
}

/* 当release模式是，编译器优化代码可能会使用除了作为返回值寄存器
   (edx:eax) 之外的其他寄存器，因此在写naked内联汇编函数时，必须
   保存相关寄存器，否则会产生意想不到的后果。
   而由windows htons和htonl可知，ecx也不需要保存， 以下汇编代码
   只适用于windows 平台
*/
static
__declspec(naked) uint64_t __cdecl __bswap64(uint64_t)
{
    __asm 
    {
        mov         edx, dword ptr[esp + 4];
        mov         eax, dword ptr[esp + 8];

        ; --- swap low 4 bytes
        mov         ecx, eax;        ecx = eax = 0x11223344
        ror         ecx, 8;          ecx = 0x44112233
        and         ecx, 0xFF00FF00; ecx = 0x44002200

        rol         eax, 8;          eax = 0x22334411
        and         eax, 0x00FF00FF; eax = 0x00330011

        or          eax, ecx;        eax = 0x44332211

        ; --- swap high 4 bytes
        mov         ecx, edx;        ecx = edx = 0x55667788
        ror         ecx, 8;          ecx = 0x88556677
        and         ecx, 0xFF00FF00; ecx = 0x88006600

        rol         edx, 8;          edx = 0x66778855
        and         edx, 0x00FF00FF; edx = 0x00770055

        or          edx, ecx;        edx = 0x88776655

        ret         ;
    }
}

static
__declspec(naked) uint64_t __cdecl __pseudo_bswap64(uint64_t)
{
    _asm mov        eax, dword ptr[esp + 4];
    _asm mov        edx, dword ptr[esp + 8];
    _asm ret;
}
#else
static
uint64_t __bswap64(uint64_t value)
{
    return
        ( static_cast<uint64_t>( htonl( static_cast<uint32_t>(value) ) 
        ) << 32 )
        | 
        ( static_cast<uint64_t>( 
        ntohl( static_cast<uint32_t>(
        static_cast<uint64_t>(value) >> 32 
        ) ) ) );
}
static
uint64_t __pseudo_bswap64(uint64_t value)
{
    return value;
}
#endif

#ifdef __GNUC__
#define __BSWAP16(from,to)                                           \
{                                                                    \
  __asm__ __volatile__(                                              \
        "mov %1, %%ax\n\t"                                           \
        "ror $8, %%ax\n\t"                                           \
        "mov %%ax, %0\n\t"                                           \
        : "=r"(to)                                                   \
        : "r"(from)                                                  \
        : "cc", "memory", "ax"                                       \
         );                                                          \
}


#define __BSWAP32(from,to)                                           \
{                                                                    \
  __asm__ __volatile__(                                              \
        "movl %1, %%eax\n\t"                                         \
        "movl %%eax, %%ecx\n\t"                                      \
        "rorl $8, %%ecx\n\t"                                         \
        "andl $0xff00ff00, %%ecx\n\t"                                \
        "roll $8, %%eax\n\t"                                         \
        "andl $0x00ff00ff, %%eax\n\t"                                \
        "orl  %%ecx, %%eax\n\t"                                      \
        "movl %%eax, %0\n\t"                                         \
        : "=r"(to)                                                   \
        : "r"((uint32_t)from)                                        \
        : "cc", "memory", "eax", "ecx"                               \
         );                                                          \
}

#define __BSWAP64(from,to)                                           \
{                                                                    \
  uint32_t& l_ref = *( (uint32_t*)&to );                             \
  uint32_t& h_ref = *( (uint32_t*)&to + 1 );                         \
  __asm__ __volatile__(                                              \
        "movl %3, %%eax\n\t"                                         \
        "movl %%eax, %%ecx\n\t"                                      \
        "rorl $8, %%ecx\n\t"                                         \
        "andl $0xff00ff00, %%ecx\n\t"                                \
        "roll $8, %%eax\n\t"                                         \
        "andl $0x00ff00ff, %%eax\n\t"                                \
        "orl  %%ecx, %%eax\n\t"                                      \
        "movl %%eax, %0\n\t"                                         \
        "movl %2, %%eax\n\t"                                         \
        "movl %%eax, %%ecx\n\t"                                      \
        "rorl $8, %%ecx\n\t"                                         \
        "andl $0xff00ff00, %%ecx\n\t"                                \
        "roll $8, %%eax\n\t"                                         \
        "andl $0x00ff00ff, %%eax\n\t"                                \
        "orl  %%ecx, %%eax\n\t"                                      \
        "movl %%eax, %1\n\t"                                         \
        : "=r"(l_ref), "=r"(h_ref)                                   \
        : "r"(*( (uint32_t*)&from )), "r"(*( (uint32_t*)&from + 1 )) \
        : "cc", "memory", "eax", "ecx"                               \
         );                                                          \
}
#endif

/// atomic defs
#ifdef _MSC_VER
#define atomic_inc32(counter) _InterlockedIncrement((volatile long*)&counter)
#define atomic_dec32(counter) _InterlockedDecrement((volatile long*)&counter)
#define atomic_add32(counter,i) _InterlockedExchangeAdd((volatile long*)&counter,i)
#define atomic_cmpxchg32(dest,exchg,cmp) _InterlockedCompareExchange((volatile long*)&dest,exchg,cmp)
#endif
#ifdef __GNUC__
#define atomic_inc32(counter)     \
    __asm__ __volatile__(      \
        "lock incl %0"         \
        : "+m"(counter)           \
        )
#define atomic_dec32(vval)     \
    __asm__ __volatile__(      \
        "lock decl %0"         \
        : "+m"(vval)           \
        )
#define atomic_add32(counter,i) \
    __asm__ __volatile__(       \
        "lock addl %1,%0"       \
        :"+m" (counter)         \
        :"ir" (i))
#define atomic_cmpxchg32(dest,exchg,cmp)   \
    __asm__ __volatile__(                  \
        "movl %2, %%eax\n\t"               \
        "lock cmpxchg %1, %0"              \
        : "+m"(dest)                       \
        : "r"(exchg),"r"(cmp)              \
        : "cc", "memory", "eax", "ecx"     \
        )
#endif

/* 函数指针形式：
debug  性能略低于直接调用（每1亿次 慢100ms左右）
release模式下和直接调用相当
*/
static uint64_t (*htonll)(uint64_t);

static
void auto_resources_init(void)
{
    if(is_little_endian)
    {
        htonll = __bswap64;
    }
    else {
        htonll = __pseudo_bswap64;
    }
}

static
const int INITIALIZER_VAL0 = (auto_resources_init(), 3389);

#define ntohll htonll
#define hton ntoh

template<typename _Int> inline
_Int ntoh(_Int value)
{
    return ntohl(value);
}

template<> inline
bool ntoh(bool value)
{
    return value;
}

template<> inline
uint8_t ntoh(uint8_t value)
{
    return value;
}

template<> inline
uint16_t ntoh(uint16_t value)
{
    return htons(value);
}

template<> inline
uint32_t ntoh(uint32_t value)
{
    return htonl(value);
}

template<> inline
uint64_t ntoh(uint64_t value)
{
    return htonll(value);
}

template<> inline
int8_t ntoh(int8_t value)
{
    return value;
}

template<> inline
int16_t ntoh(int16_t value)
{
    return htons(value);
}

template<> inline
int32_t ntoh(int32_t value)
{
    return htonl(value);
}

template<> inline
int64_t ntoh(int64_t value)
{
    return htonll(value);
}

template<typename _Int> inline
char* set_value(_Int value, char* to)
{
        *( (_Int*)to ) = (value);
        return to;
}

template<typename _Int> inline
_Int& get_value(_Int& value, const char* from)
{
    return (value = ( *( (_Int*)from ) ));
}

inline
char* set_value(const char* value, char* to, size_t size)
{
    return (char*)::memcpy(to, value, size);
}

inline
char* get_value(char* value, const char* from, size_t size)
{
    return (char*)::memcpy(value, from, size);
}

// non auto step intefraces
template<typename _Int> inline
void to_netval_i(_Int value, char* const to)
{
        *( (_Int*)to ) = hton<_Int>(value);
}

template<typename _Int> inline
void to_hostval_i(_Int& value, const char* const from)
{
    value = ntoh<_Int>( *( (_Int*)from ) );
}

inline
void to_netval_i(const char* const value, char* const to, size_t size)
{
    ::memcpy(to, value, size);
}

inline
void to_hostval_i(char* const value, const char* const from, size_t size)
{
    ::memcpy(value, from, size);
}

// auto step converters
template<typename _Int> inline
char*& to_netval(_Int value, char*& to)
{
        *( (_Int*)to ) = hton<_Int>(value);
        to += (sizeof(_Int));
        return to;
}

template<typename _Int> inline
_Int& to_hostval(_Int& value, const char*& from)
{
    value = ntoh<_Int>( *( (_Int*)from ) );
    from += (sizeof(_Int));
    return value;
}

inline
char*& to_netval(const char* const value, char*& to, size_t size)
{
        ::memcpy(to, value, size);
        to += (size);
        return to;
}

inline
char* to_hostval(char* const value, const char*& from, size_t size)
{
    ::memcpy(value, from, size);
    from += (size);
    return value;
}


}; // namespace thelib::endian

}; // namespace thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

