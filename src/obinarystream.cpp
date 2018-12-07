#include "obinarystream.h"
#include <iostream>
#include <fstream>

obinarystream::obinarystream(size_t buffersize)
{
    buffer_.reserve(buffersize);
}

void obinarystream::push32()
{
    offset_stack_.push(buffer_.size());
    write_i(static_cast<uint32_t>(0));
}

void obinarystream::pop32()
{
    auto offset = offset_stack_.top();
    modify_i(offset, static_cast<uint32_t>(buffer_.size()));
    offset_stack_.pop();
}

void obinarystream::pop32(uint32_t value)
{
    auto offset = offset_stack_.top();
    modify_i(offset, value);
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
//
//void obinarystream::compress(size_t offset/* header maybe */)
//{
//    auto compr = crypto::zlib::abi::compress(unmanaged_string(this->buffer_.data() + offset, this->buffer_.size() - offset));
//    this->buffer_.resize(offset + compr.size());
//    memcpy(&this->buffer_.front() + offset, compr.data(), compr.size());
//}

