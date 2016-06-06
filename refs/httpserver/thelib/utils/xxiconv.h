/* xxiconv.h
 Purpose: Implenemt to convert between UNICODE and UTF-8
          U-00000000 每 U-0000007F:  0xxxxxxx
          U-00000080 每 U-000007FF:  110xxxxx 10xxxxxx
          U-00000800 每 U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
          U-00010000 每 U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
          U-00200000 每 U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
          U-04000000 每 U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
*/
#ifndef _XXICONV_H_
#define _XXICONV_H_

#include "unreal_string.h"
using namespace simplepp_1_13_201301;

#define UCS_1BYTE_MASK 0x00000000
#define UCS_2BYTE_MASK 0x0000C080
#define UCS_3BYTE_MASK 0x00E08080
#define UCS_4BYTE_MASK 0xF0808080

typedef unsigned int ucs4_t;
// typedef wchar_t ucs4_t;
typedef wchar_t ucs_t;

/* Return code if invalid input after a shift sequence of n bytes was read.
   (xxx_mbtowc) */
#define RET_SHIFT_ILSEQ(n)  (-1-2*(n))
/* Return code if invalid. (xxx_mbtowc) */
#define RET_ILSEQ           RET_SHIFT_ILSEQ(0)
/* Return code if only a shift sequence of n bytes was read. (xxx_mbtowc) */
#define RET_TOOFEW(n)       (-2-2*(n))
/* Retrieve the n from the encoded RET_... value. */
#define DECODE_SHIFT_ILSEQ(r)  ((unsigned int)(RET_SHIFT_ILSEQ(0) - (r)) / 2)
#define DECODE_TOOFEW(r)       ((unsigned int)(RET_TOOFEW(0) - (r)) / 2)
/* Return code if invalid. (xxx_wctomb) */
#define RET_ILUNI      -1
/* Return code if output buffer is too small. (xxx_wctomb, xxx_reset) */
#define RET_TOOSMALL   -2

namespace xxiconv {

namespace utf8 {
/*
 * UTF-8
 */
/* Specification: RFC 3629 */
static int
mbtowc (ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c = s[0];

  if (c < 0x80) {
    *pwc = c;
    return 1;
  } else if (c < 0xc2) {
    return RET_ILSEQ;
  } else if (c < 0xe0) {
    if (n < 2)
      return RET_TOOFEW(0);
    if (!((s[1] ^ 0x80) < 0x40))
      return RET_ILSEQ;
    *pwc = ((ucs4_t) (c & 0x1f) << 6)
           | (ucs4_t) (s[1] ^ 0x80);
    return 2;
  } else if (c < 0xf0) {
    if (n < 3)
      return RET_TOOFEW(0);
    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
          && (c >= 0xe1 || s[1] >= 0xa0)))
      return RET_ILSEQ;
    *pwc = ((ucs4_t) (c & 0x0f) << 12)
           | ((ucs4_t) (s[1] ^ 0x80) << 6)
           | (ucs4_t) (s[2] ^ 0x80);
    return 3;
  } else if (c < 0xf8 && sizeof(ucs4_t)*8 >= 32) {
    if (n < 4)
      return RET_TOOFEW(0);
    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
          && (s[3] ^ 0x80) < 0x40
          && (c >= 0xf1 || s[1] >= 0x90)))
      return RET_ILSEQ;
    *pwc = ((ucs4_t) (c & 0x07) << 18)
           | ((ucs4_t) (s[1] ^ 0x80) << 12)
           | ((ucs4_t) (s[2] ^ 0x80) << 6)
           | (ucs4_t) (s[3] ^ 0x80);
    return 4;
  } else if (c < 0xfc && sizeof(ucs4_t)*8 >= 32) {
    if (n < 5)
      return RET_TOOFEW(0);
    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
          && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
          && (c >= 0xf9 || s[1] >= 0x88)))
      return RET_ILSEQ;
    *pwc = ((ucs4_t) (c & 0x03) << 24)
           | ((ucs4_t) (s[1] ^ 0x80) << 18)
           | ((ucs4_t) (s[2] ^ 0x80) << 12)
           | ((ucs4_t) (s[3] ^ 0x80) << 6)
           | (ucs4_t) (s[4] ^ 0x80);
    return 5;
  } else if (c < 0xfe && sizeof(ucs4_t)*8 >= 32) {
    if (n < 6)
      return RET_TOOFEW(0);
    if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40
          && (s[3] ^ 0x80) < 0x40 && (s[4] ^ 0x80) < 0x40
          && (s[5] ^ 0x80) < 0x40
          && (c >= 0xfd || s[1] >= 0x84)))
      return RET_ILSEQ;
    *pwc = ((ucs4_t) (c & 0x01) << 30)
           | ((ucs4_t) (s[1] ^ 0x80) << 24)
           | ((ucs4_t) (s[2] ^ 0x80) << 18)
           | ((ucs4_t) (s[3] ^ 0x80) << 12)
           | ((ucs4_t) (s[4] ^ 0x80) << 6)
           | (ucs4_t) (s[5] ^ 0x80);
    return 6;
  } else
    return RET_ILSEQ;
}

