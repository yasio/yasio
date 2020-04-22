//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
//
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

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

Copy of
https://github.com/xamarin/xamarin-android/blob/master/src/monodroid/jni/xamarin_getifaddrs.h
*/
#ifndef YASIO__IFADDRS_HPP
#define YASIO__IFADDRS_HPP

#include "yasio/detail/config.hpp"

#if !(defined(ANDROID) || defined(__ANDROID__)) || __ANDROID_API__ >= 24
#  include <ifaddrs.h>
namespace yasio
{
using ::freeifaddrs;
using ::getifaddrs;
} // namespace yasio
#else

/* We're implementing getifaddrs behavior, this is the structure we use. It is exactly the same as
 * struct ifaddrs defined in ifaddrs.h but since bionics doesn't have it we need to mirror it here.
 */
struct ifaddrs
{
  struct ifaddrs* ifa_next; /* Pointer to the next structure.      */

  char* ifa_name;         /* Name of this network interface.     */
  unsigned int ifa_flags; /* Flags as from SIOCGIFFLAGS ioctl.   */

  struct sockaddr* ifa_addr;    /* Network address of this interface.  */
  struct sockaddr* ifa_netmask; /* Netmask of this interface.          */
  union
  {
    /* At most one of the following two is valid.  If the IFF_BROADCAST
       bit is set in `ifa_flags', then `ifa_broadaddr' is valid.  If the
       IFF_POINTOPOINT bit is set, then `ifa_dstaddr' is valid.
       It is never the case that both these bits are set at once.  */
    struct sockaddr* ifu_broadaddr; /* Broadcast address of this interface. */
    struct sockaddr* ifu_dstaddr;   /* Point-to-point destination address.  */
  } ifa_ifu;
  /* These very same macros are defined by <net/if.h> for `struct ifaddr'.
     So if they are defined already, the existing definitions will be fine.  */
#  ifndef ifa_broadaddr
#    define ifa_broadaddr ifa_ifu.ifu_broadaddr
#  endif
#  ifndef ifa_dstaddr
#    define ifa_dstaddr ifa_ifu.ifu_dstaddr
#  endif
  void* ifa_data; /* Address-specific data (may be unused).  */
};

namespace yasio
{
inline int getifaddrs(struct ifaddrs** ifap);
inline void freeifaddrs(struct ifaddrs* ifa);
} // namespace yasio
#  include "yasio/detail/ifaddrs.ipp"

#endif

#endif
