#ifndef _XXBSWAP_H_
#define _XXBSWAP_H_

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
inline
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

inline
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
inline
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

#else

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
        : "cc", "memory"                               \
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

inline uint16_t  __bswap16(uint16_t fromval)
{
    uint16_t toval = 0;
    __BSWAP16(fromval, toval);
    return toval;
}

inline uint32_t  __bswap32(uint32_t fromval)
{
    uint32_t toval = 0;
    __BSWAP32(fromval, toval);
    return toval;
}

inline uint64_t  __bswap64(uint64_t fromval)
{
    uint64_t toval = 0;
    __BSWAP64(fromval, toval);
    return toval;
}

#endif

#endif
/*
* Copyright (c) 2012-2014 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

