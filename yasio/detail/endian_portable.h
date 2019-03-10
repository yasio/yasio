#if !defined(_ENDIAN_PORTABLE_H_)
#define _ENDIAN_PORTABLE_H_

#include "politedef.h"
#include <assert.h>

#if !defined(ASSERT)
#  if defined(_DEBUG)
#    define ASSERT(exp) assert(exp)
#  else
#    define ASSERT(exp) ((void)0)
#  endif
#endif

#ifdef _WIN32
#  include <WinSock2.h>

#  include <Windows.h>
#else
#  include <arpa/inet.h>
#endif

#if !defined(_MSC_VER) || (defined(_MSC_VER) && _MSC_VER < 1800) ||                                \
    (NTDDI_VERSION <= 0x06010000 && !defined(WINRT))
/*
 * Byte order conversion functions for 64-bit integers and 32 + 64 bit
 * floating-point numbers.  IEEE big-endian format is used for the
 * network floating point format.
 */
#  define _WS2_32_WINSOCK_SWAP_LONG(l)                                                             \
    ((((l) >> 24) & 0x000000FFL) | (((l) >> 8) & 0x0000FF00L) | (((l) << 8) & 0x00FF0000L) |       \
     (((l) << 24) & 0xFF000000L))

#  define _WS2_32_WINSOCK_SWAP_LONGLONG(l)                                                         \
    ((((l) >> 56) & 0x00000000000000FFLL) | (((l) >> 40) & 0x000000000000FF00LL) |                 \
     (((l) >> 24) & 0x0000000000FF0000LL) | (((l) >> 8) & 0x00000000FF000000LL) |                  \
     (((l) << 8) & 0x000000FF00000000LL) | (((l) << 24) & 0x0000FF0000000000LL) |                  \
     (((l) << 40) & 0x00FF000000000000LL) | (((l) << 56) & 0xFF00000000000000LL))

#  ifndef htonll
inline uint64_t htonll(uint64_t Value)
{
  const uint64_t Retval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
  return Retval;
}
#  endif /* htonll */

#  ifndef ntohll
inline uint64_t ntohll(uint64_t Value)
{
  const uint64_t Retval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
  return Retval;
}
#  endif /* ntohll */

#  ifndef htonf
inline uint32_t htonf(float Value)
{
  uint32_t Tempval;
  uint32_t Retval;
  Tempval = *(uint32_t *)(&Value);
  Retval  = _WS2_32_WINSOCK_SWAP_LONG(Tempval);
  return Retval;
}
#  endif /* htonf */

#  ifndef ntohf
inline float ntohf(uint32_t Value)
{
  const uint32_t Tempval = _WS2_32_WINSOCK_SWAP_LONG(Value);
  float Retval;
  *((uint32_t *)&Retval) = Tempval;
  return Retval;
}
#  endif /* ntohf */

#  ifndef htond
inline uint64_t htond(double Value)
{
  uint64_t Tempval;
  uint64_t Retval;
  Tempval = *(uint64_t *)(&Value);
  Retval  = _WS2_32_WINSOCK_SWAP_LONGLONG(Tempval);
  return Retval;
}
#  endif /* htond */

#  ifndef ntohd
inline double ntohd(uint64_t Value)
{
  const uint64_t Tempval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
  double Retval;
  *((uint64_t *)&Retval) = Tempval;
  return Retval;
}
#  endif /* ntohd */
#endif   /* NO_EXTRA_HTON_FUNCTIONS */

namespace purelib
{
namespace endian
{

#define htonv ntohv
template <typename _Int> inline _Int ntohv(_Int value) { return ntohl(value); }

template <> inline bool ntohv(bool value) { return value; }

template <> inline uint8_t ntohv(uint8_t value) { return value; }

template <> inline uint16_t ntohv(uint16_t value) { return htons(value); }

template <> inline uint32_t ntohv(uint32_t value) { return htonl(value); }

template <> inline uint64_t ntohv(uint64_t value) { return htonll(value); }

template <> inline int8_t ntohv(int8_t value) { return value; }

template <> inline int16_t ntohv(int16_t value) { return htons(value); }

template <> inline int32_t ntohv(int32_t value) { return htonl(value); }

template <> inline int64_t ntohv(int64_t value) { return htonll(value); }

}; // namespace endian

namespace bits
{
static const unsigned char bits_wmask_table[8][8] = {
    {0xFE /*11111110*/},
    {0xFD /*11111101*/, 0xFC /*11111100*/},
    {0xFB /*11111011*/, 0xF9 /*11111001*/, 0xF8 /*11111000*/},
    {0xF7 /*11110111*/, 0xF3 /*11110011*/, 0xF1 /*11110001*/, 0xF0 /*11110000*/},
    {0xEF /*11101111*/, 0xE7 /*11100111*/, 0xE3 /*11100011*/, 0xE1 /*11100001*/, 0xE0 /*11100000*/},
    {0xDF /*11011111*/, 0xCF /*11001111*/, 0xC7 /*11000111*/, 0xC3 /*11000011*/, 0xC1 /*11000001*/,
     0xC0 /*11000000*/},
    {0xBF /*10111111*/, 0x9F /*10011111*/, 0x8F /*10001111*/, 0x87 /*10000111*/, 0x83 /*10000011*/,
     0x81 /*10000001*/, 0x80 /*10000000*/},
    {0x7F /*01111111*/, 0x3F /*00111111*/, 0x1F /*00011111*/, 0x0F /*00001111*/, 0x07 /*00000111*/,
     0x03 /*00000011*/, 0x01 /*00000001*/, 0x00 /*00000000*/}};

static const unsigned char bits_rmask_table[8] = {
    0x01 /*00000001*/, 0x03 /*00000011*/, 0x07 /*00000111*/, 0x0F /*00001111*/,
    0x1F /*00011111*/, 0x3F /*00111111*/, 0x7F /*01111111*/, 0xFF /*11111111*/
};

inline void set_bits_value(void *pByteValue, unsigned int pos, unsigned char bitsValue,
                           unsigned int bits)
{
  ASSERT(bits > 0 && bits <= (pos + 1) && pos < 8);

  *((unsigned char *)pByteValue) =
      ((*((unsigned char *)pByteValue) & bits_wmask_table[pos][bits - 1]) |
       (bitsValue << (pos + 1 - bits)));
}

inline unsigned char get_bits_value(unsigned char byteValue, unsigned int pos, unsigned int bits)
{
  ASSERT(bits > 0 && bits <= (pos + 1) && pos < 8);

  return ((byteValue & bits_rmask_table[pos]) >> (pos + 1 - bits));
}
}; // namespace bits

}; // namespace purelib

/*
 * Copyright (c) 2012-2019 by HALX99  ALL RIGHTS RESERVED.
 * Consult your license regarding permissions and restrictions.
 **/
#endif
