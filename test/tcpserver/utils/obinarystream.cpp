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

std::vector<char> obinarystream::move()
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
	auto size = value.size();
	auto l = purelib::endian::htonv(static_cast<uint16_t>(size));

	auto append_size = sizeof(l) + size;

	auto offset = buffer_.size();
	buffer_.resize(offset + append_size);

	::memcpy(buffer_.data() + offset, &l, sizeof(l));
	if (size > 0) {
		::memcpy(buffer_.data() + offset + sizeof l, value.c_str(), size);
	}
}

void obinarystream::write_v(const void* v, int size)
{
	auto l = purelib::endian::htonv(static_cast<uint16_t>(size));

	auto append_size = sizeof(l) + size;
	auto offset = buffer_.size();
	buffer_.resize(offset + append_size);

	::memcpy(buffer_.data() + offset, &l, sizeof(l));
	if (size > 0)
		::memcpy(buffer_.data() + offset + sizeof l, v, size);
}

void obinarystream::write_array(const std::string& v)
{
    write_array(v.c_str(), v.size());
}

void obinarystream::write_array(const void* v, int vl)
{
	if (vl > 0) {
		auto offset = buffer_.size();
		buffer_.resize(buffer_.size() + vl);
		::memcpy(buffer_.data() + offset, v, vl);
	}
}

void obinarystream::set_length()
{
	modify_i<uint16_t>(0, static_cast<uint16_t>(buffer_.size()));
}

//void ibinarystream::write_binarybuf(const obinarystream & buf)
//{
//    auto size = buf.buffer_.size();
//    buffer_.append((const char*)&size, sizeof(size));
//    if (size != 0)
//        buffer_.append(buf.buffer_.c_str(), size);
//}

void obinarystream::save(const char * filename)
{
	//    fsutil::write_file_data(filename, buffer_.data(), buffer_.size());
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

