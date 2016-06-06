// mempool2.h: a simple mempool implementation
#ifndef _MEMPOOL2_H_
#define _MEMPOOL2_H_

#ifndef _POLITE_SZ_ALIGN
#define _POLITE_SZ_ALIGN
#define sz_align(d,a) (((d) + ((a) - 1)) & ~((a) - 1))  
// #define sz_align(x,n) ( (x) + ( (n) - (x) % (n) ) - ( ( (x) % (n) == 0 ) * (n) ) )
#endif

/*
 * _POL_CONSTANTS_ Definitions
 */
#ifndef _POL_CONSTANTS
#define _POL_CONSTANTS
static const uint64_t __B__ =  (1);
static const uint64_t __KB__ = (1024 * __B__) ;
static const uint64_t __MB__ = (1024 * __KB__);
static const uint64_t __GB__ = (1024 * __MB__);
static const uint64_t __TB__ = (1024 * __GB__);
static const uint64_t __PB__ = (1024 * __TB__);
static const uint64_t __EB__ = (1024 * __PB__);
#endif  /* _POL_CONSTANTS */
#include <memory>
/*
 * TEMPLATE TYPE structs Definition
 */
#ifndef _POL_BOOL
#define _POL_BOOL
template<typename _Int>
struct Bool
{
    typedef _Int type;
    static const type True = ~0;
    static const type False = 0;
} ;
#endif /* _POL_BOOL */

/*
 * size in bytes constant: SZ Definition
 */
#ifndef _POL_SZ
#define _POL_SZ
#define __b__ __B__
#define __k__ __K__
#define __m__ __M__
#define __g__ __G__
#define __t__ __T__
#define __e__ __E__
#define __K__ __KB__
#define __M__ __MB__
#define __G__ __GB__
#define __T__ __TB__
#define __P__ __PB__
#define __E__ __EB__
#define SZ(n, u) ( (n) * __##u##__ )
static const uint8_t __MAX_UINT8 = SZ(256, B) - 1;
static const uint16_t __MAX_UINT16 = SZ(64, KB) - 1;
static const uint32_t __MAX_UINT32 = SZ(4, GB) - 1;
static const uint64_t __MAX_UINT64 = SZ(15, EB) + (SZ(1, EB) - 1);
#endif   /* _POL_SZ */

#include <assert.h>
#include <stdint.h>

namespace thelib {
namespace gc {

class memblock 
{
    typedef struct free_link_node
    { 
        free_link_node* next; 
    } *free_link;

    typedef struct chunk_link_node
    {
        chunk_link_node* next; /* at start of data */
        char            data[0];
    } *chunk_link;

    memblock(const memblock&);
    void operator= (const memblock&);

public:
    memblock(size_t elemsize, size_t elemcount) : _Myhead(nullptr), _Mychunk(nullptr), _Mycount(0), _MyElemSize(elemsize), _MyElemCount(elemcount)
    {
        this->_Enlarge();
    }

