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
#include "obinarystream.h"
#include <iostream>
#include <fstream>

obinarystream::obinarystream(size_t buffersize)
{
    buffer_.reserve(buffersize);
}

void obinarystream::push8()
{
    offset_stack_.push(buffer_.size());
    write_i(static_cast<uint8_t>(0));
}
void obinarystream::pop8()
{
    auto offset = offset_stack_.top();
    set_i(offset, static_cast<uint8_t>(buffer_.size()));
    offset_stack_.pop();
}
void obinarystream::pop8(uint8_t value)
{
    auto offset = offset_stack_.top();
    set_i(offset, value);
    offset_stack_.pop();
}

void obinarystream::push16()
{
    offset_stack_.push(buffer_.size());
    write_i(static_cast<uint16_t>(0));
}
void obinarystream::pop16()
{
    auto offset = offset_stack_.top();
    set_i(offset, static_cast<uint16_t>(buffer_.size()));
    offset_stack_.pop();
}
void obinarystream::pop16(uint16_t value)
{
    auto offset = offset_stack_.top();
    set_i(offset, value);
    offset_stack_.pop();
}

void obinarystream::push24()
{
    offset_stack_.push(buffer_.size());

    unsigned char u32buf[4] = { 0, 0, 0, 0 };
    write_bytes(u32buf, 3);
}
void obinarystream::pop24()
{
    auto offset = offset_stack_.top();
    auto value = htonl(static_cast<uint32_t>(buffer_.size())) >> 8;
    memcpy(wdata(offset), &value, 3);
    offset_stack_.pop();
}
void obinarystream::pop24(uint32_t value)
{
    auto offset = offset_stack_.top();
    value = htonl(value) >> 8;
    memcpy(wdata(offset), &value, 3);
    offset_stack_.pop();
}

void obinarystream::push32()
{
    offset_stack_.push(buffer_.size());
    write_i(static_cast<uint32_t>(0));
}

void obinarystream::pop32()
{
    auto offset = offset_stack_.top();
    set_i(offset, static_cast<uint32_t>(buffer_.size()));
    offset_stack_.pop();
}

void obinarystream::pop32(uint32_t value)
{
    auto offset = offset_stack_.top();
    set_i(offset, value);
    offset_stack_.pop();
}

obinarystream::obinarystream(const obinarystream& right) : buffer_(right.buffer_)
{

}

obinarystream::obinarystream(obinarystream && right) : buffer_(std::move(right.buffer_))
{
}

obinarystream::~obinarystream()
{
}

std::vector<char> obinarystream::take_buffer()
{
	return std::move(buffer_);
};

obinarystream& obinarystream::operator=(const obinarystream& right)
{
	buffer_ = right.buffer_;
	return *this;
}

obinarystream& obinarystream::operator=(obinarystream&& right)
{
	buffer_ = std::move(right.buffer_);
	return *this;
}

size_t obinarystream::write_i24(uint32_t value)
{
    value = htonl(value) >> 8;
    write_bytes(&value, 3);
    return buffer_.size();
}

size_t obinarystream::write_v(std::string_view value)
{
	return write_v(value.data(), static_cast<int>(value.size()));
}

size_t obinarystream::write_v16(std::string_view value)
{
    return write_v16(value.data(), static_cast<int>(value.size()));
}

size_t obinarystream::write_v32(std::string_view value)
{
    return write_v32(value.data(), static_cast<int>(value.size()));
}

size_t obinarystream::write_v(const void* v, int size)
{
    return write_vx<LENGTH_FIELD_TYPE>(v, size);
}

size_t obinarystream::write_v16(const void* v, int size)
{
    return write_vx<uint16_t>(v, size);
}

size_t obinarystream::write_v32(const void* v, int size)
{
    return write_vx<uint32_t>(v, size);
}

size_t obinarystream::write_bytes(std::string_view v)
{
    return write_bytes(v.data(), static_cast<int>(v.size()));
}

size_t obinarystream::write_bytes(const void* v, int vl)
{
	if (vl > 0) {
		auto offset = buffer_.size();
		buffer_.resize(buffer_.size() + vl);
		::memcpy(buffer_.data() + offset, v, vl);
	}
	return buffer_.size();
}

void obinarystream::save(const char * filename)
{
    std::ofstream fout;
    fout.open(filename, std::ios::binary);
    if (!this->buffer_.empty())
        fout.write(&this->buffer_.front(), this->length());
    fout.close();
}
