#ifndef _SUPER_CAST_H_
#define _SUPER_CAST_H_

namespace thelib {
namespace super_cast {

template <typename _To, typename _From>
_To uniform_cast(_From value)
{
    static_assert(sizeof(_From) == sizeof(_To), "invalid types for uniform_cast!");
    union
    {
        _From _from;
        _To   _to;
    } _Swaper;

    _Swaper._from = value;
    return _Swaper._to;
}

template <typename _To, typename _From>
_To force_cast(_From value)
{
    //static_assert(sizeof(_From) == sizeof(_To), "invalid types for _uni_cast!");
    union
    {
        _From _from;
        _To   _to;
    } _Swaper;

    _Swaper._from = value;
    return _Swaper._to;
}

#if !defined(_WIN64) && defined(_WIN32)
#define pointer_cast(dst, src)    \
    _asm push ebx                \
    _asm lea ebx, src            \
    _asm mov dword ptr[dst], ebx \
    _asm pop ebx
#else
//template<typename _Target, typename _Source> 
//__declspec(naked) 
//    _Target pointer_cast(_Source)
//{
//    __asm { // win64 not support inline asm code
//        mov eax, [esp + 4];
//        mov edx, [esp + 8];
//        ret;
//    }
//}
#endif

}; // namespace: simplepp_1_13_201307::super_cast
}; // namespace: simplepp_1_13_201307

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
***/
