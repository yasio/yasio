#include <string.h>
#include <assert.h>
#include <ctype.h>
#include "crypto_utils.h"
#define local static
#define BAD_CAST (unsigned char*)
#define rrand(min,max) rand() % ((max) - (min)) + (min)
#define sz_align(d, a)     (((d) + ((a) - 1)) & ~((a) - 1))
#define align_size(size) align((unsigned int)(size), sizeof(void*))
#define min(a,b) ( (a) <= (b) ? (a) : (b) )
/* 
** dirty table
** 
**/
static const int dirty_len_tab[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    
    /* @  A  B  C  D  E  F  G  H  I  J  K  L  M  N  O */
    1, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3, 4, 1, 2, 3,
    
    /* P  Q  R  S  T  U  V  W  X  Y  Z  [  \  ]  ^  _  */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    
    /* ,  a  b  c  d  e  f  g  h  i  j  k  l  m  n  o  */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    
    /* p  q  r  s  t  u  v  w  x  y  z */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

/* 
** Get system timestamp as millsecond 
** 
**/
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#define sprintf_s snprintf
#endif

const char* microtime(void)
{
    static char tms[32];
    sprintf(tms, "%021d", time(NULL));
//#ifdef _WIN32
//    SYSTEMTIME t;
//    GetLocalTime(&t);
//    sprintf_s(tms, sizeof(tms), "[%d-%02d-%02d %02d:%02d:%02d.%03d]", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
//#else
//    struct timeval tv;
//    gettimeofday(&tv, NULL);
//    struct tm* t = localtime(&tv.tv_sec);
//    sprintf_s(tms, sizeof(tms), "[%d-%02d-%02d %02d:%02d:%02d.%03d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec / 1000);
//#endif
    return tms;
}
/************** End of ctimestamp() **************************************/
/************** Continuing where we left off in basic char convertors ******************/

uint8_t     hex2chr(const uint8_t hex)
{
    return hex > 9 ? (hex - 10 + 'a') : (hex + '0');
}

uint8_t     chr2hex(const uint8_t ch)
{
    return isdigit(ch) ? (ch - '0') : (ch - 'A' + 10);
}

uint8_t     hex2uchr(const uint8_t hex)
{
    return hex > 9 ? (hex - 10 + 'A') : (hex + '0');
}

uint8_t     uchr2hex(const uint8_t ch)
{
    return isdigit(ch) ? (ch - '0') : (ch - 'A' + 10);
}

char*       hex2chrp(const uint8_t hex, char charp[2])
{
	charp[0] = hex2chr( hex >> 4 );
	charp[1] = hex2chr( hex & 0x0f );
	return charp;
}

uint8_t     chrp2hex(const char* charp[2])
{
	return 0;
}

void        hexs2chars(const void* source, unsigned int sourceLen,
	                               char* dest, unsigned int destLen)
{
	int i = 0, j = 0;
	int n = min(sourceLen, destLen);
	// assert( (sourceLen << 1) <= destLen );

    for(; i < n; ++i, j = i << 1)
    {
        (void)hex2chrp(((const uint8_t*)source) [i], dest + j);
    }
}

/* end of basic convertors */


/*
** Simple MD5 implementation
**
*/
// Constants are the integer part of the sines of integers (in radians) * 2^32.
local const uint32_t k[64] = {
0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee ,
0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501 ,
0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be ,
0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821 ,
0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa ,
0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8 ,
0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed ,
0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a ,
0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c ,
0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70 ,
0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05 ,
0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665 ,
0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039 ,
0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1 ,
0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1 ,
0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391 };
 
// r specifies the per-round shift amounts
local const uint32_t r[] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                      5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
                      4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                      6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

local void to_bytes(uint32_t val, uint8_t *bytes)
{
    bytes[0] = (uint8_t) val;
    bytes[1] = (uint8_t) (val >> 8);
    bytes[2] = (uint8_t) (val >> 16);
    bytes[3] = (uint8_t) (val >> 24);
}

local void to_chars(uint32_t value, char chars[32])
{
	(void)hex2chrp( (uint8_t) value, chars);
    (void)hex2chrp( (uint8_t) (value >> 8),  chars + 2 );
	(void)hex2chrp( (uint8_t) (value >> 16), chars + 4 );
	(void)hex2chrp( (uint8_t) (value >> 24), chars + 6 );

}

local uint32_t to_int32(uint8_t *bytes)
{
    return (uint32_t) bytes[0]
        | ((uint32_t) bytes[1] << 8)
        | ((uint32_t) bytes[2] << 16)
        | ((uint32_t) bytes[3] << 24);
}
 
void  md5(const void* initial_msg, size_t initial_len, void* digest) 
{
    // These vars will contain the hash
    uint32_t h0, h1, h2, h3;
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    size_t new_len, offset;
    uint32_t w[16];
    uint32_t a, b, c, d, i, f, g, temp;
 
    // Initialize variables - simple count in nibbles:
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
 
    //Pre-processing:
    //append "1" bit to message    
    //append "0" bits until message length in bits ¡Ô 448 (mod 512)
    //append length mod (2^64) to message
 
    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
        ;
 
    msg = (uint8_t*)malloc(new_len + 8);
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
    for (offset = initial_len + 1; offset < new_len; offset++)
        msg[offset] = 0; // append "0" bits
 
    // append the len in bits at the end of the buffer.
    to_bytes(initial_len << 3, msg + new_len);
    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
    to_bytes(initial_len>>29, msg + new_len + 4);
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    for(offset=0; offset<new_len; offset += (512/8)) {
 
        // break chunk into sixteen 32-bit words w[j], 0 ¡Ü j ¡Ü 15
        for (i = 0; i < 16; i++)
            w[i] = to_int32(msg + offset + (i << 2) );
 
        // Initialize hash value for this chunk:
        a = h0;
        b = h1;
        c = h2;
        d = h3;
 
        // Main loop:
        for(i = 0; i<64; i++) {
 
            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }
 
            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
 
        }
 
        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
 
    }
 
    // cleanup
    free(msg);
 
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    to_bytes(h0, BAD_CAST digest);
    to_bytes(h1, BAD_CAST digest + 4);
    to_bytes(h2, BAD_CAST digest + 8);
    to_bytes(h3, BAD_CAST digest + 12);
}

