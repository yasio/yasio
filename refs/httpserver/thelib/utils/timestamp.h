#ifndef _RANDOM_H_
#define _RANDOM_H_
//#define __random__(min,max) rand() % ((max) - (min)) + (min)

#include <time.h>
#include <stdio.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#define sprintf_s snprintf
#endif
#include <sstream>
#include <iomanip>


inline const char* ctimestamp(void)
{
    static char tms[32];
#ifdef _WIN32
    SYSTEMTIME t;
    GetLocalTime(&t);
    sprintf_s(tms, sizeof(tms), "[%d-%02d-%02d %02d:%02d:%02d.%03d]", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* t = localtime(&tv.tv_sec);
    sprintf_s(tms, sizeof(tms), "[%d-%02d-%02d %02d:%02d:%02d.%03d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec / 1000);
#endif
    return tms;
}

/*static
std::string timestamp(void)
{    
#ifdef _WIN32
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::ostringstream oss;
    oss << "[" 
        << std::setw(2) << std::setfill('0') << t.wHour << ":"
        << std::setw(2) << std::setfill('0') << t.wMinute << ":"
        << std::setw(2) << std::setfill('0') << t.wSecond << "."
        << std::setw(3) << std::setfill('0') << t.wMilliseconds
        << "]";
    return oss.str();
#else 
    timeval tv;
    gettimeofday(&tv, NULL);
    tm* t = localtime(&tv.tv_sec);
    std::ostringstream oss;
    oss << "[" 
        << std::setw(2) << std::setfill('0') << t->tm_hour << ":"
        << std::setw(2) << std::setfill('0') << t->tm_min << ":"
        << std::setw(2) << std::setfill('0') << t->tm_sec << "."
        << std::setw(3) << std::setfill('0') << tv.tv_usec / 1000
        << "]";
    return oss.str();
#endif
}
*/

//static void dump_hex(const char* prompt, const char* hex, int len, FILE* fp = stdout, int line_feed = 16)
//{
//    fprintf(fp, prompt);
//    for(int i = 0; i < len; ++i)
//    {
//        if( i + 1 != len) {
//            if( (i + 1) % line_feed != 0 ) {
//                fprintf(fp, "%02x ", (unsigned int)hex[i]);
//            }
//            else {
//                fprintf(fp, "%02x\n", (unsigned int)hex[i]);
//            }
//        }
//        else {
//            fprintf(fp, "%02x\n", (unsigned int)hex[i]);
//        }
//    }
//}

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/


