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
#ifndef YASIO__XXSOCKET_CPP
#define YASIO__XXSOCKET_CPP
#include <assert.h>
#ifdef _DEBUG
#  include <stdio.h>
#endif

#if !defined(YASIO_HEADER_ONLY)
#  include "yasio/xxsocket.hpp"
#endif

#if !defined(_WIN32)
#  include "yasio/detail/ifaddrs.hpp"
#endif

// For apple bsd socket implemention
#if !defined(TCP_KEEPIDLE)
#  define TCP_KEEPIDLE TCP_KEEPALIVE
#endif

#define TIME_GRANULARITY 1000000

#if !defined(_WS2IPDEF_)
inline bool IN4_IS_ADDR_LOOPBACK(const in_addr* a)
{
  return ((a->s_addr & 0xff) == 0x7f); // 127/8
}
inline bool IN4_IS_ADDR_LINKLOCAL(const in_addr* a)
{
  return ((a->s_addr & 0xffff) == 0xfea9); // 169.254/16
}
inline bool IN6_IS_ADDR_GLOBAL(const in6_addr* a)
{
  //
  // Check the format prefix and exclude addresses
  // whose high 4 bits are all zero or all one.
  // This is a cheap way of excluding v4-compatible,
  // v4-mapped, loopback, multicast, link-local, site-local.
  //
  unsigned int High = (a->s6_addr[0] & 0xf0);
  return ((High != 0) && (High != 0xf0));
}
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif

using namespace yasio;
using namespace yasio::inet;

#if defined(_WIN32) && !defined(_WINSTORE)
static LPFN_ACCEPTEX __accept_ex                           = nullptr;
static LPFN_GETACCEPTEXSOCKADDRS __get_accept_ex_sockaddrs = nullptr;
static LPFN_CONNECTEX __connect_ex                         = nullptr;
#endif

