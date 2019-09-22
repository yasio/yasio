/*
 If you compile with tolua runtime at windows platform, you need add this source to your build
 system. tolua runtime repo: https://github.com/jarjin/tolua_rumtime
*/
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libluajit.a")
#include <math.h>
#include <stdlib.h>

#if _MSC_VER >= 1900
#  include <stdio.h>

#  ifdef __cplusplus
extern "C" {
#  endif

FILE* __cdecl __iob_func(unsigned int i) { return __acrt_iob_func(i); }

#  if defined(_fmode)
#    undef _fmode
#  endif

static inline int __get_fmode_impl()
{
  int value = 0;
  _get_fmode(&value);
  return value;
}
int _fmode = __get_fmode_impl();

#  ifdef __cplusplus
}
#  endif

#endif /* _MSC_VER >= 1900 */