    ~memblock(void)
    {
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
            char* rbegin    = begin + (_MyElemCount - 1) * _MyElemSize; 

            if(prev != nullptr)
                prev->next = reinterpret_cast<free_link>(begin);
            for (char* ptr = begin; ptr < rbegin; ptr += _MyElemSize )
            { 
                reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + _MyElemSize);
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
        free_link_node* ptr = reinterpret_cast<free_link_node*>(_Ptr);
        ptr->next = this->_Myhead;
        this->_Myhead = ptr;
        --_Mycount;
    }

private:
    void _Enlarge(void)
    {
        // static_assert(_ElemCount > 0, "Invalid Element Count");
        chunk_link new_chunk  = (chunk_link)malloc(sizeof(chunk_link_node) + _MyElemCount * _MyElemSize); 
        if(new_chunk == nullptr)
            return;
#ifdef _DEBUG
        ::memset(new_chunk, 0x00, sizeof(chunk_link_node) + _MyElemCount * _MyElemSize);
#endif
        new_chunk->next = this->_Mychunk; 
        this->_Mychunk  = new_chunk; 

        char* begin     = this->_Mychunk->data; 
        char* rbegin    = begin + (_MyElemCount - 1) * _MyElemSize; 

        for (char* ptr = begin; ptr < rbegin; ptr += _MyElemSize )
        { 
            reinterpret_cast<free_link_node*>(ptr)->next = reinterpret_cast<free_link_node*>(ptr + _MyElemSize);
        } 

        reinterpret_cast <free_link_node*>(rbegin)->next = nullptr; 
        this->_Myhead = reinterpret_cast<free_link_node*>(begin); 
    }

private:
    free_link        _Myhead; // link to free head
    chunk_link       _Mychunk; // chunk link
    size_t           _Mycount; // allocated count 
    const size_t     _MyElemSize; // elem size
    const size_t     _MyElemCount; // elem count
}; /* equalize size block */

#ifndef _INLINE_ASM_BSR
#define _INLINE_ASM_BSR
#ifdef _WIN32
#pragma intrinsic(_BitScanReverse)
#define BSR(_Index, _Mask) _BitScanReverse((unsigned long*)&_Index, _Mask);
#else
#define BSR(_Index,_Mask) __asm__ __volatile__("bsrl %1, %%eax\n\tmovl %%eax, %0":"=r"(_Index):"r"(_Mask):"eax")
#endif
#endif

class mempool2 /// Granularity BOUNDARY_SIZE Geometric Progression
{
    static const size_t MAX_INDEX = 11;
    static const size_t BOUNDARY_INDEX = 5 + sizeof(void*) / 8; // 5 at x86_32, 6 at x86_64
    static const size_t BOUNDARY_SIZE = (1 << BOUNDARY_INDEX); // 32bytes at x86_32, 64bytes at x86_64
public:
    mempool2(void)
    {
        init();
    }
    ~mempool2(void)
    {
        for(size_t index = 0; index <= MAX_INDEX; ++index)
        {
            delete blocks[index];
        }
    }
    void* allocate(size_t in_size)
    {
        uint32_t size = sz_align(in_size + sizeof(uint32_t), BOUNDARY_SIZE);
        uint32_t index = 0;
        BSR(index, size >> BOUNDARY_INDEX);
        index += (bool)((size-1) & size); // adjust index
        void* raw = nullptr;
        if(index <= MAX_INDEX) 
        {
            raw = this->blocks[index]->get();
        }
        else {
            raw = malloc(size);
        }

        if(raw != nullptr) {
            *reinterpret_cast<uint32_t*>(raw) = size;
            return (char*)raw + sizeof(uint32_t);
        }

        return nullptr;
    }
    void deallocate(void* p)
    {
        char* raw = (char*)p - sizeof(uint32_t);
        uint32_t size = *reinterpret_cast<uint32_t*>( raw );
        uint32_t index = 0;
        BSR(index, size >> BOUNDARY_INDEX);
        index += (bool)((size-1) & size); // adjust index
        if(index <= MAX_INDEX) {
            this->blocks[index]->release(raw);
            return;
        }
        free(raw);
    }

private:
    void init(void)
    {
        for(size_t index = 0; index <= MAX_INDEX; ++index)
        {
            size_t elem_size = BOUNDARY_SIZE << index;
            blocks[index] = new memblock(elem_size, SZ(512,k) / elem_size);
        }
    }

    memblock* blocks[MAX_INDEX + 1];
}; // namespace: thelib::gc::mempool2

class mempool3 /// Granularity: BOUNDARY_SIZE * n
{
    static const size_t MAX_INDEX = 39;
    static const size_t BOUNDARY_INDEX = 7; 
    static const size_t BOUNDARY_SIZE = (1 << BOUNDARY_INDEX);
public:
    mempool3(void)
    {
        init();
    }
    ~mempool3(void)
    {
        for(size_t index = 0; index <= MAX_INDEX; ++index)
        {
            delete blocks[index];
        }
    }
    void* allocate(size_t in_size)
    {
        uint32_t size = sz_align(in_size + sizeof(uint32_t), BOUNDARY_SIZE);
        uint32_t index = (size >> BOUNDARY_INDEX) - 1;
        void* raw = nullptr;
        if(index <= MAX_INDEX) 
        {
            raw = this->blocks[index]->get();
        }
        else {
            raw = malloc(size);
        }

        if(raw != nullptr) {
            *reinterpret_cast<uint32_t*>(raw) = size;
            return (char*)raw + sizeof(uint32_t);
        }

        return nullptr;
    }
    void deallocate(void* p)
    {
        char* raw = (char*)p - sizeof(uint32_t);
        uint32_t size = *reinterpret_cast<uint32_t*>( raw );
        uint32_t index = (size >> BOUNDARY_INDEX) - 1;
        if(index <= MAX_INDEX) {
            this->blocks[index]->release(raw);
            return;
        }
        free(raw);
    }

private:
    void init(void)
    {
        for(size_t index = 0; index <= MAX_INDEX; ++index)
        {
            size_t elem_size = BOUNDARY_SIZE * (index + 1);
            blocks[index] = new memblock(elem_size, SZ(64,k) / elem_size);
        }
    }

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
}; // namespace: thelib::gc::mempool2

}; // namespace: thelib::gc
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