static int
wctomb (unsigned char *r, ucs4_t wc, int n) /* n == 0 is acceptable */
{
  int count;
  if (wc < 0x80)
    count = 1;
  else if (wc < 0x800)
    count = 2;
  else if (wc < 0x10000)
    count = 3;
  else if (wc < 0x200000)
    count = 4;
  else if (wc < 0x4000000)
    count = 5;
  else if (wc <= 0x7fffffff)
    count = 6;
  else
    return RET_ILUNI;
  if (n < count)
    return RET_TOOSMALL;
  switch (count) { /* note: code falls through cases! */
    case 6: r[5] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x4000000;
    case 5: r[4] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x200000;
    case 4: r[3] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x10000;
    case 3: r[2] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0x800;
    case 2: r[1] = 0x80 | (wc & 0x3f); wc = wc >> 6; wc |= 0xc0;
    case 1: r[0] = wc;
  }
  return count;
}

static
size_t wclen_as_mb(ucs4_t wc)
{
  size_t count;
  if (wc < 0x80)
    count = 1;
  else if (wc < 0x800)
    count = 2;
  else if (wc < 0x10000)
    count = 3;
  else if (wc < 0x200000)
    count = 4;
  else if (wc < 0x4000000)
    count = 5;
  else if (wc <= 0x7fffffff)
    count = 6;
  else
    return RET_ILUNI;
  return count;
}

static
size_t wcslen_as_mbs(const wchar_t* wcs)
{
    size_t l = 0;
    wchar_t c;
    while( (c = *wcs++) != L'\0' ) l += wclen_as_mb(c);
    return l;
}

static
managed_cstring wcstombs(const wchar_t* wcs)
{
    size_t length = xxiconv::utf8::wcslen_as_mbs(wcs);

    if(length > 0) 
    {
        unsigned char* temp = (unsigned char*)malloc(length + 1);
        
        const wchar_t* ptr = wcs;
        unsigned char* dst = temp;
        while(*ptr != L'\0') 
        {
            int c = xxiconv::utf8::wctomb(dst, *ptr, length);
            dst += c;
            ++ptr;
        }
        *dst = '\0';

        return managed_cstring((char*)temp, length);
    }
    return managed_cstring(); 
}

static
size_t mbclen_as_enc(unsigned char mbc)
{
  size_t count;
  unsigned char ch = mbc;
  if ( ch >= 0xfc)
    count = 6;
  else if ( ch >= 0xf8)
    count = 5;
  else if ( ch >= 0xf0)
    count = 4;
  else if ( ch >= 0xe0 )
    count = 3;
  else if ( ch >= 0xc0 )
    count = 2;
  else if ( ch < 0x80 )
    count = 1;
  else
    return RET_ILUNI;
  return count;
}

static
size_t mbclen_as_wc(size_t el)
{
  switch(el)
  {
  case 1:
  case 2:
  case 3:
      return 2;
  case 4:
  case 5:
  case 6:
      return 4;
  default:
      return RET_ILUNI;
  }
}

static
size_t mbslen_as_wcs(const unsigned char *mbs)
{
    size_t l = 0;
    unsigned char ch;
    while( (ch = *mbs) != 0x0 ) 
    {
        int cl = mbclen_as_enc(ch);
        l += mbclen_as_wc(cl);
        mbs += cl;
    }
    return l / sizeof(wchar_t);
}

static
managed_wcstring mbstowcs(const unsigned char* mbs)
{
    size_t length = xxiconv::utf8::mbslen_as_wcs(mbs);

    if(length > 0) 
    {
        wchar_t* temp = (wchar_t*)malloc( sizeof(wchar_t) * (length + 1) );
        
        const unsigned char* ptr = mbs;
        wchar_t* dst = temp;
        while(*ptr != L'\0') 
        {
            int count = xxiconv::utf8::mbtowc((ucs4_t*)dst, ptr, length);
            ++dst;
            ptr += count;
        }
        *dst = L'\0';

        return managed_wcstring(temp, length);
    }
    return managed_wcstring(); 
}

}; // namespace xxiconv::utf8

namespace ascii {

static managed_wcstring mbstowcs(const char* source)
{
    int count = strlen(source);
    // int wcount = count * sizeof(ucs_t) + sizeof(ucs_t);
    ucs_t* temp = (ucs_t*)malloc((count + 1) * sizeof(ucs_t) );
    setlocale(LC_ALL, "");
    (void)::mbstowcs(temp, source, count + 1);
    return managed_wcstring(temp);
}
/*
 * ASCII
 */
static int
mbtowc (ucs4_t *pwc, const unsigned char *s, int n)
{
  unsigned char c = *s;
  if (c < 0x80) {
    *pwc = (ucs4_t) c;
    return 1;
  }
  return RET_ILSEQ;
}

static int
wctomb (unsigned char *r, ucs4_t wc, int n)
{
  if (wc < 0x0080) {
    *r = wc;
    return 1;
  }
  return RET_ILUNI;
}

}; // namespace xxiconv::ascii

}; // namespace xxiconv

#define to_utf8(str) xxiconv::utf8::wcstombs(xxiconv::ascii::mbstowcs(str).c_str()).c_str()

#endif

