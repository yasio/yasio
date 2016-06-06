// object_pool.h: a simple object pool implementation
#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_
#include "simpleppdef.h"
#if defined(_ENABLE_MULTITHREAD)
#include "thread_synch.h"
#endif
#include <assert.h>

namespace thelib {
#if defined(_ENABLE_MULTITHREAD)
using namespace asy;
#endif
namespace gc {

template<typename _Ty, size_t _ElemCount = 512>
class object_pool 
{
    static const size_t elem_size = sz_align(sizeof(_Ty), sizeof(void*));

    typedef struct free_link_node
    { 
        free_link_node* next; 
    } *free_link;

    typedef struct chunk_link_node
    {
        char             data[elem_size * _ElemCount];
        chunk_link_node* next;
    } *chunk_link;

    object_pool(const object_pool&);
    void operator= (const object_pool&);

public:
    object_pool(void) : _Myhead(nullptr), _Mychunk(nullptr), _Mycount(0)
    {
        this->_Enlarge();
    }

    ~object_pool(void)
    {
#if defined(_ENABLE_MULTITHREAD)
        scoped_lock<thread_mutex> guard(this->_Mylock);
#endif

        this->purge();
    }

    void cleanup(void)
    {
        if(this->_Mychunk = nullptr) {
            return;
        }

        free_link_node* prev = nullptr;
        chunk_link_node* chunk = this->_Mychunk;
        for (; chunk != nullptr; chunk = chunk->next) 
        {
            char* begin     = chunk->data; 
            char* rbegin    = begin + (_ElemCount - 1) * elem_size; 

            if(prev != nullptr)
                prev->next = reinterpret_cast<free_link>(begin);
            for (char* ptr = begin; ptr < rbegin; ptr += elem_size )
            { 
                reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + elem_size);
            } 

            prev = reinterpret_cast <free_link_node*>(rbegin); 
        } 

        this->_Myhead = reinterpret_cast<free_link_node*>(this->_Mychunk->data);
        this->_Mycount = 0;
    }

    void purge(void)
    {
        chunk_link_node* ptr = this->_Mychunk;
        while (ptr != nullptr) 
        {
            chunk_link_node* deleting = ptr;
            ptr = ptr->next;
            free(deleting);
        }
        _Myhead = nullptr;
        _Mychunk = nullptr;
        _Mycount = 0;
    }

    size_t count(void) const
    {
        return _Mycount;
    }

    // if the type is not pod, you may be use placement new to call the constructor,
    // for example: _Ty* obj = new(pool.get()) _Ty(arg1,arg2,...);
    void* get(void) 
    {
#if defined(_ENABLE_MULTITHREAD)
        scoped_lock<thread_mutex>  guard(this->_Mylock);
#endif

        if (nullptr == this->_Myhead) 
        { 
            this->_Enlarge(); 
        }
        free_link_node* ptr = this->_Myhead;
        this->_Myhead = ptr->next;
        ++_Mycount;
        return reinterpret_cast<void*>(ptr);
    }

    void release(void* _Ptr)
    {
        ( (_Ty*)_Ptr)->~_Ty(); // call the destructor
#ifdef _DEBUG
        //::memset(_Ptr, 0x00, sizeof(_Ty));
#endif
#if defined(_ENABLE_MULTITHREAD)
        scoped_lock<thread_mutex>  guard(this->_Mylock);
#endif

        free_link_node* ptr = reinterpret_cast<free_link_node*>(_Ptr);
        ptr->next = this->_Myhead;
        this->_Myhead = ptr;
        --_Mycount;
    }

private:
    void _Enlarge(void)
    {
        static_assert(_ElemCount > 0, "Invalid Element Count");

        chunk_link new_chunk  = (chunk_link)malloc(sizeof(chunk_link_node)); 
#ifdef _DEBUG
        ::memset(new_chunk, 0x00, sizeof(chunk_link_node));
#endif
        new_chunk->next = this->_Mychunk; 
        this->_Mychunk  = new_chunk; 

        char* begin     = this->_Mychunk->data; 
        char* rbegin    = begin + (_ElemCount - 1) * elem_size; 

        for (char* ptr = begin; ptr < rbegin; ptr += elem_size )
        { 
            reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + elem_size);
        } 

        reinterpret_cast <free_link_node*>(rbegin)->next = nullptr; 
        this->_Myhead = reinterpret_cast<free_link_node*>(begin); 
    }

private:
    free_link        _Myhead; // link to free head
    chunk_link       _Mychunk; // chunk link
#if defined(_ENABLE_MULTITHREAD)
    thread_mutex     _Mylock;  // thread mutex
#endif
    size_t           _Mycount; // allocated count 
};

// TEMPLATE CLASS object_pool_alloctor, can't used by std::vector
template<class _Ty, size_t _ElemCount = SZ(64,K) / sizeof(_Ty)>
class object_pool_alloctor
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
        typedef object_pool_alloctor<_Other> other;
    };

    pointer address(reference _Val) const
    {	// return address of mutable _Val
        return ((pointer) &(char&)_Val);
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return ((const_pointer) &(char&)_Val);
    }

    object_pool_alloctor() throw()
    {	// construct default allocator (do nothing)
    }

    object_pool_alloctor(const object_pool_alloctor<_Ty>&) throw()
    {	// construct by copying (do nothing)
    }

    template<class _Other>
    object_pool_alloctor(const object_pool_alloctor<_Other>&) throw()
    {	// construct from a related allocator (do nothing)
    }

    template<class _Other>
    object_pool_alloctor<_Ty>& operator=(const object_pool_alloctor<_Other>&)
    {	// assign from a related allocator (do nothing)
        return (*this);
    }

    void deallocate(pointer _Ptr, size_type)
    {	// deallocate object at _Ptr, ignore size
        _Mempool.release(_Ptr);
    }

    pointer allocate(size_type count)
    {	// allocate array of _Count elements
        assert(count == 1);
        return static_cast<pointer>(_Mempool.get());
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
        // _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        size_type _Count = (size_type)(-1) / sizeof (_Ty);
        return (0 < _Count ? _Count : 1);
    }

// private:
    static object_pool<_Ty, _ElemCount> _Mempool;
};

template<class _Ty, size_t _ElemCount>
object_pool<_Ty, _ElemCount> object_pool_alloctor<_Ty, _ElemCount>::_Mempool;

}; // namespace: thelib::gc
}; // namespace: thelib


#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

