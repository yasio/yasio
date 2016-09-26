/***************************************************************
* xxpie_logger.h - implement a simple file log
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: log
* 
****************************************************************/
#ifndef _LOG_H_
#define _LOG_H_
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <simple++/thread_synch.h>
#include <simple++/utility.h>
#include <simple++/singleton.h>
using namespace exx;
using namespace exx::asy;

class xxpie_logger
{
public:
    xxpie_logger(const char* filename = "xxpie_server_out.log", const char* err = "xxpie_server_err.log", const char* proto = "xxpie_server_protocol.log");
	~xxpie_logger(void);

    void log(const char* format, ...);

    void log_as_error(const char* format, ...);

    void log_all(const char* format, ...); 

    void log_all_as_error(const char* format, ...); 

    void proto_log(const char* format, ...);

    void dump_hex(const char* prompt, const char* hex, int len);

private:
	FILE*         out_;
    FILE*         err_;
    FILE*         proto_;
	thread_mutex  mutex_;
};

#define LOG_TRACE(format,...) singleton<xxpie_logger>::instance()->log("%s[TRACE]" format,ctimestamp(),##__VA_ARGS__)
#define LOG_TRACE_ALL(format,...) singleton<xxpie_logger>::instance()->log_all("%s[TRACE]" format,ctimestamp(),##__VA_ARGS__)
#define LOG_ERROR(format,...) singleton<xxpie_logger>::instance()->log_as_error("%s[ERROR]" format,ctimestamp(),##__VA_ARGS__)
#define LOG_ERROR_ALL(format,...) singleton<xxpie_logger>::instance()->log_all_as_error("%s[ERROR]" format,ctimestamp(),##__VA_ARGS__)
#ifdef _DEBUG
#ifdef LOG_PROTOCOL
#define PROTO_LOG(msg,from,to) singleton<xxpie_logger>::instance()->proto_log("%sPROTO_LOG: %s --> %s \n%s\n", ctimestamp(), from, to, msg);
#else 
#define PROTO_LOG(msg,from,to)
#endif
#else
#define PROTO_LOG(msg,from,to)
#endif

#ifdef ENABLE_DUMP_HEX
#define DUMP_HEX(prompt,hex,len)  singleton<xxpie_logger>::instance()->dump_hex(prompt,hex,len)
#else
#define DUMP_HEX(prompt,hex,len)
#endif
#endif

