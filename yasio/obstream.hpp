//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
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
#ifndef YASIO__OBSTREAM_HPP
#define YASIO__OBSTREAM_HPP
#include <string>
#include "yasio/cxx17/string_view.hpp"
#include <sstream>
#include <vector>
#include <stack>
#include "yasio/detail/endian_portable.hpp"
#include "yasio/detail/config.hpp"
namespace yasio
{
class obstream
{
public:
  YASIO__DECL obstream(size_t capacity = 256);
  YASIO__DECL obstream(const obstream& rhs);
  YASIO__DECL obstream(obstream&& rhs);
  YASIO__DECL ~obstream();

  YASIO__DECL void push8();
  YASIO__DECL void pop8();
  YASIO__DECL void pop8(uint8_t);

  YASIO__DECL void push16();
  YASIO__DECL void pop16();
  YASIO__DECL void pop16(uint16_t);

  YASIO__DECL void push24();
  YASIO__DECL void pop24();
  YASIO__DECL void pop24(uint32_t);

  YASIO__DECL void push32();
  YASIO__DECL void pop32();
  YASIO__DECL void pop32(uint32_t);

  YASIO__DECL obstream& operator=(const obstream& rhs);
  YASIO__DECL obstream& operator=(obstream&& rhs);

  template <typename _Nty> inline void write_i(_Nty value);

  YASIO__DECL void write_i24(uint32_t value); // highest byte ignored

  YASIO__DECL void write_i7(int value);

  /* write blob data with variant length of length field. */
  YASIO__DECL void write_va(cxx17::string_view sv);

  /* 32 bits length field */
  YASIO__DECL void write_v(cxx17::string_view);
  /* 16 bits length field */
  YASIO__DECL void write_v16(cxx17::string_view);
  /* 8 bits length field */
  YASIO__DECL void write_v8(cxx17::string_view);

  YASIO__DECL void write_v(const void* v, int size);
  YASIO__DECL void write_v16(const void* v, int size);
  YASIO__DECL void write_v8(const void* v, int size);

  YASIO__DECL void write_bytes(cxx17::string_view);
  YASIO__DECL void write_bytes(const void* v, int vl);
  YASIO__DECL void write_byte(char v);

  bool empty() const { return buffer_.empty(); }
  size_t length() const { return buffer_.size(); }
  const char* data() const { return buffer_.data(); }

  const std::vector<char>& buffer() const { return buffer_; }
  std::vector<char>& buffer() { return buffer_; }

  char* wptr(size_t offset = 0) { return &buffer_.front() + offset; }

  template <typename _Nty> inline void set(std::streamoff offset, const _Nty value);

  template <typename _LenT> inline void write_vx(const void* v, int size)
  {
    auto l = yasio::endian::htonv(static_cast<_LenT>(size));

    auto append_size = sizeof(l) + size;
    auto offset      = buffer_.size();
    buffer_.resize(offset + append_size);

    ::memcpy(buffer_.data() + offset, &l, sizeof(l));
    if (size > 0)
      ::memcpy(buffer_.data() + offset + sizeof l, v, size);
  }
  YASIO__DECL obstream sub(size_t offset, size_t count = -1);

public:
  YASIO__DECL void save(const char* filename);

protected:
  std::vector<char> buffer_;
  std::stack<size_t> offset_stack_;
}; // class obstream

template <typename _Nty> inline void obstream::write_i(_Nty value)
{
  auto nv = yasio::endian::htonv(value);
  buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
}

template <> inline void obstream::write_i<uint8_t>(uint8_t value) { buffer_.push_back(value); }

template <> inline void obstream::write_i<float>(float value)
{
  auto nv = htonf(value);
  buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
}

template <> inline void obstream::write_i<double>(double value)
{
  auto nv = htond(value);
  buffer_.insert(buffer_.end(), (const char*)&nv, (const char*)&nv + sizeof(nv));
}

template <typename _Nty> inline void obstream::set(std::streamoff offset, const _Nty value)
{
  auto nv = yasio::endian::htonv(value);
  memcpy(buffer_.data() + offset, &nv, sizeof(nv));
}
} // namespace yasio

#if defined(YASIO_HEADER_ONLY)
#  include "yasio/obstream.cpp" // lgtm [cpp/include-non-header]
#endif

#endif
