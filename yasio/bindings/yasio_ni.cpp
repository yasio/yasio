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
** yasio_ni.cpp: The yasio native interface for interop.
*/
#if !defined(_WIN32) || defined(_WINDLL)

#  include <string.h>
#  include "yasio/yasio.hpp"

#  if defined(_WINDLL)
#    define YASIO_NI_API __declspec(dllexport)
#  else
#    define YASIO_NI_API
#  endif

#  define YASIO_MAX_OPTION_ARGC 5

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

extern "C" {

YASIO_NI_API int yasio_start(const char* params,
                             void (*callback)(uint32_t emask, int cidx, intptr_t sid,
                                              intptr_t bytes, int len))
{
  std::vector<io_hostent> hosts;
  std::string strParams = params;
  fast_split(&strParams.front(), strParams.length(), ';', [&](char* s, char* e) {
    if (s != e)
    {
      io_hostent host;
      int idx = 0;
      fast_split(s, e - s, ':', [&](char* ss, char* ee) {
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

  yasio_shared_service->start_service(hosts, [=](event_ptr e) {
    uint32_t emask = ((e->kind() << 16) & 0xffff0000) | (e->status() & 0xffff);
    callback(emask, e->cindex(), reinterpret_cast<intptr_t>(e->transport()),
             reinterpret_cast<intptr_t>(!e->packet().empty() ? e->packet().data() : nullptr),
             static_cast<int>(e->packet().size()));
  });

  return static_cast<int>(hosts.size());
}
YASIO_NI_API void yasio_set_resolv_fn(int (*resolv)(const char* host, intptr_t sbuf))
{
  resolv_fn_t fn = [resolv](std::vector<ip::endpoint>& eps, const char* host, unsigned short port) {
    char buffer[128] = {0};
    int ret          = resolv(host, (intptr_t)&buffer[0]);
    if (0 == ret)
    {
      eps.push_back(ip::endpoint(buffer, port));
    }
    return ret;
  };
  yasio_shared_service->set_option(YOPT_S_RESOLV_FN, &fn);
}
YASIO_NI_API void yasio_set_option(int opt, const char* params)
{
  std::string strParams = params;
  auto service          = yasio_shared_service;
  switch (opt)
  {
    case YOPT_C_REMOTE_ENDPOINT: {
      int cidx = 0;
      std::string ip;
      int port = 0;
      int idx  = 0;
      fast_split(&strParams.front(), strParams.length(), ';', [&](char* s, char* e) {
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
      service->set_option(opt, cidx, ip.c_str(), port);
    }
    break;
    case YOPT_C_LFBFD_PARAMS:
    case YOPT_S_TIMEOUTS: {
      int args[YASIO_MAX_OPTION_ARGC];
      int idx    = 0;
      int limits = opt == YOPT_C_LFBFD_PARAMS ? 5 : 3;
      fast_split(&strParams.front(), strParams.length(), ';', [&](char* s, char* e) {
        auto ch = *e;
        if (idx < limits)
          args[idx] = atoi(s);
        ++idx;
        *e = ch;
      });
      if (opt == YOPT_C_LFBFD_PARAMS)
        service->set_option(opt, args[0], args[1], args[2], args[3], args[4]);
      else
        service->set_option(opt, args[0], args[1], args[2]);
      break;
    }
  }
}
YASIO_NI_API void yasio_open(int cindex, int kind) { yasio_shared_service->open(cindex, kind); }
YASIO_NI_API void yasio_close(int cindex) { yasio_shared_service->close(cindex); }
YASIO_NI_API void yasio_close_handle(intptr_t thandle)
{
  auto p = reinterpret_cast<transport_handle_t>(thandle);
  yasio_shared_service->close(p);
}
YASIO_NI_API int yasio_write(intptr_t thandle, const unsigned char* bytes, int len)
{
  std::vector<char> buf(bytes, bytes + len);
  auto p = reinterpret_cast<transport_handle_t>(thandle);
  return yasio_shared_service->write(p, std::move(buf));
}
YASIO_NI_API void yasio_dispatch(int count) { yasio_shared_service->dispatch(count); }
YASIO_NI_API void yasio_stop() { yasio_shared_service->stop_service(); }
YASIO_NI_API long long yasio_highp_time(void) { return highp_clock<system_clock_t>(); }
YASIO_NI_API long long yasio_highp_clock(void) { return highp_clock<highp_clock_t>(); }
YASIO_NI_API void yasio_set_print_fn(void (*print_fn)(const char*))
{
  yasio::inet::print_fn_t custom_print = print_fn;
  yasio_shared_service->set_option(YOPT_S_PRINT_FN, &custom_print);
}
YASIO_NI_API void yasio_memcpy(void* dst, const void* src, unsigned int len)
{
  ::memcpy(dst, src, len);
}
}

#endif
