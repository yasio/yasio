// mempool2.h: Granularity: BOUNDARY_SIZE Geometric Progression
#ifndef _MEMPOOL2_H_
#define _MEMPOOL2_H_
#include "oslib.h"
#include <assert.h>
#include <stdint.h>
#include "oslib.h"
#include "memblock.h"

#ifndef _INLINE_ASM_BSR
#define _INLINE_ASM_BSR
#ifdef _WIN32
#pragma intrinsic(_BitScanReverse)
#define BSR(_Index, _Mask) _BitScanReverse((unsigned long*)&_Index, _Mask);
#else
#define BSR(_Index,_Mask) __asm__ __volatile__("bsrl %1, %%eax\n\tmovl %%eax, %0":"=r"(_Index):"r"(_Mask):"eax")
#endif
#endif

namespace thelib {
namespace gc {

class mempool2
{
    static const size_t MAX_INDEX = 11;
    static const size_t BOUNDARY_INDEX = 5 + sizeof(void*) / 8; // 5 at x86_32, 6 at x86_64
    static const size_t BOUNDARY_SIZE = (1 << BOUNDARY_INDEX); // 32bytes at x86_32, 64bytes at x86_64
public:
    mempool2(void);
    ~mempool2(void);
    void* allocate(size_t in_size);
    void deallocate(void* p);

private:
    void init(void);

    memblock* blocks[MAX_INDEX + 1];

public:
    // TEMPLATE CLASS object_pool_alloctor, can't used by std::vector
template<class _Ty>
class allocator
{	// generic allocator for objects of class _Ty
public:
    typedef _Ty value_type;

    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type* const_pointer;
    typedef const value_type& const_reference;

    typedef size_t size_type;
#ifdef _WIN32
    typedef ptrdiff_t difference_type;
#else
    typedef long  difference_type;
#endif

    template<class _Other>
    struct rebind
    {	// convert this type to _ALLOCATOR<_Other>
        typedef allocator<_Other> other;
    };

    pointer address(reference _Val) const
    {	// return address of mutable _Val
        return ((pointer) &(char&)_Val);
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return ((const_pointer) &(char&)_Val);
    }

    allocator() throw()
    {	// construct default allocator (do nothing)
    }

    allocator(const allocator<_Ty>&) throw()
    {	// construct by copying (do nothing)
    }

    template<class _Other>
    allocator(const allocator<_Other>&) throw()
    {	// construct from a related allocator (do nothing)
    }

    template<class _Other>
    allocator<_Ty>& operator=(const allocator<_Other>&)
    {	// assign from a related allocator (do nothing)
        return (*this);
    }

    void deallocate(pointer _Ptr, size_type)
    {	// deallocate object at _Ptr, ignore size
        global_mp.deallocte(_Ptr);
    }

    pointer allocate(size_type count)
    {	// allocate array of _Count elements
        return static_cast<pointer>( global_mp.allocate(sizeof(_Ty) * count) );
    }

    pointer allocate(size_type count, const void*)
    {	// allocate array of _Count elements, not support, such as std::vector
        return allocate(count);
    }

    void construct(pointer _Ptr, const _Ty& _Val)
    {	// construct object at _Ptr with value _Val
        new (_Ptr) _Ty(_Val);
    }

#ifdef __cxx0x
    void construct(pointer _Ptr, _Ty&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std:: forward<_Ty>(_Val));
    }

    template<class _Other>
    void construct(pointer _Ptr, _Other&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std:: forward<_Other>(_Val));
    }
#endif

    template<class _Uty>
    void destroy(_Uty *_Ptr)
    {	// destroy object at _Ptr, do nothing, because destructor will called in _Mempool.release(_Ptr)
        _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        size_type _Count = (size_type)(-1) / sizeof (_Ty);
        return (0 < _Count ? _Count : 1);
    }
}; // thelib::gc::mempool2::allocator

static mempool2 global_mp; // global mempool

typedef std::basic_string<char, std::char_traits<char>, mempool2::allocator<char>> string;
typedef std::basic_stringstream<char, std::char_traits<char>, mempool2::allocator<char>> stringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, mempool2::allocator<char>> istringstream;
typedef std::basic_ostringstream<char, std::char_traits<char>, mempool2::allocator<char>> ostringstream;

}; // thelib::gc::mempool2


}; // namespace: thelib::gc
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