namespace yasio
{
namespace inet
{
YASIO__NS_INLINE
namespace ip
{
namespace compat
{

// from glibc
#ifdef SPRINTF_CHAR
#  define SPRINTF(x) strlen(sprintf /**/ x)
#else
#  ifndef SPRINTF
#    define SPRINTF(x) (/*(size_t)*/ sprintf x)
#  endif
#endif

/*
 * Define constants based on RFC 883, RFC 1034, RFC 1035
 */
#define NS_PACKETSZ 512   /*%< default UDP packet size */
#define NS_MAXDNAME 1025  /*%< maximum domain name */
#define NS_MAXMSG 65535   /*%< maximum message size */
#define NS_MAXCDNAME 255  /*%< maximum compressed domain name */
#define NS_MAXLABEL 63    /*%< maximum length of domain label */
#define NS_HFIXEDSZ 12    /*%< #/bytes of fixed data in header */
#define NS_QFIXEDSZ 4     /*%< #/bytes of fixed data in query */
#define NS_RRFIXEDSZ 10   /*%< #/bytes of fixed data in r record */
#define NS_INT32SZ 4      /*%< #/bytes of data in a u_int32_t */
#define NS_INT16SZ 2      /*%< #/bytes of data in a u_int16_t */
#define NS_INT8SZ 1       /*%< #/bytes of data in a u_int8_t */
#define NS_INADDRSZ 4     /*%< IPv4 T_A */
#define NS_IN6ADDRSZ 16   /*%< IPv6 T_AAAA */
#define NS_CMPRSFLGS 0xc0 /*%< Flag bits indicating name compression. */
#define NS_DEFAULTPORT 53 /*%< For both TCP and UDP. */

/////////////////// inet_ntop //////////////////
/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static const char* inet_ntop4(const u_char* src, char* dst, socklen_t size);
static const char* inet_ntop6(const u_char* src, char* dst, socklen_t size);

/* char *
 * inet_ntop(af, src, dst, size)
 *	convert a network format address to presentation format.
 * return:
 *	pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *	Paul Vixie, 1996.
 */
const char* inet_ntop(int af, const void* src, char* dst, socklen_t size)
{
  switch (af)
  {
    case AF_INET:
      return (inet_ntop4((const u_char*)src, dst, size));
    case AF_INET6:
      return (inet_ntop6((const u_char*)src, dst, size));
    default:
      errno = EAFNOSUPPORT;
      return (NULL);
  }
  /* NOTREACHED */
}

/* const char *
 * inet_ntop4(src, dst, size)
 *	format an IPv4 address
 * return:
 *	`dst' (as a const)
 * notes:
 *	(1) uses no statics
 *	(2) takes a u_char* not an in_addr as input
 * author:
 *	Paul Vixie, 1996.
 */
static const char* inet_ntop4(const u_char* src, char* dst, socklen_t size)
{
  char fmt[] = "%u.%u.%u.%u";
  char tmp[sizeof "255.255.255.255"];

  if (SPRINTF((tmp, fmt, src[0], src[1], src[2], src[3])) >= static_cast<int>(size))
  {
    errno = (ENOSPC);
    return (NULL);
  }
  return strcpy(dst, tmp);
}

/* const char *
 * inet_ntop6(src, dst, size)
 *	convert IPv6 binary address into presentation (printable) format
 * author:
 *	Paul Vixie, 1996.
 */
static const char* inet_ntop6(const u_char* src, char* dst, socklen_t size)
{
  /*
   * Note that int32_t and int16_t need only be "at least" large enough
   * to contain a value of the specified size.  On some systems, like
   * Crays, there is no such thing as an integer variable with 16 bits.
   * Keep this in mind if you think this function should have been coded
   * to use pointer overlays.  All the world's not a VAX.
   */
  char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"], *tp;
  struct
  {
    int base, len;
  } best, cur;
  u_int words[NS_IN6ADDRSZ / NS_INT16SZ];
  int i;

  /*
   * Preprocess:
   *	Copy the input (bytewise) array into a wordwise array.
   *	Find the longest run of 0x00's in src[] for :: shorthanding.
   */
  memset(words, '\0', sizeof words);
  for (i = 0; i < NS_IN6ADDRSZ; i += 2)
    words[i / 2] = (src[i] << 8) | src[i + 1];
  best.base = -1;
  cur.base  = -1;
  best.len  = 0;
  cur.len   = 0;
  for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
  {
    if (words[i] == 0)
    {
      if (cur.base == -1)
        cur.base = i, cur.len = 1;
      else
        cur.len++;
    }
    else
    {
      if (cur.base != -1)
      {
        if (best.base == -1 || cur.len > best.len)
          best = cur;
        cur.base = -1;
      }
    }
  }
  if (cur.base != -1)
  {
    if (best.base == -1 || cur.len > best.len)
      best = cur;
  }
  if (best.base != -1 && best.len < 2)
    best.base = -1;

  /*
   * Format the result.
   */
  tp = tmp;
  for (i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
  {
    /* Are we inside the best run of 0x00's? */
    if (best.base != -1 && i >= best.base && i < (best.base + best.len))
    {
      if (i == best.base)
        *tp++ = ':';
      continue;
    }
    /* Are we following an initial run of 0x00s or any real hex? */
    if (i != 0)
      *tp++ = ':';
    /* Is this address an encapsulated IPv4? */
    if (i == 6 && best.base == 0 && (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
    {
      if (!inet_ntop4(src + 12, tp, static_cast<socklen_t>(sizeof tmp - (tp - tmp))))
        return (NULL);
      tp += strlen(tp);
      break;
    }
    tp += SPRINTF((tp, "%x", words[i]));
  }
  /* Was it a trailing run of 0x00's? */
  if (best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
    *tp++ = ':';
  *tp++ = '\0';

  /*
   * Check for overflow, copy, and we're done.
   */
  if ((socklen_t)(tp - tmp) > size)
  {
    errno = (ENOSPC);
    return (NULL);
  }
  return strcpy(dst, tmp);
}

/////////////////// inet_pton ///////////////////

/*
 * WARNING: Don't even consider trying to compile this on a system where
 * sizeof(int) < 4.  sizeof(int) > 4 is fine; all the world's not a VAX.
 */

static int inet_pton4(const char* src, u_char* dst);
static int inet_pton6(const char* src, u_char* dst);

/* int
 * inet_pton(af, src, dst)
 *	convert from presentation format (which usually means ASCII printable)
 *	to network format (which is usually some kind of binary format).
 * return:
 *	1 if the address was valid for the specified address family
 *	0 if the address wasn't valid (`dst' is untouched in this case)
 *	-1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *	Paul Vixie, 1996.
 */
int inet_pton(int af, const char* src, void* dst)
{
  switch (af)
  {
    case AF_INET:
      return (inet_pton4(src, (u_char*)dst));
    case AF_INET6:
      return (inet_pton6(src, (u_char*)dst));
    default:
      errno = (EAFNOSUPPORT);
      return (-1);
  }
  /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal, octal (with the
 *	exception of 0) and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int inet_pton4(const char* src, u_char* dst)
{
  int saw_digit, octets, ch;
  u_char tmp[NS_INADDRSZ], *tp;

  saw_digit   = 0;
  octets      = 0;
  *(tp = tmp) = 0;
  while ((ch = *src++) != '\0')
  {

    if (ch >= '0' && ch <= '9')
    {
      u_int newv = *tp * 10 + (ch - '0');

      if (saw_digit && *tp == 0)
        return (0);
      if (newv > 255)
        return (0);
      *tp = static_cast<u_char>(newv);
      if (!saw_digit)
      {
        if (++octets > 4)
          return (0);
        saw_digit = 1;
      }
    }
    else if (ch == '.' && saw_digit)
    {
      if (octets == 4)
        return (0);
      *++tp     = 0;
      saw_digit = 0;
    }
    else
      return (0);
  }
  if (octets < 4)
    return (0);
  memcpy(dst, tmp, NS_INADDRSZ);
  return (1);
}

/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int inet_pton6(const char* src, u_char* dst)
{
  static const char xdigits[] = "0123456789abcdef";
  u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
  const char* curtok;
  int ch, saw_xdigit;
  u_int val;

  tp     = (u_char*)memset(tmp, '\0', NS_IN6ADDRSZ);
  endp   = tp + NS_IN6ADDRSZ;
  colonp = NULL;
  /* Leading :: requires some special handling. */
  if (*src == ':')
    if (*++src != ':')
      return (0);
  curtok     = src;
  saw_xdigit = 0;
  val        = 0;
  while ((ch = tolower(*src++)) != '\0')
  {
    const char* pch;

    pch = strchr(xdigits, ch);
    if (pch != NULL)
    {
      val <<= 4;
      val |= (pch - xdigits);
      if (val > 0xffff)
        return (0);
      saw_xdigit = 1;
      continue;
    }
    if (ch == ':')
    {
      curtok = src;
      if (!saw_xdigit)
      {
        if (colonp)
          return (0);
        colonp = tp;
        continue;
      }
      else if (*src == '\0')
      {
        return (0);
      }
      if (tp + NS_INT16SZ > endp)
        return (0);
      *tp++      = (u_char)(val >> 8) & 0xff;
      *tp++      = (u_char)val & 0xff;
      saw_xdigit = 0;
      val        = 0;
      continue;
    }
    if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) && inet_pton4(curtok, tp) > 0)
    {
      tp += NS_INADDRSZ;
      saw_xdigit = 0;
      break; /* '\0' was seen by inet_pton4(). */
    }
    return (0);
  }
  if (saw_xdigit)
  {
    if (tp + NS_INT16SZ > endp)
      return (0);
    *tp++ = (u_char)(val >> 8) & 0xff;
    *tp++ = (u_char)val & 0xff;
  }
  if (colonp != NULL)
  {
    /*
     * Since some memmove()'s erroneously fail to handle
     * overlapping regions, we'll do the shift by hand.
     */
    const auto n = tp - colonp;
    int i;

    if (tp == endp)
      return (0);
    for (i = 1; i <= n; i++)
    {
      endp[-i]      = colonp[n - i];
      colonp[n - i] = 0;
    }
    tp = endp;
  }
  if (tp != endp)
    return (0);
  memcpy(dst, tmp, NS_IN6ADDRSZ);
  return (1);
}
} // namespace compat
} // namespace ip
} // namespace inet
} // namespace yasio

static int getipsv_internal(void)
{
  int flags = 0;
  int count = 0;
  /* Only windows support use getaddrinfo to get local ip address(not loopback or linklocal),
    Because nullptr same as "localhost": always return loopback address and at unix/linux the
    gethostname always return "localhost"
    */
#if defined(_WIN32)
  char hostname[256] = {0};
  gethostname(hostname, sizeof(hostname));

  // ipv4 & ipv6
  addrinfo hint, *ailist = nullptr;
  memset(&hint, 0x0, sizeof(hint));

  endpoint ep;
#  if defined(_DEBUG)
  YASIO_LOG("getipsv_internal: localhost=%s", hostname);
#  endif
  int iret = getaddrinfo(hostname, nullptr, &hint, &ailist);

  const char* errmsg = nullptr;
  if (ailist != nullptr)
  {
    for (auto aip = ailist; aip != NULL; aip = aip->ai_next)
    {
      memcpy(&ep, aip->ai_addr, aip->ai_addrlen);

      auto straddr = ep.to_string();
      YASIO_LOG("getipsv_internal: endpoint=%s", straddr.c_str());
      ++count;
      switch (ep.af())
      {
        case AF_INET:
          if (!IN4_IS_ADDR_LOOPBACK(&ep.in4_.sin_addr) && !IN4_IS_ADDR_LINKLOCAL(&ep.in4_.sin_addr))
            flags |= ipsv_ipv4;
          break;
        case AF_INET6:
          if (IN6_IS_ADDR_GLOBAL(&ep.in6_.sin6_addr))
          {
            flags |= ipsv_ipv6;
          }
          break;
      }
      if (flags == ipsv_dual_stack)
        break;
    }
    freeaddrinfo(ailist);
  }
  else
  {
    errmsg = xxsocket::gai_strerror(iret);
  }
#else // __APPLE__ or linux with <ifaddrs.h>
  struct ifaddrs *ifaddr, *ifa;
  /*
  The value of ifa->ifa_name:
   Android:
    wifi: "w"
    cellular: "r"
   iOS:
    wifi: "en0"
    cellular: "pdp_ip0"
  */

  if (getifaddrs(&ifaddr) == -1)
  {
    YASIO_LOG("getipsv_internal: getifaddrs fail!");
    return ipsv_ipv4;
  }

  endpoint ep;
  /* Walk through linked list*/
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
  {
    if (ifa->ifa_addr == NULL)
      continue;

    ep.assign(ifa->ifa_addr);

    auto straddr = ep.to_string();
    if (!straddr.empty())
    {
      ++count;
      YASIO_LOG("getipsv_internal: endpoint=%s", straddr.c_str());
    }

    switch (ep.af())
    {
      case AF_INET:
        if (!IN4_IS_ADDR_LOOPBACK(&ep.in4_.sin_addr) && !IN4_IS_ADDR_LINKLOCAL(&ep.in4_.sin_addr))
          flags |= ipsv_ipv4;
        break;
      case AF_INET6:
        if (IN6_IS_ADDR_GLOBAL(&ep.in6_.sin6_addr))
        {
          flags |= ipsv_ipv6;
        }
        break;
    }
    if (flags == ipsv_dual_stack)
      break;
  }

  freeifaddrs(ifaddr);
#endif

  YASIO_LOG("getipsv_internal: flags=%d, ifa_count=%d", flags, count);

  return flags;
}

int xxsocket::getipsv(void) { return getipsv_internal(); }

int xxsocket::xpconnect(const char* hostname, u_short port, u_short local_port)
{
  auto flags = getipsv();

  int error = -1;

  xxsocket::resolve_i(
      [&](const endpoint& ep) {
        switch (ep.af())
        {
          case AF_INET:
            if (flags & ipsv_ipv4)
            {
              error = pconnect(ep, local_port);
            }
            else if (flags & ipsv_ipv6)
            {
              xxsocket::resolve_i(
                  [&](const endpoint& ep6) { return 0 == (error = pconnect(ep6, local_port)); },
                  hostname, port, AF_INET6, AI_V4MAPPED);
            }
            break;
          case AF_INET6:
            if (flags & ipsv_ipv6)
            {
              error = pconnect(ep, local_port);
            }
            break;
        }

        return error == 0;
      },
      hostname, port, AF_UNSPEC, AI_ALL);

  return error;
}

int xxsocket::xpconnect_n(const char* hostname, u_short port,
                          const std::chrono::microseconds& wtimeout, u_short local_port)
{
  auto flags = getipsv();

  int error = -1;

  xxsocket::resolve_i(
      [&](const endpoint& ep) {
        switch (ep.af())
        {
          case AF_INET:
            if (flags & ipsv_ipv4)
            {
              error = pconnect_n(ep, wtimeout, local_port);
            }
            else if (flags & ipsv_ipv6)
            {
              xxsocket::resolve_i(
                  [&](const endpoint& ep6) {
                    return 0 == (error = pconnect_n(ep6, wtimeout, local_port));
                  },
                  hostname, port, AF_INET6, AI_V4MAPPED);
            }
            break;
          case AF_INET6:
            if (flags & ipsv_ipv6)
            {
              error = pconnect_n(ep, wtimeout, local_port);
            }
            break;
        }

        return error == 0;
      },
      hostname, port, AF_UNSPEC, AI_ALL);

  return error;
}

int xxsocket::pconnect(const char* hostname, u_short port, u_short local_port)
{
  int error = -1;
  xxsocket::resolve_i([&](const endpoint& ep) { return 0 == (error = pconnect(ep, local_port)); },
                      hostname, port);
  return error;
}

int xxsocket::pconnect_n(const char* hostname, u_short port,
                         const std::chrono::microseconds& wtimeout, u_short local_port)
{
  int error = -1;
  xxsocket::resolve_i(
      [&](const endpoint& ep) { return 0 == (error = pconnect_n(ep, wtimeout, local_port)); },
      hostname, port);
  return error;
}

int xxsocket::pconnect_n(const char* hostname, u_short port, u_short local_port)
{
  int error = -1;
  xxsocket::resolve_i(
      [&](const endpoint& ep) {
        (error = pconnect_n(ep, local_port));
        return true;
      },
      hostname, port);
  return error;
}

int xxsocket::pconnect(const endpoint& ep, u_short local_port)
{
  if (this->reopen(ep.af()))
  {
    if (local_port != 0)
      this->bind("0.0.0.0", local_port);
    return this->connect(ep);
  }
  return -1;
}

int xxsocket::pconnect_n(const endpoint& ep, const std::chrono::microseconds& wtimeout,
                         u_short local_port)
{
  if (this->reopen(ep.af()))
  {
    if (local_port != 0)
      this->bind("0.0.0.0", local_port);
    return this->connect_n(ep, wtimeout);
  }
  return -1;
}

int xxsocket::pconnect_n(const endpoint& ep, u_short local_port)
{
  if (this->reopen(ep.af()))
  {
    if (local_port != 0)
      this->bind("0.0.0.0", local_port);
    return xxsocket::connect_n(this->fd, ep);
  }
  return -1;
}

int xxsocket::pserv(const char* addr, u_short port)
{
  endpoint local(addr, port);

  if (!this->reopen(local.af()))
  {
    return -1;
  }

  set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

  int n = this->bind(local);
  if (n != 0)
    return n;

  return this->listen();
}

int xxsocket::resolve(std::vector<endpoint>& endpoints, const char* hostname, unsigned short port)
{
  return resolve_i(
      [&](const endpoint& ep) {
        endpoints.push_back(ep);
        return false;
      },
      hostname, port, AF_UNSPEC, AI_ALL);
}

int xxsocket::resolve_v4(std::vector<endpoint>& endpoints, const char* hostname,
                         unsigned short port)
{
  return resolve_i(
      [&](const endpoint& ep) {
        endpoints.push_back(ep);
        return false;
      },
      hostname, port, AF_INET, 0);
}

int xxsocket::resolve_v6(std::vector<endpoint>& endpoints, const char* hostname,
                         unsigned short port)
{
  return resolve_i(
      [&](const endpoint& ep) {
        endpoints.push_back(ep);
        return false;
      },
      hostname, port, AF_INET6, 0);
}

int xxsocket::resolve_v4to6(std::vector<endpoint>& endpoints, const char* hostname,
                            unsigned short port)
{
  return xxsocket::resolve_i(
      [&](const endpoint& ep) {
        endpoints.push_back(ep);
        return false;
      },
      hostname, port, AF_INET6, AI_V4MAPPED);
}

int xxsocket::force_resolve_v6(std::vector<endpoint>& endpoints, const char* hostname,
                               unsigned short port)
{
  return resolve_i(
      [&](const endpoint& ep) {
        endpoints.push_back(ep);
        return false;
      },
      hostname, port, AF_INET6, AI_ALL | AI_V4MAPPED);
}

xxsocket::xxsocket(void) : fd(invalid_socket) {}

xxsocket::xxsocket(socket_native_type h) : fd(h) {}

xxsocket::xxsocket(xxsocket&& right) : fd(invalid_socket) { swap(right); }

xxsocket& xxsocket::operator=(socket_native_type handle)
{
  if (!this->is_open())
  {
    this->fd = handle;
  }
  return *this;
}

xxsocket& xxsocket::operator=(xxsocket&& right) { return swap(right); }

xxsocket::xxsocket(int af, int type, int protocol) : fd(invalid_socket)
{
  open(af, type, protocol);
}

xxsocket::~xxsocket(void) { close(); }

xxsocket& xxsocket::swap(xxsocket& rhs)
{
  std::swap(this->fd, rhs.fd);
  return *this;
}

bool xxsocket::open(int af, int type, int protocol)
{
  if (invalid_socket == this->fd)
  {
    this->fd = ::socket(af, type, protocol);
  }
  return is_open();
}

bool xxsocket::reopen(int af, int type, int protocol)
{
  this->close();
  return this->open(af, type, protocol);
}

#if defined(_WIN32) && !defined(_WINSTORE)
bool xxsocket::open_ex(int af, int type, int protocol)
{
#  if !defined(WP8)
  if (invalid_socket == this->fd)
  {
    this->fd = ::WSASocket(af, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);

    DWORD dwBytes = 0;
    if (nullptr == __accept_ex)
    {
      GUID guidAcceptEx = WSAID_ACCEPTEX;
      (void)WSAIoctl(this->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidAcceptEx,
                     sizeof(guidAcceptEx), &__accept_ex, sizeof(__accept_ex), &dwBytes, nullptr,
                     nullptr);
    }

    if (nullptr == __connect_ex)
    {
      GUID guidConnectEx = WSAID_CONNECTEX;
      (void)WSAIoctl(this->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidConnectEx,
                     sizeof(guidConnectEx), &__connect_ex, sizeof(__connect_ex), &dwBytes, nullptr,
                     nullptr);
    }

    if (nullptr == __get_accept_ex_sockaddrs)
    {
      GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
      (void)WSAIoctl(this->fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &guidGetAcceptExSockaddrs,
                     sizeof(guidGetAcceptExSockaddrs), &__get_accept_ex_sockaddrs,
                     sizeof(__get_accept_ex_sockaddrs), &dwBytes, nullptr, nullptr);
    }
  }
  return is_open();
#  else
  return false;
#  endif
}

#  if !defined(WP8)
bool xxsocket::accept_ex(SOCKET sockfd_listened, SOCKET sockfd_prepared, PVOID lpOutputBuffer,
                         DWORD dwReceiveDataLength, DWORD dwLocalAddressLength,
                         DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived,
                         LPOVERLAPPED lpOverlapped)
{
  return __accept_ex(sockfd_listened, sockfd_prepared, lpOutputBuffer, dwReceiveDataLength,
                     dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived,
                     lpOverlapped) != FALSE;
}

bool xxsocket::connect_ex(SOCKET s, const struct sockaddr* name, int namelen, PVOID lpSendBuffer,
                          DWORD dwSendDataLength, LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped)
{
  return __connect_ex(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent,
                      lpOverlapped);
}

void xxsocket::translate_sockaddrs(PVOID lpOutputBuffer, DWORD dwReceiveDataLength,
                                   DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength,
                                   sockaddr** LocalSockaddr, LPINT LocalSockaddrLength,
                                   sockaddr** RemoteSockaddr, LPINT RemoteSockaddrLength)
{
  __get_accept_ex_sockaddrs(lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength,
                            dwRemoteAddressLength, LocalSockaddr, LocalSockaddrLength,
                            RemoteSockaddr, RemoteSockaddrLength);
}
#  endif

#endif

bool xxsocket::is_open(void) const { return this->fd != invalid_socket; }

socket_native_type xxsocket::detach(void)
{
  socket_native_type result = this->fd;
  this->fd                  = invalid_socket;
  return result;
}

socket_native_type xxsocket::native_handle(void) const { return this->fd; }

int xxsocket::set_nonblocking(bool nonblocking) const
{
  return set_nonblocking(this->fd, nonblocking);
}

int xxsocket::set_nonblocking(socket_native_type s, bool nonblocking)
{
  u_long argp = nonblocking;
  return ::ioctlsocket(s, FIONBIO, &argp);
}

int xxsocket::bind(const char* addr, unsigned short port) const
{
  return this->bind(endpoint(addr, port));
}

int xxsocket::bind(const endpoint& ep) const
{
  return ::bind(this->fd, &ep.sa_, ep.af() == AF_INET6 ? sizeof(ep.in6_) : sizeof(ep.in4_));
}

int xxsocket::bind_any(bool ipv6) const
{
  return this->bind(endpoint(!ipv6 ? "0.0.0.0" : "::", 0));
}

int xxsocket::listen(int backlog) const { return ::listen(this->fd, backlog); }

xxsocket xxsocket::accept(socklen_t) { return ::accept(this->fd, nullptr, nullptr); }

int xxsocket::accept_n(socket_native_type& new_sock)
{
  for (;;)
  {
    // Accept the waiting connection.
    new_sock = ::accept(this->fd, nullptr, nullptr);

    // Check if operation succeeded.
    if (new_sock != invalid_socket)
      return 0;

    auto ec = get_last_errno();

    // Retry operation if interrupted by signal.
    if (ec == EINTR)
      continue;

    /* Operation failed.
    ** The ec maybe EWOULDBLOCK, EAGAIN, ECONNABORTED, EPROTO,
    ** Simply Fall through to retry operation.
    */
    return ec;
  }
}

int xxsocket::connect(const char* addr, u_short port) { return connect(endpoint(addr, port)); }

int xxsocket::connect(const endpoint& ep) { return xxsocket::connect(fd, ep); }

int xxsocket::connect(socket_native_type s, const char* addr, u_short port)
{
  endpoint peer(addr, port);

  return xxsocket::connect(s, peer);
}

int xxsocket::connect(socket_native_type s, const endpoint& ep)
{
  return ::connect(s, &ep.sa_, ep.af() == AF_INET6 ? sizeof(ep.in6_) : sizeof(ep.in4_));
}

int xxsocket::connect_n(const char* addr, u_short port, const std::chrono::microseconds& wtimeout)
{
  timeval timeout = {static_cast<decltype(timeval::tv_sec)>(wtimeout.count() / TIME_GRANULARITY),
                     static_cast<decltype(timeval::tv_usec)>(wtimeout.count() % TIME_GRANULARITY)};
  return connect_n(addr, port, &timeout);
}

int xxsocket::connect_n(const endpoint& ep, const std::chrono::microseconds& wtimeout)
{
  timeval timeout = {static_cast<decltype(timeval::tv_sec)>(wtimeout.count() / TIME_GRANULARITY),
                     static_cast<decltype(timeval::tv_usec)>(wtimeout.count() % TIME_GRANULARITY)};
  return connect_n(ep, &timeout);
}

int xxsocket::connect_n(const char* addr, u_short port, timeval* timeout)
{
  return connect_n(endpoint(addr, port), timeout);
}

int xxsocket::connect_n(const endpoint& ep, timeval* timeout)
{
  if (xxsocket::connect_n(this->fd, ep, timeout) != 0)
  {
    this->fd = invalid_socket;
    return -1;
  }
  return 0;
}

int xxsocket::connect_n(socket_native_type s, const char* addr, u_short port, timeval* timeout)
{
  return connect_n(s, endpoint(addr, port), timeout);
}

int xxsocket::connect_n(socket_native_type s, const endpoint& ep, timeval* timeout)
{
  fd_set rset, wset;
  int n, error = 0;
#ifdef _WIN32
  set_nonblocking(s, true);
#else
  int flags = ::fcntl(s, F_GETFL, 0);
  ::fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
  if ((n = xxsocket::connect(s, ep)) < 0)
  {
    error = xxsocket::get_last_errno();
    if (error != EINPROGRESS && error != EWOULDBLOCK)
      return -1;
  }

  /* Do whatever we want while the connect is taking place. */
  if (n == 0)
    goto done; /* connect completed immediately */

  FD_ZERO(&rset);
  FD_SET(s, &rset);
  wset = rset;

  if ((n = ::select(static_cast<int>(s + 1), &rset, &wset, NULL, timeout)) == 0)
  {
    ::closesocket(s); /* timeout */
    xxsocket::set_last_errno(ETIMEDOUT);
    return (-1);
  }

  if (FD_ISSET(s, &rset) || FD_ISSET(s, &wset))
  {
    socklen_t len = sizeof(error);
    if (::getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
      return (-1); /* Solaris pending error */
  }
  else
    return -1;
done:
  if (error != 0)
  {
    ::closesocket(s); /* just in case */
    xxsocket::set_last_errno(error);
    return (-1);
  }

  /* restore file status flags */
#ifdef _WIN32
  // pitfall: because on win32, there is no way to test whether the s is non-blocking
  // so, can't restore properly
  set_nonblocking(s, false);
#else
  ::fcntl(s, F_SETFL, flags);
#endif
  return (0);
}

int xxsocket::connect_n(const endpoint& ep) { return xxsocket::connect_n(this->fd, ep); }
int xxsocket::connect_n(socket_native_type s, const endpoint& ep)
{
#ifdef _WIN32
  set_nonblocking(s, true);
#else
  int flags = ::fcntl(s, F_GETFL, 0);
  ::fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif

  return xxsocket::connect(s, ep);
}

int xxsocket::send(const void* buf, int len, int flags) const
{
  int bytes_transferred = 0;
  int n                 = 0;
  do
  {
    bytes_transferred += (n = ::send(this->fd, (const char*)buf + bytes_transferred,
                                     len - bytes_transferred, flags));
  } while (bytes_transferred < len && n > 0);
  return bytes_transferred;
}

int xxsocket::send_n(const void* buf, int len, const std::chrono::microseconds& wtimeout, int flags)
{
  timeval timeout = {static_cast<decltype(timeval::tv_sec)>(wtimeout.count() / TIME_GRANULARITY),
                     static_cast<decltype(timeval::tv_usec)>(wtimeout.count() % TIME_GRANULARITY)};
  return send_n(this->fd, buf, len, &timeout, flags);
}

int xxsocket::send_n(const void* buf, int len, timeval* timeout, int flags)
{
  return xxsocket::send_n(this->fd, buf, len, timeout, flags);
}

int xxsocket::send_n(socket_native_type s, const void* buf, int len, timeval* timeout, int flags)
{
  int bytes_transferred;
  int n;
  int errcode = 0;

  for (bytes_transferred = 0; bytes_transferred < len; bytes_transferred += n)
  {
    // Try to transfer as much of the remaining data as possible.
    // Since the socket is in non-blocking mode, this call will not
    // block.
    n = xxsocket::send_i(s, (const char*)buf + bytes_transferred, len - bytes_transferred, flags);
    //++send_times;
    // Check for errors.
    if (n <= 0)
    {
      // Check for possible blocking.
#ifdef _WIN32
      errcode = WSAGetLastError();
#else
      errcode = errno;
#endif
      if (n == -1 &&
          (errcode == EAGAIN || errcode == EWOULDBLOCK || errcode == ENOBUFS || errcode == EINTR))
      {
        // Wait upto <timeout> for the blocking to subside.
        int const rtn = handle_write_ready(s, timeout);

        // Did select() succeed?
        if (rtn != -1)
        {
          // Blocking subsided in <timeout> period.  Continue
          // data transfer.
          n = 0;
          continue;
        }
      }

      // Wait in select() timed out or other data transfer or
      // select() failures.
      return n;
    }
  }

  return bytes_transferred;
}

int xxsocket::recv(void* buf, int len, int flags) const
{
  int bytes_transfrred = 0;
  int n                = 0;
  do
  {
    bytes_transfrred +=
        (n = ::recv(this->fd, (char*)buf + bytes_transfrred, len - bytes_transfrred, flags));

  } while (bytes_transfrred < len && n > 0);
  return bytes_transfrred;
}

int xxsocket::recv_n(void* buf, int len, const std::chrono::microseconds& wtimeout, int flags) const
{
  timeval timeout = {static_cast<decltype(timeval::tv_sec)>(wtimeout.count() / TIME_GRANULARITY),
                     static_cast<decltype(timeval::tv_usec)>(wtimeout.count() % TIME_GRANULARITY)};
  return recv_n(this->fd, buf, len, &timeout, flags);
}

int xxsocket::recv_n(void* buf, int len, timeval* timeout, int flags) const
{
  return recv_n(this->fd, buf, len, timeout, flags);
}

int xxsocket::recv_n(socket_native_type s, void* buf, int len, timeval* timeout, int flags)
{
  int bytes_transferred;
  int n;
  int ec = 0;

  for (bytes_transferred = 0; bytes_transferred < len; bytes_transferred += n)
  {
    // Try to transfer as much of the remaining data as possible.
    // Since the socket is in non-blocking mode, this call will not
    // block.
    n = recv_i(s, static_cast<char*>(buf) + bytes_transferred, len - bytes_transferred, flags);

    // Check for errors.
    if (n <= 0)
    {
      // Check for possible blocking.
#ifdef _WIN32
      ec = WSAGetLastError();
#else
      ec      = errno; // socket errno
#endif
      if (n == -1 && (ec == EAGAIN || ec == EINTR || ec == EWOULDBLOCK || ec == EINPROGRESS))
      {
        // Wait upto <timeout> for the blocking to subside.
        int const rtn = handle_read_ready(s, timeout);

        // Did select() succeed?
        if (rtn != -1)
        {
          // Blocking subsided in <timeout> period.  Continue
          // data transfer.
          n = 0;
          continue;
        }
      }

      // Wait in select() timed out or other data transfer or
      // select() failures.
      return n;
    }
  }

  return bytes_transferred;
}

bool xxsocket::read_until(std::string& buffer, const char delim)
{
  return read_until(buffer, &delim, sizeof(delim));
}

bool xxsocket::read_until(std::string& buffer, const std::string& delims)
{
  return read_until(buffer, delims.c_str(), delims.size());
}

bool xxsocket::read_until(std::string& buffer, const char* delims, size_t len)
{
  if (len == static_cast<size_t>(-1))
    len = strlen(delims);

  bool ok = false;
  char buf[128];
  int retry = 3; // retry three times
  int n     = 0;
  for (; retry > 0;)
  {
    memset(buf, 0, sizeof(buf));
    n = recv_i(buf, sizeof(buf));
    if (n <= 0)
    {
      auto error = xxsocket::get_last_errno();
      if (n == -1 &&
          (error == EAGAIN || error == EINTR || error == EWOULDBLOCK || error == EINPROGRESS))
      {
        timeval tv = {3, 500000};
        int rtn    = handle_read_ready(&tv);

        if (rtn != -1)
        { // read ready
          continue;
        }
      }

      // read not ready, retry.
      --retry;
      continue;
    }

    buffer.append(buf, n);
    if (buffer.size() >= len)
    {
      auto eof = &buffer[buffer.size() - len];
      if (0 == memcmp(eof, delims, len))
      {
        ok = true;
        break;
      }
    }
  }

  return ok;
}

int xxsocket::send_i(const void* buf, int len, int flags) const
{
  return ::send(this->fd, (const char*)buf, len, flags);
}

int xxsocket::send_i(socket_native_type s, const void* buf, int len, int flags)
{
  return ::send(s, (const char*)buf, len, flags);
}

int xxsocket::recv_i(void* buf, int len, int flags) const
{
  return recv_i(this->fd, buf, len, flags);
}

int xxsocket::recv_i(socket_native_type s, void* buf, int len, int flags)
{
  return ::recv(s, (char*)buf, len, flags);
}

int xxsocket::recvfrom_i(void* buf, int len, endpoint& from, int flags) const
{
  socklen_t addrlen = sizeof(from);
  return ::recvfrom(this->fd, (char*)buf, len, flags, &from.sa_, &addrlen);
}

int xxsocket::sendto_i(const void* buf, int len, const endpoint& to, int flags) const
{
  return ::sendto(this->fd, (const char*)buf, len, flags, &to.sa_,
                  to.af() == AF_INET6 ? sizeof(to.in6_) : sizeof(to.in4_));
}

int xxsocket::handle_write_ready(timeval* timeo) const
{
  return handle_write_ready(this->fd, timeo);
}

int xxsocket::handle_write_ready(socket_native_type s, timeval* timeo)
{
  fd_set fds_wr;
  FD_ZERO(&fds_wr);
  FD_SET(s, &fds_wr);
  int ret = ::select(static_cast<int>(s + 1), nullptr, &fds_wr, nullptr, timeo);
  return ret;
}

int xxsocket::handle_connect_ready(socket_native_type s, timeval* timeo)
{
  fd_set fds_wr;
  FD_ZERO(&fds_wr);
  FD_SET(s, &fds_wr);

  if (::select(0, nullptr, &fds_wr, nullptr, timeo) > 0 && FD_ISSET(s, &fds_wr))
  { // connect successfully
    return 0;
  }
  return -1;
}

int xxsocket::handle_read_ready(const std::chrono::microseconds& wtimeout) const
{
  return handle_read_ready(this->fd, wtimeout);
}

int xxsocket::handle_read_ready(timeval* timeo) const { return handle_read_ready(this->fd, timeo); }

int xxsocket::handle_read_ready(socket_native_type s, const std::chrono::microseconds& wtimeout)
{
  timeval timeout = {static_cast<decltype(timeval::tv_sec)>(wtimeout.count() / TIME_GRANULARITY),
                     static_cast<decltype(timeval::tv_usec)>(wtimeout.count() % TIME_GRANULARITY)};
  return handle_read_ready(s, &timeout);
}

int xxsocket::handle_read_ready(socket_native_type s, timeval* timeo)
{
  fd_set fds_rd;
  FD_ZERO(&fds_rd);
  FD_SET(s, &fds_rd);
  int ret = ::select(static_cast<int>(s + 1), &fds_rd, nullptr, nullptr, timeo);
  return ret;
}

endpoint xxsocket::local_endpoint(void) const { return local_endpoint(this->fd); }

endpoint xxsocket::local_endpoint(socket_native_type fd)
{
  endpoint ep;
  socklen_t socklen = sizeof(ep);
  getsockname(fd, &ep.sa_, &socklen);
  return ep;
}

endpoint xxsocket::peer_endpoint(void) const { return peer_endpoint(this->fd); }

endpoint xxsocket::peer_endpoint(socket_native_type fd)
{
  endpoint ep;
  socklen_t socklen = sizeof(ep);
  getpeername(fd, &ep.sa_, &socklen);
  return ep;
}

int xxsocket::set_keepalive(int flag, int idle, int interval, int probes)
{
  return set_keepalive(this->fd, flag, idle, interval, probes);
}

int xxsocket::set_keepalive(socket_native_type s, int flag, int idle, int interval, int probes)
{
#if defined(_WIN32) && !defined(WP8) && !defined(_WINSTORE)
  tcp_keepalive buffer_in;
  buffer_in.onoff             = flag;
  buffer_in.keepalivetime     = idle * 1000;
  buffer_in.keepaliveinterval = interval * 1000;

  return WSAIoctl(s, SIO_KEEPALIVE_VALS, &buffer_in, sizeof(buffer_in), nullptr, 0, (DWORD*)&probes,
                  nullptr, nullptr);
#else
  int n = set_optval(s, SOL_SOCKET, SO_KEEPALIVE, flag);
  n += set_optval(s, IPPROTO_TCP, TCP_KEEPIDLE, idle);
  n += set_optval(s, IPPROTO_TCP, TCP_KEEPINTVL, interval);
  n += set_optval(s, IPPROTO_TCP, TCP_KEEPCNT, probes);
  return n;
#endif
}

xxsocket::operator socket_native_type(void) const { return this->fd; }

bool xxsocket::alive(void) const { return this->send_i("", 0) != -1; }

int xxsocket::shutdown(int how) const { return ::shutdown(this->fd, how); }

void xxsocket::close(void)
{
  if (is_open())
  {
    ::closesocket(this->fd);
    this->fd = invalid_socket;
  }
}

void xxsocket::init_ws32_lib(void) {}

int xxsocket::get_last_errno(void)
{
#if defined(_WIN32)
  return ::WSAGetLastError();
#else
  return errno;
#endif
}

void xxsocket::set_last_errno(int error)
{
#if defined(_WIN32)
  ::WSASetLastError(error);
#else
  errno = error;
#endif
}

const char* xxsocket::strerror(int error)
{
#if defined(_MSC_VER) && !defined(_WINSTORE)
  static char error_msg[256];
  ZeroMemory(error_msg, sizeof(error_msg));
  ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
                       FORMAT_MESSAGE_MAX_WIDTH_MASK /* remove line-end charactors \r\n */,
                   NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // english language
                   error_msg, sizeof(error_msg), NULL);

  return error_msg;
#else
  return ::strerror(error);
#endif
}

const char* xxsocket::gai_strerror(int error)
{
#if defined(_WIN32)
  return xxsocket::strerror(error);
#else
  return ::gai_strerror(error);
#endif
}

// initialize win32 socket library
#ifdef _WIN32
namespace
{
struct ws2_32_gc
{
  ws2_32_gc(void)
  {
    WSADATA dat = {0};
    WSAStartup(0x0202, &dat);
  }
  ~ws2_32_gc(void) { WSACleanup(); }
};

ws2_32_gc __ws32_lib_gc;
} // namespace
#endif

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif
