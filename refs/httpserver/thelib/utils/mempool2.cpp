#include "mempool2.h"

using namespace thelib::gc;

thelib::gc::mempool2 thelib::gc::mempool2::global_mp;

mempool2::mempool2(void)
{
    init();
}
mempool2::~mempool2(void)
{
    for(size_t index = 0; index <= MAX_INDEX; ++index)
    {
        delete blocks[index];
    }
}
void* mempool2::allocate(size_t in_size)
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
void mempool2::deallocate(void* p)
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

void mempool2::init(void)
{
    for(size_t index = 0; index <= MAX_INDEX; ++index)
    {
        size_t elem_size = BOUNDARY_SIZE << index;
        blocks[index] = new memblock(elem_size, SZ(512,k) / elem_size);
    }
}

