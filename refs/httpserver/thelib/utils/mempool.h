#ifndef _MEMPOOL_H_
#define _MEMPOOL_H_
#include "simpleppdef.h"
#include <cstddef>
#include <iostream>
#include <mutex>

namespace thelib {
namespace gc {

class mempool
{
struct memblock_head_t
{
    memblock_head_t* prev;
    memblock_head_t* next;
    size_t           size;
    bool             unused;
}; 

struct memblock_foot_t
{
    memblock_head_t* uplink;
}; 

static const size_t reserved_size = sizeof(memblock_head_t) + sizeof(memblock_foot_t);
static const size_t min_size = sizeof(void*) * 2;
static const size_t min_block_size = reserved_size + min_size;

#define FOOT_LOC(h) ((memblock_foot_t*)( (char*)h + h->size - sizeof(memblock_foot_t) ) )
#define HEAD_LOC(pu) ( (memblock_head_t*)( (char*)pu - sizeof(memblock_head_t) ) )
#define UPDATE_FOOT(h) ( FOOT_LOC(h)->uplink = h )

#define LEFT_FOOT_LOC(h) ( (memblock_foot_t*)( (char*)h - sizeof(memblock_foot_t) ) )
#define RIGHT_HEAD_LOC(h) ( (memblock_head_t*)( (char*)h + h->size ) )

#define REMAIN_SIZE(blk,n) (blk->size - reserved_size - (n) ) // calculate remain size after n bytes allocated.


public:
    mempool(size_t size = SZ(64, k));

    ~mempool(void);

    void init(size_t size);

    void* allocate(size_t size);

    void deallocte(void* pUserData);

    bool belong_to(void* p);

    bool is_valid_leftf(void* lf);

    bool is_valid_righth(void* rh);

private:
    void*            memory_;
    size_t           memory_size_; // memory total size;
    memblock_head_t* freelist_;
#ifdef EANBLE_MULTI_THREAD
    std::mutex       mtx_;
#endif

public: /// alocator
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
}; /* thelib::mempool::allocator */


static mempool global_mp; // global mempool

typedef std::basic_string<char, std::char_traits<char>, mempool::allocator<char>> string;
typedef std::basic_stringstream<char, std::char_traits<char>, mempool::allocator<char>> stringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, mempool::allocator<char>> istringstream;
typedef std::basic_ostringstream<char, std::char_traits<char>, mempool::allocator<char>> ostringstream;

}; /* thelib::mempool */
}; /* thelib::gc */
}; /* thelib */

#endif

