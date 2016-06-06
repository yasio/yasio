/*
  Copyright (C) 2013 YYKG Enterprises.  All rights reserved.

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  X.D. Guo
  xxseekerj@gmail.com

 */
/* $Id: jxcodec.h,v 1.0.0 2013/07/20 02:23:30 lpd Exp $ */
/*
  Independent implementation of MD5 (RFC 1321).

  This code implements the auth Algorithm defined in RFC 1321, whose
  text is available at
	http://www.ietf.org/rfc/rfc1321.txt
  The code is derived from the text of the RFC, including the test suite
  (section A.5) but excluding the rest of Appendix A.  It does not include
  any code or documentation that is identified in the RFC as being
  copyrighted.
 */
#ifndef _CRYPTO_UTILS_H_
#define _CRYPTO_UTILS_H_
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "md5.h"
//#include "md6.h"
#include "libb64.h"

#ifdef __cplusplus
extern "C" 
{
#endif

/*
 * This package supports both compile-time and run-time determination of CPU
 * byte order.  If ARCH_IS_BIG_ENDIAN is defined as 0, the code will be
 * compiled to run only on little-endian CPUs; if ARCH_IS_BIG_ENDIAN is
 * defined as non-zero, the code will be compiled to run only on big-endian
 * CPUs; if ARCH_IS_BIG_ENDIAN is not defined, the code will be compiled to
 * run on either big- or little-endian CPUs, but will run slightly less
 * efficiently on either one than if ARCH_IS_BIG_ENDIAN is defined.
 */

#define JXC_OK         0
#define JXC_BAD_ALLOC -2

/* Get system timestamp millsecond */
const char* microtime(void);

/* basic char convertors */
uint8_t     hex2uchr(const uint8_t hex);

uint8_t     uchr2hex(const uint8_t ch);

uint8_t     hex2chr(const uint8_t hex);

uint8_t     chr2hex(const uint8_t ch);

char*       hex2chrp(const uint8_t hex, char charp[2]);

uint8_t     chrp2hex(const char* charp[2]);

/* convert hexstream to charstream */
void        hexs2chars(const void* source, unsigned int sourceLen,
	                               char* dest, unsigned int destLen);
/* -- end of basic convertors -- */

/* Simple MD5 implementation, the return is a hexstream */
void        md5(const void* initial_msg, size_t initial_len, void* digest);

/* Simple MD5 implementation, the return is a hexstream */
void        md5chars(const void* initial_msg, size_t initial_len, char chars[32]);

/* MD5 V3: implement by call the openssl-md5 library, the return is a hexstream*/
// void        jxc_md5_v3(const void* source, unsigned int sourceLen, void* dest);

/* Simple MD6 implementation, the return is a hexstream*/
//void        jxc_md6(const void* source, unsigned int sourceLen, void* dest, unsigned int hashByteLen);

/* Simple MD6 implementation, the return is a hexstream */
//void        jxc_md6chars(const void* source, unsigned int sourceLen, char* dest, unsigned int hashByteLen);

/* base 64 encode: destLen should be set sourceLen * 2 */
int         base64enc(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen); 
/* base 64 decode: destLen should be set sourceLen */
int         base64dec (const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen);

/* urlencode(RFC1738 standard implement): destLen should be sourceLen * 3.
**
** remark: php urlencode ' ' to '+'; but php urldecode can decode both
**         RFC1738 encode-text and php urlencode.
*/
int         urlenc(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen);

/* urldecode: the destLen should be sourceLen
**
*  remark: support decode RFC1738 encode-text and php urlencode.
*/
int         urldec(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen);

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* _JXCODEC_H_ */
