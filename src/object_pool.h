// object_pool.h: a simple object pool implementation
#ifndef _OBJECT_POOL_H_
#define _OBJECT_POOL_H_
#include "politedef.h"
#include <assert.h>

#pragma warning(push)
#pragma warning(disable:4200)

namespace purelib {
namespace gc {

template<typename _Ty, size_t _ElemCount = 512>
class object_pool
{
#define FL_BEGIN(chunk) reinterpret_cast <free_link_node*>(chunk->data)

    static const size_t element_size = sz_align(sizeof(_Ty), sizeof(void*));

    typedef struct free_link_node
    {
        free_link_node* next;
    } *free_link;

    typedef struct chunk_link_node
    {
        chunk_link_node* next;
        char data[0];
    } *chunk_link;

    object_pool(const object_pool&) = delete;
    void operator= (const object_pool&) = delete;

public:
    object_pool(void) : _Myhead(nullptr), _Mychunk(nullptr), _Mycount(0)
    {
        this->_Enlarge();
    }

    ~object_pool(void)
    {
        this->purge();
    }

    void cleanup(void)
    {
        if (this->_Mychunk == nullptr) {
            return;
        }
 
        chunk_link_node* chunk = this->_Mychunk;
        free_link_node* linkend = _Tidy(chunk);

        while (chunk = chunk->next)
        {
            linkend->next = FL_BEGIN(chunk);

            linkend = _Tidy(chunk);
        }

        linkend->next = nullptr;

        this->_Myhead = FL_BEGIN(this->_Mychunk);
        this->_Mycount = 0;
    }

    void purge(void)
    {
        chunk_link_node *p, **q = &this->_Mychunk;
        while (p = *q)
        {
            *q = p->next;
            free(p);
        }
        _Myhead = nullptr;
        _Mycount = 0;
    }

    size_t count(void) const
    {
        return _Mycount;
    }

    template<typename..._Args>
    _Ty* construct(const _Args&...args)
    {
        return new (get()) _Ty(args...);
    }

    void destroy(void* _Ptr)
    {

        ((_Ty*)_Ptr)->~_Ty(); // call the destructor
        release(_Ptr);
    }

    void* get(void)
    {
        if (this->_Myhead != nullptr)
        {
            return geti();
        }
        
        _Enlarge();
        return geti();
    }

    void* geti(void)
    {
        free_link_node* ptr = this->_Myhead;
        this->_Myhead = ptr->next;
        ++_Mycount;
        return reinterpret_cast<void*>(ptr);
    }

    void release(void* _Ptr)
    {
#ifdef _DEBUG
        //::memset(_Ptr, 0x00, sizeof(_Ty));
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

        chunk_link new_chunk = (chunk_link)malloc(sizeof(chunk_link_node) + element_size * _ElemCount);
#ifdef _DEBUG
        ::memset(new_chunk, 0x00, sizeof(chunk_link_node));
#endif
        _Tidy(new_chunk)->next = nullptr;

        this->_Myhead = FL_BEGIN(new_chunk);

        new_chunk->next = this->_Mychunk;
        this->_Mychunk = new_chunk;
    }

    static free_link_node* _Tidy(chunk_link chunk)
    {
        char* rbegin = chunk->data + (_ElemCount - 1) * element_size;

        for (char* ptr = chunk->data; ptr < rbegin; ptr += element_size)
        {
            reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + element_size);
        }

        return reinterpret_cast <free_link_node*>(rbegin);
    }

private:
    free_link        _Myhead; // link to free head
    chunk_link       _Mychunk; // chunk link
    size_t           _Mycount; // allocated count 
};

// TEMPLATE CLASS object_pool_allocator, can't used by std::vector
template<class _Ty, size_t _ElemCount = SZ(8, k) / sizeof(_Ty)>
class object_pool_allocator
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
        typedef object_pool_allocator<_Other> other;
    };

    pointer address(reference _Val) const
    {	// return address of mutable _Val
        return ((pointer) &(char&)_Val);
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return ((const_pointer) &(char&)_Val);
    }

    object_pool_allocator() throw()
    {	// construct default allocator (do nothing)
    }

    object_pool_allocator(const object_pool_allocator<_Ty>&) throw()
    {	// construct by copying (do nothing)
    }

    template<class _Other>
    object_pool_allocator(const object_pool_allocator<_Other>&) throw()
    {	// construct from a related allocator (do nothing)
    }

    template<class _Other>
    object_pool_allocator<_Ty>& operator=(const object_pool_allocator<_Other>&)
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

    void construct(_Ty *_Ptr)
    {	// default construct object at _Ptr
        ::new ((void *)_Ptr) _Ty();
    }

    void construct(pointer _Ptr, const _Ty& _Val)
    {	// construct object at _Ptr with value _Val
        new (_Ptr) _Ty(_Val);
    }

#ifdef __cxx0x
    void construct(pointer _Ptr, _Ty&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std::forward<_Ty>(_Val));
    }

    template<class _Other>
    void construct(pointer _Ptr, _Other&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std::forward<_Other>(_Val));
    }

    template<class _Objty,
        class... _Types>
        void construct(_Objty *_Ptr, _Types&&... _Args)
    {	// construct _Objty(_Types...) at _Ptr
        ::new ((void *)_Ptr) _Objty(std::forward<_Types>(_Args)...);
    }
