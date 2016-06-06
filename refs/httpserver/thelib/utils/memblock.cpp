#include "memblock.h"
using namespace thelib::gc;

memblock::memblock(size_t elemsize, size_t elemcount) : _Myhead(nullptr), _Mychunk(nullptr), _Mycount(0), _MyElemSize(elemsize), _MyElemCount(elemcount)
{
    this->_Enlarge();
}

memblock::~memblock(void)
{
    this->purge();
}

void memblock::cleanup(void)
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

void memblock::purge(void)
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

size_t memblock::count(void) const
{
    return _Mycount;
}

// if the type is not pod, you may be use placement new to call the constructor,
// for example: _Ty* obj = new(pool.get()) _Ty(arg1,arg2,...);
void* memblock::get(void) 
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

void memblock::release(void* _Ptr)
{
    free_link_node* ptr = reinterpret_cast<free_link_node*>(_Ptr);
    ptr->next = this->_Myhead;
    this->_Myhead = ptr;
    --_Mycount;
}

void memblock::_Enlarge(void)
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