void  md5chars(const void* initial_msg, size_t initial_len, char chars[32]) 
{
    // These vars will contain the hash
    uint32_t h0, h1, h2, h3;
 
    // Message (to prepare)
    uint8_t *msg = NULL;
 
    size_t new_len, offset;
    uint32_t w[16];
    uint32_t a, b, c, d, i, f, g, temp;
 
    // Initialize variables - simple count in nibbles:
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
 
    //Pre-processing:
    //append "1" bit to message    
    //append "0" bits until message length in bits ¡Ô 448 (mod 512)
    //append length mod (2^64) to message
 
    for (new_len = initial_len + 1; new_len % (512/8) != 448/8; new_len++)
        ;
 
    msg = (uint8_t*)malloc(new_len + 8);
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80; // append the "1" bit; most significant bit is "first"
    for (offset = initial_len + 1; offset < new_len; offset++)
        msg[offset] = 0; // append "0" bits
 
    // append the len in bits at the end of the buffer.
    to_bytes(initial_len << 3, msg + new_len);
    // initial_len>>29 == initial_len*8>>32, but avoids overflow.
    to_bytes(initial_len >> 29, msg + new_len + 4);
 
    // Process the message in successive 512-bit chunks:
    //for each 512-bit chunk of message:
    for(offset=0; offset<new_len; offset += (512/8)) {
 
        // break chunk into sixteen 32-bit words w[j], 0 ¡Ü j ¡Ü 15
        for (i = 0; i < 16; i++)
            w[i] = to_int32(msg + offset + (i << 2) );
 
        // Initialize hash value for this chunk:
        a = h0;
        b = h1;
        c = h2;
        d = h3;
 
        // Main loop:
        for(i = 0; i<64; i++) {
 
            if (i < 16) {
                f = (b & c) | ((~b) & d);
                g = i;
            } else if (i < 32) {
                f = (d & b) | ((~d) & c);
                g = (5*i + 1) % 16;
            } else if (i < 48) {
                f = b ^ c ^ d;
                g = (3*i + 5) % 16;          
            } else {
                f = c ^ (b | (~d));
                g = (7*i) % 16;
            }
 
            temp = d;
            d = c;
            c = b;
            b = b + LEFTROTATE((a + f + k[i] + w[g]), r[i]);
            a = temp;
 
        }
 
        // Add this chunk's hash to result so far:
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
 
    }
 
    // cleanup
    free(msg);
 
    //var char digest[16] := h0 append h1 append h2 append h3 //(Output is in little-endian)
    to_chars(h0, chars);
    to_chars(h1, chars + 8);
    to_chars(h2, chars + 16);
    to_chars(h3, chars + 24);
}

/* end of Simple MD5 implement */

/* 
**MD5 V2 & v3: implement by call the md5 library 
**
*/
//void        md5_v2(const void* source, unsigned int sourceLen, void* digiest)
//{
//    md5_state_t state;
//    md5_init(&state);
//    md5_append(&state, (const md5_byte_t*)source, sourceLen);
//    md5_finish(&state, (md5_byte_t*)digiest);
//}

