//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.3
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
#ifndef _OBINARYSTREAM_H_
#define _OBINARYSTREAM_H_
#include <string>
#include "string_view.hpp"
#include <sstream>
#include <vector>
#include <stack>
#include "endian_portable.h"
class obinarystream
{
public:
    obinarystream(size_t buffersize = 256);
    obinarystream(const obinarystream& right);
    obinarystream(obinarystream&& right);
    ~obinarystream();

    void push8();
    void pop8();
    void pop8(uint8_t);

    void push16();
    void pop16();
    void pop16(uint16_t);

    void push24();
    void pop24();
    void pop24(uint32_t);

    void push32();
    void pop32();
    void pop32(uint32_t);

    obinarystream& operator=(const obinarystream& right);
    obinarystream& operator=(obinarystream&& right);

    std::vector<char> take_buffer();

    template<typename _Nty>
    size_t write_i(const _Nty value);

    size_t write_i(float);
    size_t write_i(double);
    size_t write_i24(uint32_t value); // highest byte ignored

    size_t write_v(std::string_view);
    size_t write_v16(std::string_view);
    size_t write_v32(std::string_view);
    
    size_t write_v(const void* v, int size);
    size_t write_v16(const void* v, int size);
    size_t write_v32(const void* v, int size);

    size_t write_bytes(std::string_view);
    size_t write_bytes(const void* v, int vl);

    size_t length() const { return buffer_.size(); }
    const char* data() const { return buffer_.data(); }

    const std::vector<char>& buffer() const { return buffer_; }
    std::vector<char>& buffer() { return buffer_; }
    
    char* wdata(size_t offset = 0) { return &buffer_.front() + offset; }

    template<typename _Nty>
    void set_i(std::streamoff offset, const _Nty value);

    template<typename _LenT>
    size_t write_vx(const void* v, int size)
    {
        auto l = purelib::endian::htonv(static_cast<_LenT>(size));

        auto append_size = sizeof(l) + size;
        auto offset = buffer_.size();
        buffer_.resize(offset + append_size);

        ::memcpy(buffer_.data() + offset, &l, sizeof(l));
        if (size > 0)
            ::memcpy(buffer_.data() + offset + sizeof l, v, size);
			
	    return buffer_.size();
    }
public:
    void save(const char* filename);

protected:
    std::vector<char>  buffer_;
    std::stack<size_t> offset_stack_;
};

template <typename _Nty>
inline size_t obinarystream::write_i(const _Nty value)
{
    size_t offset = buffer_.size();
    auto nv = purelib::endian::htonv(value);
    buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
    return offset;
}

inline size_t obinarystream::write_i(float value)
{
    size_t offset = buffer_.size();
    auto nv = htonf(value);
    buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
    return offset;
}

inline size_t obinarystream::write_i(double value)
{
    size_t offset = buffer_.size();
    auto nv = htond(value);
    buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
    return offset;
}

template <typename _Nty>
inline void obinarystream::set_i(std::streamoff offset, const _Nty value)
{
    auto pvalue = (_Nty*)(buffer_.data() + offset);
    *pvalue = purelib::endian::htonv(value);
}

#endif
