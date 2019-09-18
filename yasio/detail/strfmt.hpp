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
#ifndef YASIO__SFMT_HPP
#define YASIO__SFMT_HPP

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>

namespace yasio
{
/*--- This is a C++ universal sprintf in the future.
 **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is
 *different
 **      VS2013 or Unix-Like System will return -1 when buffer not enough, but
 *VS2015/2017 will return the actural needed length for buffer at this station
 **      The _vsnprintf behavior is compatible API which always return -1 when
 *buffer isn't enough at VS2013/2015/2017
 **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later,
 *AND it's also standard-compliant, see reference:
 *http://www.cplusplus.com/reference/cstdio/vsnprintf/
 */
inline std::string strfmt(size_t n, const char* format, ...)
{
  va_list args;
  std::string buffer(n, '\0');

  va_start(args, format);
  int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
  va_end(args);

  if (nret >= 0)
  {
    if ((unsigned int)nret < buffer.length())
    {
      buffer.resize(nret);
    }
    else if ((unsigned int)nret > buffer.length())
    { // VS2015/2017 or later Visual Studio Version
      buffer.resize(nret);

      va_start(args, format);
      nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
      va_end(args);
    }
    // else equals, do nothing.
  }
  else
  { // less or equal VS2013 and Unix System glibc implement.
    do
    {
      buffer.resize(buffer.length() * 3 / 2);

      va_start(args, format);
      nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
      va_end(args);

    } while (nret < 0);

    buffer.resize(nret);
  }

  return buffer;
}

} // namespace yasio

#endif