//#include "openssl_md5.h"
//void        md5_v3(const void* source, unsigned int sourceLen, void* digiest)
//{
//    MD5_CTX ctx;
//    MD5_Init(&ctx);
//    MD5_Update(&ctx, (void*)source, sourceLen);
//    MD5_Final((unsigned char*)digiest, &ctx);
//}

/* Simple MD6 implementation, the return is a hexstream */
//void        md6(const void* source, unsigned int sourceLen, void* dest, unsigned int hashByteLen)
//{
//	md6_state state;
//
//    assert(hashByteLen <= 64);
//
//	md6_init(&state, hashByteLen << 3);
//	md6_update(&state, (unsigned char*)source, sourceLen << 3);
//	md6_final(&state, (unsigned char*)dest);
//}

/* Simple MD6 implementation, the return is a charstream*/
//void        md6chars(const void* source, unsigned int sourceLen, char* dest, unsigned int hashByteLen)
//{
//	unsigned char buffer[64];
//	md6(source, sourceLen, buffer, hashByteLen);
//	hexs2chars(buffer, hashByteLen, dest, hashByteLen << 1);
//}


/* 
** base 64 codec implement
**
*/
/* base 64 encode: destLen shuld be set sourceLen * 2 */
int         base64enc(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen)
{
    int r1, r2;
    base64_encodestate state;

    assert(source && dest && destLen); // assert is parameters valid.

    base64_init_encodestate(&state);
    r1 = base64_encode_block((const char*)source, sourceLen, (char*)dest, &state);
    r2 = base64_encode_blockend((char*)dest + r1, &state);
    
    *destLen = r1 + r2;
    ( (char*)dest ) [*destLen] = '\0';
    
    return JXC_OK;
}

/* base 64 decode: destLen should be set sourceLen*/
int         base64dec (const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen)
{
    int r1 = 0;
    base64_decodestate state;

    assert(source && dest && destLen); // assert is parameters valid.

    base64_init_decodestate(&state);
    r1 = base64_decode_block((const char*)source, sourceLen, (char*)dest, &state);
    
    *destLen = r1;
    ((char*)dest)[r1] = '\0';

    return JXC_OK;
}

/*
** url encode / decode implementation
**
*/


/* urlencode: destLen should be sourceLen * 3 */
static unsigned int
urlenc_check_len(const void *source, unsigned int sourceLen)
{
    unsigned int lenNeed = 0;
    unsigned int index = 0;

    for( ; index < sourceLen; ++index)
    {
        lenNeed += ( isalnum( ((uint8_t*)source) [index] ) ? 1 : 3 );
    }

    return lenNeed; 
}

/* urlencode: destLen should be sourceLen * 3 */
int         urlenc(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen)
{
    uint8_t buffer[4];
    int     stringLen = 0;
    char*   wrptr = (char*) dest;
    unsigned int index = 0;
    for( ; index < sourceLen; ++index)
    {      
        memset( buffer, 0, sizeof(buffer) );
        if( isalnum( ((uint8_t*)source) [index] ) )
        {      
            buffer[0] = ((uint8_t*)source) [index];
            stringLen = 1;
        }
        else
        {
            buffer[0] = '%';
            buffer[1] = hex2uchr( ((uint8_t*)source) [index] >> 4 );
            buffer[2] = hex2uchr( ((uint8_t*)source) [index] % 16); 
            stringLen = 3;
        }

        memcpy(wrptr, buffer, stringLen);
        wrptr += stringLen;
    }

    *destLen = wrptr - (char*)dest;

    return JXC_OK;
}

/* urldecode: destLen  */
int         urldec(const void *source, unsigned int sourceLen,
                                 void *dest,   unsigned int *destLen)
{
    uint8_t tmpc = 0;
    unsigned int index = 0;
    uint8_t* wrptr = (uint8_t*)dest;
    for( ; index < sourceLen; ++index )
    {
        tmpc = 0;
        if( ((const char*)source) [index] == '%')
        {
            tmpc = (uchr2hex( ((const char*)source) [index + 1] ) << 4 );
            tmpc |= uchr2hex( ((const char*)source) [index + 2] );
            index += 2;
        }
        else if( ((const char*)source)[index] == '+')
        {
            tmpc = ' ';
        }
        else
        {
            tmpc = ((const char*)source)[index];
        }
        *wrptr++ = tmpc;
    }

    *destLen = wrptr - (uint8_t*)dest; // save real destLen

    return JXC_OK;
}

