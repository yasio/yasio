#include "mempool3.h"

using namespace thelib::gc;

thelib::gc::mempool3 thelib::gc::mempool3::global_mp;

mempool3::mempool3(void)
{
    init();
}
mempool3::~mempool3(void)
{
    for(size_t index = 0; index <= MAX_INDEX; ++index)
    {
        delete blocks[index];
    }
}
void* mempool3::allocate(size_t in_size)
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
void mempool3::deallocate(void* p)
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

void mempool3::init(void)
{
    for(size_t index = 0; index <= MAX_INDEX; ++index)
    {
        size_t elem_size = BOUNDARY_SIZE * (index + 1);
        blocks[index] = new memblock(elem_size, SZ(64,k) / elem_size);
    }
}

