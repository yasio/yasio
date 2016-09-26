#include "ibinarystream.h"
//#include "crypto_wrapper.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ibinarystream::ibinarystream()
{
    this->vassign(nullptr, 0);
}

ibinarystream::ibinarystream(const char* data, int size)
{
    this->vassign(data, size);
}

void ibinarystream::vassign(const char* data, int size)
{
    ptr_ = data;
    remain_ = size;
}

int ibinarystream::read_v(std::string& ov)
{
    uint16_t n = ntohs(*(uint16_t*)(ptr_));

    (void)consume(sizeof(n));

    if (n > 0) {
        ov.resize(n);
        ::memcpy(&ov.front(), ptr_, n);
        return consume(n);
    }

    return remain_;
}

int ibinarystream::read_v(void* ov, int len)
{
    uint16_t n = ntohs(*(uint16_t*)(ptr_));

    (void)consume(sizeof(n));

    if (n > 0) {
        // ov.resize(n);
        if (len < n)
            n = len;
        ::memcpy(ov, ptr_, n);
        return consume(n);
    }

    return remain_;
}

int ibinarystream::read_array(std::string& oav, int len)
{
    if (len > 0) {
        oav.resize(len);
        return read_array(&oav.front(), len);
    }
    return 0;
}

int ibinarystream::read_array(void* oav, int len)
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

