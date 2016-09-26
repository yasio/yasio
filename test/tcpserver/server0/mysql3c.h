/***************************************************************
* mysql3c.h - implement c++ simple API for mysql
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: operate mysql conveniently by c++
* 
****************************************************************/
#ifndef _MYSQL3C_H_
#define _MYSQL3C_H_
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <mysql.h>
#include <simple++/thread_synch.h>
#ifdef __GNUC__
#if __cplusplus < 201103L
#ifndef nullptr
#define nullptr NULL
#endif
#endif
#endif

namespace mysql3c {

    typedef MYSQL_BIND user_data;

    // CLASS fwd
    class database;

    class result_set
    {
        friend class database;
    public:
        result_set(void);
        ~result_set(void);

        bool is_valid(void) const;

        my_ulonglong count_of_rows(void);

        my_ulonglong count_of_fields(void);

        /* return data as array of strings */
        char** fetch_row(void);

    private:
        MYSQL_RES* res;
    };

    class statement
    {
        friend class database;
    public:
        statement(void);

        ~statement(void);

        int  prepare(const char* sqlstr, enum_field_types types[], int count);

        template<typename _Intty>
        void fill(const _Intty& value, int index)
        {
           *( (_Intty*)this->rowbuf[index].buffer ) = value;
        }

        void fill(const void* value, unsigned long valuelen, int index);

        int  execute(void);

        int  commit(void);

        void reset(void);

        void release(void);

    private:
        MYSQL_STMT* stmt;
        user_data*  rowbuf;
        size_t      column_count;
    };

    class database
    {
    public:
        database(void);
        ~database(void);

        bool  get_statement(statement& stmt);

        bool  open(const char* dbname, const char* addr, u_short port, const char* user, const char* password);

        bool  is_open(void) const;

        void  close(void);

        /*
        ** @brief: execute query SQL
        ** params: 
        **       sqlstr: SQL cmd to execute, such as: "select * from test where cout=19"
        **       resultset: output parameter to save resultset for a query
        **                  default: nullptr(ignore resultset)
        */
        bool  exec_query(const char* sqlstr, result_set* resultset = nullptr);

    private:
        MYSQL handle;
        exx::asy::thread_mutex mtx;
    };

};

#endif

