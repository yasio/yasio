//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
// object_pool.cpp: a simple & high-performance object pool implementation v1.3

#ifndef YASIO__OBJECT_POOL_CPP
#define YASIO__OBJECT_POOL_CPP

#if !defined(YASIO_OBJECT_POOL_HEADER_ONLY)
#  include "object_pool.h"
#endif

#define OBJECT_POOL_PREALLOCATE 1

namespace yasio
{
namespace gc
{

namespace detail
{

#define POOL_FL_BEGIN(chunk) reinterpret_cast<free_link_node*>(chunk->data)

object_pool::object_pool(size_t element_size, size_t element_count)
    : free_link_(nullptr), chunk_(nullptr), element_size_(element_size),
      element_count_(element_count)
#ifdef _DEBUG
      ,
      allocated_count_(0)
#endif
{
#if OBJECT_POOL_PREALLOCATE
  release(allocate_from_process_heap()); // preallocate 1 chunk
#endif
}

object_pool::~object_pool(void) { this->purge(); }

void object_pool::cleanup(void)
{
  if (this->chunk_ == nullptr)
  {
    return;
  }

  chunk_link_node* chunk  = this->chunk_;
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
  ptr->next           = this->free_link_;
  this->free_link_    = ptr;

#if defined(_DEBUG)
  --this->allocated_count_;
#endif
}

void* object_pool::allocate_from_process_heap(void)
{
  chunk_link new_chunk =
      (chunk_link)malloc(sizeof(chunk_link_node) + element_size_ * element_count_);
#ifdef _DEBUG
  ::memset(new_chunk, 0x00, sizeof(chunk_link_node));
#endif
  tidy_chunk(new_chunk)->next = nullptr;

  // link the new_chunk
  new_chunk->next = this->chunk_;
  this->chunk_    = new_chunk;

  // allocate 1 object
  auto ptr         = POOL_FL_BEGIN(new_chunk);
  this->free_link_ = ptr->next;

#if defined(_DEBUG)
  ++this->allocated_count_;
#endif
  return reinterpret_cast<void*>(ptr);
}

void* object_pool::allocate_from_chunk(void)
{
  free_link_node* ptr = this->free_link_;
  this->free_link_    = ptr->next;
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
    reinterpret_cast<free_link_node*>(ptr)->next =
        reinterpret_cast<free_link_node*>(ptr + element_size_);
  }
  return reinterpret_cast<free_link_node*>(rbegin);
}

} // namespace detail

} // namespace gc
} // namespace yasio

#endif // YASIO__OBJECT_POOL_CPP
