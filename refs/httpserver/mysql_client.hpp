//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_MYSQL_AGENT_HPP
#define HTTP_MYSQL_AGENT_HPP
#include "thelib/utils/mysqlcc.h"
#include "thelib/utils/oslib.h"
#include <unordered_map>
#include <mutex>

class mysql_client
{
public:
    mysql_client(void);
	~mysql_client(void);
    // void init(void);

	bool                exec_query0(const char* sqlFormat, ...);
	mysqlcc::result_set exec_query1(const char* sqlFormat, ...);
	
    bool                exec_query_i(const char* sql, unsigned int length, mysqlcc::result_set* result = nullptr);
	
	my_ulonglong        get_insert_id(void);

    const char*         get_last_error(void);
	int                 get_last_errno(void);

    bool                is_lost(void);
    bool                is_connected(void);

public:
    static void          thread_init(void);
    static void          thread_end(void);
    
private:

	void                connect(void);
    void                close(void);

    MYSQL               internal_db_;
};

extern __threadlocal mysql_client* sqld2;

//#define sqld thelib::gc::singleton<mysql_client>::instance()

#endif // HTTP_MYSQL_AGENT_HPP

