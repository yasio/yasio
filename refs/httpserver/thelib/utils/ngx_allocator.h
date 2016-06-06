#ifndef _NGX_ALLOCATOR_H_
#define _NGX_ALLOCATOR_H_
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include "ngx_slab.h"

namespace ngx {

class mempool
{
public:
    mempool(size_t pool_size = 4096000);
    ~mempool(void);
    void* get(size_t size);
    void  release(void* pUserData);
    
private:
    ngx_slab_pool_t* slab_;
    std::mutex       mtx_;
};

extern mempool _Mempool;

// TEMPLATE CLASS object_pool_alloctor
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
        return (std::addressof(_Val));
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return (std::addressof(_Val));
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
        _Mempool.release(_Ptr);
    }

    pointer allocate(size_type _Count)
    {	// allocate array of _Count elements
        void *_Ptr = 0;

        if (_Count == 0)
            ;
        else 
            _Ptr = _Mempool.get(_Count * sizeof (_Ty));

        return (pointer)_Ptr;
    }

    pointer allocate(size_type _Count, const void*)
    {	// allocate array of _Count elements
        return (allocate(_Count));
    }

    void construct(pointer _Ptr, const _Ty& _Val)
    {	// construct object at _Ptr with value _Val
        new (_Ptr) _Ty(_Val);
    }

//#ifdef __cxx0x
    void construct(pointer _Ptr, _Ty&& _Val)
    {	// construct object at _Ptr with value _Val
        ::new ((void *)_Ptr) _Ty(_Val);
    }

    template<class _Other>
    void construct(pointer _Ptr, _Other&& _Val)
    {	// construct object at _Ptr with value _Val
        ::new ((void *)_Ptr) _Ty(_Val);
    }
//#endif
    
//#define _ALLOC_MEMBER_CONSTRUCT( \
//	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, CALL_OPT, X2, X3, X4) \
//	template<class _Objty COMMA LIST(_CLASS_TYPE)> \
//		void construct(_Objty *_Ptr COMMA LIST(_TYPE_REFREF_ARG)) \
//		{	/* construct _Objty(_Types...) at _Ptr */ \
//		::new ((void *)_Ptr) _Objty(LIST(_FORWARD_ARG)); \
//		}
//
//_VARIADIC_EXPAND_0X(_ALLOC_MEMBER_CONSTRUCT, , , , )
//#undef _ALLOC_MEMBER_CONSTRUCT

    template<class _Uty>
    void destroy(_Uty *_Ptr)
    {	// destroy object at _Ptr
        _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        return ((size_t)(-1) / sizeof (_Ty));
    }

// private:
};

template<class _Ty,
class _Other> inline
    bool operator==(const ngx::allocator<_Ty>&,
    const allocator<_Other>&) throw()
{	// test for allocator equality
    return (true);
}

template<class _Ty,
class _Other> inline
    bool operator!=(const ngx::allocator<_Ty>& _Left,
    const allocator<_Other>& _Right) throw()
{	// test for allocator inequality
    return (!(_Left == _Right));
}

//// stl typedefs

typedef std::basic_string<char, std::char_traits<char>, ngx::allocator<char> > string_type;

typedef std::basic_stringstream<char, std::char_traits<char>, ngx::allocator<char>> stringstream_type;

template<typename _Ty>
struct vector 
{
    typedef std::vector<_Ty, ngx::allocator<_Ty>> type;
};

template<typename _Kty, typename _Ty, typename _Pr = std::less<_Kty> >
struct map
{
    typedef std::map<_Kty, _Ty, _Pr, ngx::allocator<std::pair<const _Kty, _Ty> >> type;
};

};

#endif

