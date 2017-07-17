#include "ibinarystream.h"
//#include "crypto_wrapper.h"

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
    remain_ -= size;
    if (remain_ < 0) // == 0, packet decode complete.
        throw std::logic_error("packet error, data insufficiently!");
    return remain_;
}

