//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
//
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

/*
** yasio-ni.cpp: The yasio native interface for interop.
*/

#include "yasio/yasio.h"

#if defined(_WIN32)
#  define YASIO_NI_API __declspec(dllexport)
#else
#  define YASIO_NI_API
#endif

using namespace yasio::inet;

namespace
{
template <typename _CStr, typename _Fn>
inline void fast_split(_CStr s, size_t slen, typename std::remove_pointer<_CStr>::type delim,
                       _Fn func)
{
  auto _Start = s; // the start of every string
  auto _Ptr   = s; // source string iterator
  auto _End   = s + slen;
  while ((_Ptr = strchr(_Ptr, delim)))
  {
    if (_Start <= _Ptr)
      func(_Start, _Ptr);
    _Start = _Ptr + 1;
    ++_Ptr;
  }
  if (_Start <= _End)
  {
    func(_Start, _End);
  }
}
} // namespace

#if defined(__cplusplus)
extern "C" {
#endif

YASIO_NI_API int yasio_start(const char *params,
                             void (*callback)(uint32_t emask, int cidx, intptr_t vfd,
                                              intptr_t bytes, int len))
{
  std::vector<io_hostent> hosts;
  std::string strParams = params;
  fast_split(&strParams.front(), strParams.length(), ';', [&](char *s, char *e) {
    if (s != e)
    {
      io_hostent host;
      int idx = 0;
      fast_split(s, e - s, ':', [&](char *ss, char *ee) {
        auto ch = *ee;
        *ee     = '\0';
        switch (idx++)
        {
          case 0:
            host.host_.assign(ss, ee);
            break;
          case 1:
            host.port_ = atoi(ss);
            break;
        }
        *ee = ch;
      });
      hosts.push_back(std::move(host));
    }
  });

  myasio->start_service(hosts, [=](event_ptr e) {
    uint32_t emask = ((e->kind() << 16) & 0xffff0000) | (e->status() & 0xffff);
    callback(emask, e->cindex(), reinterpret_cast<intptr_t>(e->transport()),
             reinterpret_cast<intptr_t>(!e->packet().empty() ? e->packet().data() : nullptr),
             static_cast<int>(e->packet().size()));
  });

  return static_cast<int>(hosts.size());
}
YASIO_NI_API void yasio_set_option(int opt, const char *params)
{
  std::string strParams = params;
  switch (opt)
  {
    case YOPT_CHANNEL_REMOTE_ENDPOINT:
    {
      int cidx = 0;
      std::string ip;
      int port = 0;
      int idx  = 0;
      fast_split(&strParams.front(), strParams.length(), ';', [&](char *s, char *e) {
        auto ch = *e;
        if (idx == 0)
          cidx = atoi(s);
        else if (idx == 1)
          ip.assign(s, e);
        else if (idx == 2)
          port = atoi(s);
        ++idx;
        *e = ch;
      });
      myasio->set_option(opt, cidx, ip.c_str(), port);
    }
    break;
    case YOPT_LFBFD_PARAMS:
    {
      int args[4];
      int idx = 0;
      fast_split(&strParams.front(), strParams.length(), ';', [&](char *s, char *e) {
        auto ch = *e;
        if (idx < 4)
          args[idx] = atoi(s);
        ++idx;
        *e = ch;
      });
      myasio->set_option(opt, args[0], args[1], args[2], args[3]);
    }
    break;
    case YOPT_LOG_FILE:
      myasio->set_option(opt, params);
      break;
  }
}
YASIO_NI_API void yasio_open(int cindex, int kind) { myasio->open(cindex, kind); }
YASIO_NI_API void yasio_close(int cindex) { myasio->close(cindex); }
YASIO_NI_API void yasio_close_vfd(intptr_t vfd)
{
  auto p = reinterpret_cast<transport_ptr>(vfd);
  myasio->close(p);
}
YASIO_NI_API int yasio_write(intptr_t vfd, const unsigned char *bytes, int len)
{
  std::vector<char> buf(bytes, bytes + len);
  auto p = reinterpret_cast<transport_ptr>(vfd);
  return myasio->write(p, std::move(buf));
}
YASIO_NI_API void yasio_dispatch_events(int maxEvents) { myasio->dispatch_events(maxEvents); }
YASIO_NI_API void yasio_stop() { myasio->stop_service(); }

#if defined(__cplusplus)
}
#endif
