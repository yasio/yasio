#ifndef _OSLIB_H_
#define _OSLIB_H_

/// common includes
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/// os includes
#ifdef _WIN32

typedef unsigned long DWORD;

    //DWORD
#elif defined(__GNUC__)

#    include <strings.h>
#    include <pthread.h>

#else

#    error not support this OS

#endif /* end of os include */

/// all os exist functions
#if defined(_WIN32)

#    define __threadlocal __declspec(thread)
#    define thread_native_type DWORD
#    define this_thread GetCurrentThreadId

#elif defined(__linux)

#    define __threadlocal __thread
#    define thread_native_type pthread_t
#    define this_thread pthread_self
#    define stricmp strcasecmp

#endif /* end of os exist functions */

/// extends functions
#if defined(_WIN32)

long long atoll(const char* p);

struct timezone  
{  
  int tz_minuteswest;  
  int tz_dsttime;  
};  
 
int  
gettimeofday (struct timeval *tv, struct timezone *tz)  ;

#elif defined(__linux)


#endif /* end of os functions */

#ifdef __cplusplus
}
#endif


#endif

