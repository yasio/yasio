#pragma comment(lib, "ws2_32.lib")
#include <math.h>

#if _MSC_VER >= 1900
#  include <stdio.h>

#  ifdef __cplusplus
extern "C" {
#  endif

FILE *__cdecl __iob_func(unsigned int i) { 
    return __acrt_iob_func(i); }

#  ifdef __cplusplus
}
#  endif

#endif /* _MSC_VER >= 1900 */
