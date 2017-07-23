#include "ibinarystream.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ibinarystream::ibinarystream()
{
    this->assign(nullptr, 0);
}

ibinarystream::ibinarystream(const char* data, int size)
{
    this->assign(data, size);
}

void ibinarystream::assign(const char* data, int size)
{
    ptr_ = data;
    remain_ = size;
}

int ibinarystream::read_v(std::string& oav)
{
    return read_vv<LENGTH_FIELD_TYPE>(oav);
}

int ibinarystream::read_v16(std::string& oav)
{
    return read_vv<uint16_t>(oav);
}

int ibinarystream::read_v32( std::string& oav)
{
    return read_vv<uint32_t>(oav);
}

int ibinarystream::read_v(void* oav, int len)
{
    return read_vv<LENGTH_FIELD_TYPE>(oav, len);
}

int ibinarystream::read_v16(void* oav, int len)
{
    return read_vv<uint16_t>(oav, len);
}

int ibinarystream::read_v32(void* oav, int len)
{
    return read_vv<uint32_t>(oav, len);
}

int ibinarystream::read_bytes(std::string& oav, int len)
{
    if (len > 0) {
        oav.resize(len);
        return read_bytes(&oav.front(), len);
    }
    return 0;
}

int ibinarystream::read_bytes(void* oav, int len)
{
    if (len > 0) {
        ::memcpy(oav, ptr_, len);
        return consume(len);
    }
    return 0;
}

int ibinarystream::consume(size_t size)
{
    ptr_ += size;
    remain_ -= static_cast<int>(size);
    if (remain_ < 0) // == 0, packet decode complete.
        throw std::logic_error("packet error, data insufficiently!");
    return remain_;
}
