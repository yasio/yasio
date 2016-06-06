//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include <string.h>

#include "server_logger.hpp"

#include "mysql_client.hpp"

#include "errmsg.h"

#include "thelib/utils/container_helper.h"

//#include <my_global.h>

static std::mutex mtx;

#define SQL_MAX_LEN 512

mysql_client::mysql_client(void)
{
    memset(&this->internal_db_, 0x0, sizeof(this->internal_db_));
    connect();
}

mysql_client::~mysql_client(void)
{
    close();
}

bool mysql_client::exec_query0(const char* sqlFormat, ...)
{
    va_list ap;
    va_start(ap, sqlFormat);

    char szBuf[SQL_MAX_LEN];
    int length = vsnprintf(szBuf, SQL_MAX_LEN, sqlFormat, ap);
    // vsnprintf_s(szBuf, SQL_MAX_LEN, SQL_MAX_LEN, sqlFormat, ap);
    va_end(ap); 

    mysqlcc::result_set result;
    return this->exec_query_i(szBuf, length);
}

mysqlcc::result_set mysql_client::exec_query1(const char* sqlFormat, ...)
{
    va_list ap;
    va_start(ap, sqlFormat);

    char szBuf[SQL_MAX_LEN];
    int length = vsnprintf(szBuf, SQL_MAX_LEN, sqlFormat, ap);
    // vsnprintf_s(szBuf, SQL_MAX_LEN, SQL_MAX_LEN, sqlFormat, ap);
    va_end(ap); 

    mysqlcc::result_set result;
    this->exec_query_i(szBuf, length, &result);

    return result.release();
}

bool mysql_client::exec_query_i(const char* sql, unsigned int length, mysqlcc::result_set* result)
{
    if(this->is_lost())
    { // reconnect
        LOG_WARN("%s", "the MYSQL Server Gone Away, Now Reconnect to MYSQL server...");
        close();
        connect();
    }

    bool succeed = ( 0 == mysql_real_query(&this->internal_db_, sql, length) );
    if(succeed)
    {
        if( result != nullptr)
        {
            result->assign( mysql_store_result(&this->internal_db_) );
        }
        return true;
    }
    else {
        LOG_ERROR_ALL("exec sql `%s` failed, detail: %s", sql, this->get_last_error() );
        return false;
    } 
}

my_ulonglong mysql_client::get_insert_id(void)
{
    return mysql_insert_id(&this->internal_db_);
}

 const char*  mysql_client::get_last_error(void)
 {
     return mysql_error(&this->internal_db_);
 }

int mysql_client::get_last_errno(void)
{
    return mysql_errno(&this->internal_db_);
}

bool  mysql_client::is_lost(void)
{
    int ec = 0;
    return !this->is_connected() || 
        (ec = this->get_last_errno()) == CR_SERVER_GONE_ERROR ||
		ec == CR_SERVER_LOST || 
        mysql_ping(&this->internal_db_) != 0;    
}

bool  mysql_client::is_connected(void)
{
    return this->internal_db_.db != nullptr;
}

void mysql_client::connect(void)
{
    (void)mysql_init(&this->internal_db_);

    if(mysql_real_connect(&this->internal_db_, "127.0.0.1", "root", "BfKWSMeshh9v7Gmp", "tiny_flower", 3306, NULL, 0) != NULL)
    {
        if(0 != mysql_real_query(&this->internal_db_, "set names 'utf8';", sizeof("set names 'utf8';") - 1))
        {
            LOG_TRACE_ALL("%s", "set char-set to utf8 failed!");
            this->close();
        }
    }
    else {
        LOG_ERROR("%s", "connect to MYSQL server failed!");
    }
}

void  mysql_client::close(void)
{
    if(is_connected())
    {
        mysql_close(&this->internal_db_);
        memset(&this->internal_db_, 0x0, sizeof(this->internal_db_));
    }
}

__threadlocal mysql_client* sqld2;

void          mysql_client::thread_init(void)
{
    mtx.lock();
    sqld2 = new mysql_client();
    mtx.unlock();
}

void  mysql_client::thread_end(void)
{
    mtx.lock();
    delete sqld2;
    mysql_thread_end();
    mtx.unlock();
}
