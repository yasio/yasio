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
#ifndef YASIO__OBSTREAM_H
#define YASIO__OBSTREAM_H
#include <string>
#include "yasio/detail/string_view.hpp"
#include <sstream>
#include <vector>
#include <stack>
#include "yasio/detail/endian_portable.h"
namespace yasio
{
class obstream
{
public:
  obstream(size_t buffersize = 256);
  obstream(const obstream &right);
  obstream(obstream &&right);
  ~obstream();

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

  obstream &operator=(const obstream &right);
  obstream &operator=(obstream &&right);

  std::vector<char> take_buffer();

  template <typename _Nty> void write_i(_Nty value);

  void write_i24(uint32_t value); // highest byte ignored

  void write_i7(int value);

  void write_s(yasio::string_view sv)
  {
    write_i7(static_cast<int>(sv.size()));
    write_bytes(sv.data(), sv.length());
  }

  /* 32 bits length field */
  void write_v(yasio::string_view);
  /* 16 bits length field */
  void write_v16(yasio::string_view);
  /* 8 bits length field */
  void write_v8(yasio::string_view);

  void write_v(const void *v, int size);
  void write_v16(const void *v, int size);
  void write_v8(const void *v, int size);

  void write_bytes(yasio::string_view);
  void write_bytes(const void *v, int vl);

  size_t length() const { return buffer_.size(); }
  const char *data() const { return buffer_.data(); }

  const std::vector<char> &buffer() const { return buffer_; }
  std::vector<char> &buffer() { return buffer_; }

  char *wdata(size_t offset = 0) { return &buffer_.front() + offset; }

  template <typename _Nty> void set_i(std::streamoff offset, const _Nty value);

  template <typename _LenT> void write_vx(const void *v, int size)
  {
    auto l = yasio::endian::htonv(static_cast<_LenT>(size));

    auto append_size = sizeof(l) + size;
    auto offset      = buffer_.size();
    buffer_.resize(offset + append_size);

    ::memcpy(buffer_.data() + offset, &l, sizeof(l));
    if (size > 0)
      ::memcpy(buffer_.data() + offset + sizeof l, v, size);
  }
  obstream sub(size_t offset, size_t count = -1);

public:
  void save(const char *filename);

protected:
  std::vector<char> buffer_;
  std::stack<size_t> offset_stack_;
}; // class obstream

template <typename _Nty> inline void obstream::write_i(_Nty value)
{
  auto nv = yasio::endian::htonv(value);
  buffer_.insert(buffer_.end(), (const char *)&nv, (const char *)&nv + sizeof(nv));
}

template <> inline void obstream::write_i<uint8_t>(uint8_t value) { buffer_.push_back(value); }

template <> inline void obstream::write_i<float>(float value)
{
  auto nv = htonf(value);
  buffer_.insert(buffer_.end(), (const char *)&nv, (const char *)&nv + sizeof(nv));
}

template <> inline void obstream::write_i<double>(double value)
{
  auto nv = htond(value);
  buffer_.insert(buffer_.end(), (const char *)&nv, (const char *)&nv + sizeof(nv));
}

template <typename _Nty> inline void obstream::set_i(std::streamoff offset, const _Nty value)
{
  auto nv = yasio::endian::htonv(value);
  memcpy(buffer_.data() + offset, &nv, sizeof(nv));
}
} // namespace yasio
#endif
