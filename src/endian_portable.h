#ifndef _XXENDIAN_PORTABLE_H_
#define _XXENDIAN_PORTABLE_H_
#include "politedef.h"

#ifdef _WIN32
#include <WinSock2.h>

#include <Windows.h>
#else
#include <arpa/inet.h>
#endif

#if !defined(_MSC_VER) || (defined(_MSC_VER) && _MSC_VER < 1800) || (NTDDI_VERSION <= 0x06010000 && !defined(WINRT)) 
/*
* Byte order conversion functions for 64-bit integers and 32 + 64 bit
* floating-point numbers.  IEEE big-endian format is used for the
* network floating point format.
*/
#define _WS2_32_WINSOCK_SWAP_LONG(l)                \
            ( ( ((l) >> 24) & 0x000000FFL ) |       \
              ( ((l) >>  8) & 0x0000FF00L ) |       \
              ( ((l) <<  8) & 0x00FF0000L ) |       \
              ( ((l) << 24) & 0xFF000000L ) )

#define _WS2_32_WINSOCK_SWAP_LONGLONG(l)            \
            ( ( ((l) >> 56) & 0x00000000000000FFLL ) |       \
              ( ((l) >> 40) & 0x000000000000FF00LL ) |       \
              ( ((l) >> 24) & 0x0000000000FF0000LL ) |       \
              ( ((l) >>  8) & 0x00000000FF000000LL ) |       \
              ( ((l) <<  8) & 0x000000FF00000000LL ) |       \
              ( ((l) << 24) & 0x0000FF0000000000LL ) |       \
              ( ((l) << 40) & 0x00FF000000000000LL ) |       \
              ( ((l) << 56) & 0xFF00000000000000LL ) )


#ifndef htonll
inline uint64_t htonll(uint64_t Value)
{
	const uint64_t Retval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
	return Retval;
}
#endif /* htonll */

#ifndef ntohll
inline uint64_t ntohll(uint64_t Value)
{
	const uint64_t Retval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
	return Retval;
}
#endif /* ntohll */

#ifndef htonf
inline uint32_t htonf(float Value)
{
	uint32_t Tempval;
	uint32_t Retval;
	Tempval = *(uint32_t*)(&Value);
	Retval = _WS2_32_WINSOCK_SWAP_LONG(Tempval);
	return Retval;
}
#endif /* htonf */

#ifndef ntohf
inline float ntohf(uint32_t Value)
{
	const uint32_t Tempval = _WS2_32_WINSOCK_SWAP_LONG(Value);
	float Retval;
	*((uint32_t*)&Retval) = Tempval;
	return Retval;
}
#endif /* ntohf */

#ifndef htond
inline uint64_t htond(double Value)
{
	uint64_t Tempval;
	uint64_t Retval;
	Tempval = *(uint64_t*)(&Value);
	Retval = _WS2_32_WINSOCK_SWAP_LONGLONG(Tempval);
	return Retval;
}
#endif /* htond */

#ifndef ntohd
inline double ntohd(uint64_t Value)
{
	const uint64_t Tempval = _WS2_32_WINSOCK_SWAP_LONGLONG(Value);
	double Retval;
	*((uint64_t*)&Retval) = Tempval;
	return Retval;
}
#endif /* ntohd */
#endif /* NO_EXTRA_HTON_FUNCTIONS */

namespace purelib {
namespace endian {

#define htonv ntohv
template<typename _Int> inline
_Int ntohv(_Int value)
{
    return ntohl(value);
}

template<> inline
bool ntohv(bool value)
{
    return value;
}

template<> inline
uint8_t ntohv(uint8_t value)
{
    return value;
}

template<> inline
uint16_t ntohv(uint16_t value)
{
    return htons(value);
}

template<> inline
uint32_t ntohv(uint32_t value)
{
    return htonl(value);
}

template<> inline
uint64_t ntohv(uint64_t value)
{
    return htonll(value);
}

template<> inline
int8_t ntohv(int8_t value)
{
    return value;
}

template<> inline
int16_t ntohv(int16_t value)
{
    return htons(value);
}

template<> inline
int32_t ntohv(int32_t value)
{
    return htonl(value);
}

template<> inline
int64_t ntohv(int64_t value)
{
    return htonll(value);
}

}; // namespace thelib::endian

}; // namespace thelib

#endif
/*
* Copyright (c) 2012-2015 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

