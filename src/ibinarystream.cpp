#include "obinarystream.h"
#include "ibinarystream.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ibinarystream::ibinarystream()
{
    this->assign("", 0);
}

ibinarystream::ibinarystream(const char* data, int size)
{
    this->assign(data, size);
}

ibinarystream::ibinarystream(const obinarystream* obs)
{
    auto& buffer = obs->buffer();
    this->assign(!buffer.empty() ? buffer.data() : "", buffer.size());
}

ibinarystream::~ibinarystream()
{
}

void ibinarystream::assign(const char* data, int size)
{
    ptr_ = data;
    size_ = size;
}

void ibinarystream::read_v(std::string& oav)
{
    oav = read_vx<LENGTH_FIELD_TYPE>();
}

std::string_view ibinarystream::read_v()
{
    return read_vx<LENGTH_FIELD_TYPE>();
}

void ibinarystream::read_v16(std::string& oav)
{
    oav = read_vx<uint16_t>();
}

void ibinarystream::read_v32( std::string& oav)
{
    oav = read_vx<uint32_t>();
}

void ibinarystream::read_v(void* oav, int len)
{
    read_vx<LENGTH_FIELD_TYPE>().copy((char*)oav, len);
}

void ibinarystream::read_v16(void* oav, int len)
{
    read_vx<uint16_t>().copy((char*)oav, len);
}

void ibinarystream::read_v32(void* oav, int len)
{
    read_vx<uint32_t>().copy((char*)oav, len);
}

void ibinarystream::read_bytes(std::string& oav, int len)
{
    if (len > 0) {
        oav.resize(len);
        read_bytes(&oav.front(), len);
    }
}

void ibinarystream::read_bytes(void* oav, int len)
{
    if (len > 0) {
        ::memcpy(oav, consume(len), len);
    }
}

std::string_view ibinarystream::read_bytes(int len)
{
    std::string_view sv;
    if (len > 0) {
        sv = std::string_view(consume(len), len);
    }
    return sv;
}

const char* ibinarystream::consume(size_t size)
{
    if (size_ <= 0)
        throw std::logic_error("packet error, data insufficiently!");

    auto ptr = ptr_;
    ptr_ += size;
    size_ -= static_cast<int>(size);
    
    return ptr;
}
