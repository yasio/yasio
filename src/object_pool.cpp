// object_pool.cpp: a simple & high-performance object pool implementation
#if !defined(_OBJECT_POOL_CPP_)
#define _OBJECT_POOL_CPP_

#if !defined(OBJECT_POOL_HEADER_ONLY)
#include "object_pool.h"
#endif

#define OBJECT_POOL_PREALLOCATE 1

namespace purelib {
namespace gc {

namespace detail {

#define POOL_FL_BEGIN(chunk) reinterpret_cast <free_link_node*>(chunk->data)

object_pool::object_pool(size_t element_size, size_t element_count) : free_link_(nullptr)
    ,chunk_(nullptr)
    ,element_size_(element_size)
    ,element_count_(element_count)
#ifdef _DEBUG
    ,allocated_count_(0)
#endif
{
#if OBJECT_POOL_PREALLOCATE
    release(allocate_from_process_heap()); // preallocate 1 chunk
#endif 
}

object_pool::~object_pool(void)
{
    this->purge();
}

void object_pool::cleanup(void)
{
    if (this->chunk_ == nullptr) {
        return;
    }

    chunk_link_node* chunk = this->chunk_;
    free_link_node* linkend = this->tidy_chunk(chunk);

    while ((chunk = chunk->next) != nullptr)
    {
        linkend->next = POOL_FL_BEGIN(chunk);

        linkend = this->tidy_chunk(chunk);
    }

    linkend->next = nullptr;

    this->free_link_ = POOL_FL_BEGIN(this->chunk_);

#if defined(_DEBUG)
    this->allocated_count_ = 0;
#endif
}

void object_pool::purge(void)
{
    if (this->chunk_ == nullptr)
        return;
    
    chunk_link_node *p, **q = &this->chunk_;
    while ((p = *q) != nullptr)
    {
        *q = p->next;
        free(p);
    }

    free_link_ = nullptr;

#if defined(_DEBUG)
    allocated_count_ = 0;
#endif
}

void* object_pool::get(void)
{
    if (this->free_link_ != nullptr)
    {
        return allocate_from_chunk();
    }

    return allocate_from_process_heap();
}

void object_pool::release(void* _Ptr)
{
    free_link_node* ptr = reinterpret_cast<free_link_node*>(_Ptr);
    ptr->next = this->free_link_;
    this->free_link_ = ptr;

#if defined(_DEBUG)
    --this->allocated_count_;
#endif
}

void* object_pool::allocate_from_process_heap(void)
{
    chunk_link new_chunk = (chunk_link)malloc(sizeof(chunk_link_node) + element_size_ * element_count_);
#ifdef _DEBUG
    ::memset(new_chunk, 0x00, sizeof(chunk_link_node));
#endif
    tidy_chunk(new_chunk)->next = nullptr;

    // link the new_chunk
    new_chunk->next = this->chunk_;
    this->chunk_ = new_chunk;

    // allocate 1 object
    auto ptr = POOL_FL_BEGIN(new_chunk);
    this->free_link_ = ptr->next;

#if defined(_DEBUG)
    ++this->allocated_count_;
#endif
    return reinterpret_cast<void*>(ptr);
}

void* object_pool::allocate_from_chunk(void)
{
    free_link_node* ptr = this->free_link_;
    this->free_link_ = ptr->next;
#if defined(_DEBUG)
    ++this->allocated_count_;
#endif
    return reinterpret_cast<void*>(ptr);
}

object_pool::free_link_node* object_pool::tidy_chunk(chunk_link chunk)
{
    char* rbegin = chunk->data + (element_count_ - 1) * element_size_;

    for (char* ptr = chunk->data; ptr < rbegin; ptr += element_size_)
    {
        reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + element_size_);
    }
    return reinterpret_cast <free_link_node*>(rbegin);
}

} // purelib::gc::detail
} // namespace purelib::gc
}; // namespace purelib

#endif // _OBJECT_POOL_CPP_
/*
* Copyright (c) 2012-2017 by HALX99,  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
