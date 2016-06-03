#ifndef _IOCP_FWD_H_
#define _IOCP_FWD_H_
#include "simpleppdef.h"
#include "xxsocket.h"
#include "object_pool.h"
#include "threadpool.h"
#include <list>

using namespace thelib::net;
using namespace thelib::asy;
using namespace thelib::gc;

static const int OP_MAX_SIZE = 64;

class iocp_io_op;

template<typename _Handler>
class iocp_send_op;

template<typename _Handler>
class iocp_recv_op;

template<typename _Handler>
class iocp_accept_op;

class tcp_acceptor;

class tcp_connector;

class iocp_io_service;

struct op_max_capacity
{
    char place[OP_MAX_SIZE];
};

extern object_pool<op_max_capacity> iocp_op_gc;

#endif

/*
* Copyright (c) 2012 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
