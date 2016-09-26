/***************************************************************
* server_logger.h - implement a simple file log
*
* Author: halx99
*
* Purpose: log
* 
****************************************************************/
#ifndef _SERVER_LOG_HPP_
#define _SERVER_LOG_HPP_
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <mutex>
#include "utils/timestamp.h"
#include "utils/singleton.h"

class server_logger
{
public:
    server_logger(const char* filename = "server_out.log");
	~server_logger(void);

    void log(const char* format, ...);

    void log_all(const char* format, ...); 

    void dump_hex(const char* prompt, const char* hex, int len);

private:
	FILE*         log_;
    std::mutex    mutex_;
};

#ifdef _DEBUG
#    define LOG_TRACE(format,...)     purelib::gc::singleton<server_logger>::instance()->log("%s[TRACE]" format "\n",ctimestamp(),##__VA_ARGS__)
#    define LOG_TRACE_ALL(format,...) purelib::gc::singleton<server_logger>::instance()->log_all("%s[TRACE]" format "\n",ctimestamp(),##__VA_ARGS__)
#else
#    define LOG_TRACE(format,...)
#    define LOG_TRACE_ALL(format,...)
#endif

#define LOG_ERROR(format,...)     purelib::gc::singleton<server_logger>::instance()->log("%s[ERROR]" format "\n",ctimestamp(),##__VA_ARGS__)
#define LOG_ERROR_ALL(format,...) purelib::gc::singleton<server_logger>::instance()->log_all("%s[ERROR]" format "\n",ctimestamp(),##__VA_ARGS__)
#define LOG_WARN(format,...)     purelib::gc::singleton<server_logger>::instance()->log("%s[WARN]" format "\n",ctimestamp(),##__VA_ARGS__)
#define LOG_WARN_ALL(format,...)     purelib::gc::singleton<server_logger>::instance()->log_all("%s[WARN]" format "\n",ctimestamp(),##__VA_ARGS__)

#endif

