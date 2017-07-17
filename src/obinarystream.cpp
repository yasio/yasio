#include "obinarystream.h"
#include <iostream>
#include <fstream>
//#include "xxfsutility.h"
//#include "crypto_wrapper.h"

obinarystream::obinarystream(size_t buffersize)
{
	buffer_.reserve(buffersize);
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

void obinarystream::write_v(const std::string & value)
{
	write_v(value.c_str(), static_cast<int>(value.size()));
}

void obinarystream::write_bytes(const std::string& v)
{
    write_bytes(v.c_str(), static_cast<int>(v.size()));
}

void obinarystream::write_bytes(const void* v, int vl)
{
	if (vl > 0) {
		auto offset = buffer_.size();
		buffer_.resize(buffer_.size() + vl);
		::memcpy(buffer_.data() + offset, v, vl);
	}
}

void obinarystream::save(const char * filename)
{
	// fsutil::write_file_data(filename, buffer_.data(), buffer_.size());
	std::ofstream fout;
	fout.open(filename, std::ios::binary);
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

