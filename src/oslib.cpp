#include <time.h>
#include "oslib.h"
#include <math.h>
#if defined (_WIN32)
#include <Windows.h>

/// the implemention of atoll on win32 for visual studio 2012 or before
#if _MSC_VER <= 1700
extern long long atoll(const char* p)  
{  
    int minus = 0;  
    long long value = 0;  
    if (*p == '-')  
    {  
        minus ++;  
        p ++;  
    }  
    while (*p >= '0' && *p <= '9')  
    {  
        value *= 10;  
        value += *p - '0';  
        p ++;  
    }  
    return minus ? 0 - value : value;  
}
#endif

/// the implemention of gettimeofday on win32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)    
#define EPOCHFILETIME  11644473600000000Ui64
#else   
#define EPOCHFILETIME  11644473600000000ULL
#endif

int
oslib::gettimeofday(struct timeval *tv, struct timezone2 *tz)
{
    FILETIME ft;
    unsigned __int64 now = 0;
    static int tzflag = 0;

    if (NULL != tv)
    {
        GetSystemTimeAsFileTime(&ft);

        now |= ft.dwHighDateTime;
        now <<= 32;
        now |= ft.dwLowDateTime;

        now /= 10;  /*convert into microseconds*/
        /*converting file time to unix epoch*/
        now -= EPOCHFILETIME;
        tv->tv_sec = (long)(now / 1000000UL);
        tv->tv_usec = (long)(now % 1000000UL);
    }

#if 0
    if (tz)
    {
        if (!tzflag)
        {
            tzset();

            tzflag++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
#endif

    return 0;
}

#elif defined(__linux)

#endif

// common libs
long long strtoll_s(const char* p, int n)
{
    int minus = 0;
    long long value = 0;
    if (*p == '-')
    {
        minus++;
        p++;
        --n;
    }
    while (*p >= '0' && *p <= '9' && n-- > 0)
    {
        value *= 10;
        value += *p - '0';
        p++;
    }
    if (minus)
        value = -value;
    return value;
}

int strtoi_s(const char* p, int n)
{
    int minus = 0;
    int value = 0;
    if (*p == '-')
    {
        minus++;
        p++;
        --n;
    }
    while (*p >= '0' && *p <= '9' && n-- > 0)
    {
        value *= 10;
        value += *p - '0';
        p++;
    }
    if (minus)
        value = -value;
    return value;
}

double strtolf_s(const char* p, int size)
{
    const char* start = p;
    long long valuelonglong = 0;

    double n = 0, sign = 1, scale = 0; double subscale = 0, signsubscale = 1;

    if (*p == '-') sign = -1, p++;    /* Has sign? */

    if (*p == '0') p++;            /* is zero */

    if (*p >= '1' && *p <= '9')    do    valuelonglong = (valuelonglong * 10) + (*p++ - '0');    while (*p >= '0' && *p <= '9' && (p - start) < size);    /* Number? */

    n = (double)valuelonglong;

    bool is_real = *p == '.' || *p == 'e' || *p == 'E';

    if (!is_real)
    {
        return sign * valuelonglong;
    }

    if (*p == '.' && p[1] >= '0' && p[1] <= '9') { p++;        do    n = (n*10.0) + (*p++ - '0'), scale--; while (*p >= '0' && *p <= '9' && (p - start) < size); }    /* Fractional part? */

    if (*p == 'e' || *p == 'E')        /* Exponent? */
    {
        p++; if (*p == '+') p++;    else if (*p == '-') signsubscale = -1, p++;        /* With sign? */
        while (*p >= '0' && *p <= '9' && (p - start) < size) subscale = (subscale * 10) + (*p++ - '0');    /* Number? */
    }

    n = sign*n*pow(10.0, (scale + subscale*signsubscale));    /* number = +/- number.fraction * 10^+/- exponent */

    return n;
}

long long millitime(void)
{
    return microtime() / 1000;
}

long long microtime(void)
{
    timeval tv;
    oslib::gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
}

// end of common libs

