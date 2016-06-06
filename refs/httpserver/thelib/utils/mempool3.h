// mempool3.h: Granularity: BOUNDARY_SIZE * n
#ifndef _MEMPOOL3_H_
#define _MEMPOOL3_H_

#include <assert.h>
#include <stdint.h>
#include "memblock.h"

namespace thelib {
namespace gc {

class mempool3
{
    static const size_t MAX_INDEX = 39;
    static const size_t BOUNDARY_INDEX = 7; 
    static const size_t BOUNDARY_SIZE = (1 << BOUNDARY_INDEX);
public:
    mempool3(void);
    ~mempool3(void);
    void* allocate(size_t in_size);
    void deallocate(void* p);

private:
    void init(void);

    /**
     * Lists of free nodes.
     * and the slots 0..MAX_INDEX contain nodes of sizes
     * (i + 1) * BOUNDARY_SIZE. Example for BOUNDARY_INDEX == 12:
     * slot  0: size 128
     * slot  1: size 256
     * slot  2: size 384
     * ...
     * slot 19: size 5120
     */
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
}; // thelib::gc::mempool3::allocator
   
static mempool3 global_mp; // global mempool
typedef std::basic_string<char, std::char_traits<char>, mempool3::allocator<char>> string;
typedef std::basic_stringstream<char, std::char_traits<char>, mempool3::allocator<char>> stringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, mempool3::allocator<char>> istringstream;
typedef std::basic_ostringstream<char, std::char_traits<char>, mempool3::allocator<char>> ostringstream;

}; // namespace: thelib::gc::mempool3

}; // namespace: thelib::gc
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