#endif

    template<class _Uty>
    void destroy(_Uty *_Ptr)
    {	// destroy object at _Ptr, do nothing
        _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        size_type _Count = (size_type)(-1) / sizeof(_Ty);
        return (0 < _Count ? _Count : 1);
    }

    // private:
    static object_pool<_Ty, _ElemCount> _Mempool;
};

template<class _Ty,
    class _Other> inline
    bool operator==(const object_pool_allocator<_Ty>&,
        const object_pool_allocator<_Other>&) throw()
{	// test for allocator equality
    return (true);
}

template<class _Ty,
    class _Other> inline
    bool operator!=(const object_pool_allocator<_Ty>& _Left,
        const object_pool_allocator<_Other>& _Right) throw()
{	// test for allocator inequality
    return (!(_Left == _Right));
}

template<class _Ty, size_t _ElemCount>
object_pool<_Ty, _ElemCount> object_pool_allocator<_Ty, _ElemCount>::_Mempool;

// stl string 
// TEMPLATE CLASS buffer_pool_allocator, can't used by std::vector
template<class _Ty, size_t _BufferSize = 128, size_t _ElemCount = SZ(512,k) / _BufferSize>
class buffer_pool_allocator
{	// generic allocator for objects of class _Ty
private:
    static std::allocator<_Ty> s_default_allocator;
public:

    typedef char buffer_type[_BufferSize];

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
        typedef buffer_pool_allocator<_Other> other;
    };

    pointer address(reference _Val) const
    {	// return address of mutable _Val
        return ((pointer) &(char&)_Val);
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return ((const_pointer) &(char&)_Val);
    }

    buffer_pool_allocator() throw()
    {	// construct default allocator (do nothing)
    }

    buffer_pool_allocator(const buffer_pool_allocator<_Ty>&) throw()
    {	// construct by copying (do nothing)
    }

    template<class _Other>
    buffer_pool_allocator(const buffer_pool_allocator<_Other>&) throw()
    {	// construct from a related allocator (do nothing)
    }

    template<class _Other>
    buffer_pool_allocator<_Ty>& operator=(const buffer_pool_allocator<_Other>&)
    {	// assign from a related allocator (do nothing)
        return (*this);
    }

    void deallocate(pointer _Ptr, size_type count)
    {	// deallocate object at _Ptr, ignore size
        auto _User_size = count * sizeof(_Ty);
        if (_User_size <= sizeof(buffer_type)) {
            _Bufferpool.release(_Ptr);
        }
        else {
            s_default_allocator.deallocate(_Ptr, count);
        }
    }

    pointer allocate(size_type count)
    {	// allocate array of _Count elements
        auto _User_size = count * sizeof(_Ty);
        if (_User_size <= sizeof(buffer_type)) {
            return static_cast<pointer>(_Bufferpool.get());
        }
        return s_default_allocator.allocate(count);
    }

    pointer allocate(size_type count, const void*)
    {	// allocate array of _Count elements, not support, such as std::vector
        return allocate(count);
    }

    void construct(_Ty *_Ptr)
    {	// default construct object at _Ptr
        ::new ((void *)_Ptr) _Ty();
    }

    void construct(pointer _Ptr, const _Ty& _Val)
    {	// construct object at _Ptr with value _Val
        new (_Ptr) _Ty(_Val);
    }

#ifdef __cxx0x
    void construct(pointer _Ptr, _Ty&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std::forward<_Ty>(_Val));
    }

    template<class _Other>
    void construct(pointer _Ptr, _Other&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std::forward<_Other>(_Val));
    }

    template<class _Objty,
        class... _Types>
        void construct(_Objty *_Ptr, _Types&&... _Args)
    {	// construct _Objty(_Types...) at _Ptr
        ::new ((void *)_Ptr) _Objty(std::forward<_Types>(_Args)...);
    }
#endif

    template<class _Uty>
    void destroy(_Uty *_Ptr)
    {	// destroy object at _Ptr
        _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        size_type _Count = (size_type)(-1) / sizeof(_Ty);
        return (0 < _Count ? _Count : 1);
    }

    // private:
    static object_pool<buffer_type, _ElemCount> _Bufferpool;
};

template<class _Ty,
    class _Other> inline
    bool operator==(const buffer_pool_allocator<_Ty>&,
        const buffer_pool_allocator<_Other>&) throw()
{	// test for allocator equality
    return (true);
}

template<class _Ty,
    class _Other> inline
    bool operator!=(const buffer_pool_allocator<_Ty>& _Left,
        const buffer_pool_allocator<_Other>& _Right) throw()
{	// test for allocator inequality
    return (!(_Left == _Right));
}

template<class _Ty, size_t _BufferSize, size_t _ElemCount>
 object_pool<typename buffer_pool_allocator<_Ty, _BufferSize, _ElemCount>::buffer_type, _ElemCount> buffer_pool_allocator<_Ty, _BufferSize, _ElemCount>::_Bufferpool;

 template<class _Ty, size_t _BufferSize, size_t _ElemCount>
 std::allocator<_Ty> buffer_pool_allocator<_Ty, _BufferSize, _ElemCount>::s_default_allocator;

}; // namespace: purelib::gc
}; // namespace: purelib

#pragma warning(pop)

#endif
/*
* Copyright (c) 2012-2017 by HALX99,  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

