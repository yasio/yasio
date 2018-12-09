//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.2
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2018 halx99

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
#include "obinarystream.h"
#include "ibinarystream.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

ibinarystream::ibinarystream()
{
    this->assign("", 0);
}

ibinarystream::ibinarystream(const void* data, int size)
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

void ibinarystream::assign(const void* data, int size)
{
    ptr_ = static_cast<const char*>(data);
    size_ = size;
}

// TODO: rewrite a class uint24_t
uint32_t ibinarystream::read_i24()
{
    uint32_t value = 0;
    auto ptr = consume(3);
    memcpy(&value, ptr, 3);
    return ntohl(value) >> 8;
}

void ibinarystream::read_v(std::string& oav)
{
    auto sv = read_vx<LENGTH_FIELD_TYPE>();
    oav.assign(sv.data(), sv.length());
}

std::string_view ibinarystream::read_v()
{
    return read_vx<LENGTH_FIELD_TYPE>();
}

void ibinarystream::read_v16(std::string& oav)
{
    auto sv = read_vx<uint16_t>();
	oav.assign(sv.data(), sv.length());
}

void ibinarystream::read_v32( std::string& oav)
{
    auto sv = read_vx<uint32_t>();
    oav.assign(sv.data(), sv.length());
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
