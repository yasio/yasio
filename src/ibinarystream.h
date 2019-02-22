//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.7
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
#ifndef _IBINARYSTREAM_H_
#define _IBINARYSTREAM_H_
#include <string>
#include "string_view.hpp"
#include <sstream>
#include <exception>
#include "endian_portable.h"
class obinarystream;
class ibinarystream
{
public:
  ibinarystream();
  ibinarystream(const void *data, int size);
  ibinarystream(const obinarystream *);
  ibinarystream(const ibinarystream &right) = delete;
  ibinarystream(ibinarystream &&right)      = delete;

  ~ibinarystream();

  void assign(const void *data, int size);

  ibinarystream &operator=(const ibinarystream &right) = delete;
  ibinarystream &operator=(ibinarystream &&right) = delete;

  template <typename _Nty> void read_i(_Nty &ov);

  template <typename _Nty> _Nty read_ix()
  {
    _Nty value;
    read_i(value);
    return value;
  }

  int32_t read_i24();
  uint32_t read_u24();

  void read_v(std::string &);   // 32 bits length field
  void read_v16(std::string &); // 16 bits length field
  void read_v8(std::string &);  // 8 bits length field

  void read_v(void *, int);
  void read_v16(void *, int);
  void read_v8(void *, int);

  void read_bytes(std::string &oav, int len);
  void read_bytes(void *oav, int len);

  std::string_view read_v();
  std::string_view read_v16();
  std::string_view read_v8();

  std::string_view read_bytes(int len);

  inline const char *data() { return ptr_; }
  inline int size(void) { return size_; }

  template <typename _LenT> std::string_view read_vx()
  {
    _LenT n = purelib::endian::ntohv(*(_LenT *)consume(sizeof(n)));

    if (n > 0)
    {
      return read_bytes(n);
    }

    return {};
  }

protected:
  // will throw std::logic_error
  const char *consume(size_t size) noexcept(false);

protected:
  const char *ptr_;
  int size_;
};

template <typename _Nty> inline void ibinarystream::read_i(_Nty &ov)
{
  ov = purelib::endian::ntohv(*((_Nty *)consume(sizeof(_Nty))));
}

template <> inline void ibinarystream::read_i<float>(float &ov)
{
  ov = ntohf(*((uint32_t *)consume(sizeof(ov))));
}

template <> inline void ibinarystream::read_i<double>(double &ov)
{
  ov = ntohd(*((uint64_t *)consume(sizeof(uint64_t))));
}

#endif
