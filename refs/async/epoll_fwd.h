// epoll_fwd.h
#ifndef _EPOLL_FWD_H_
#define _EPOLL_FWD_H_

#include "simpleppdef.h"
#include "xxsocket.h"
#include "object_pool.h"
#include "threadpool.h"
#include <list>

using namespace exx::net;
using namespace exx::asy;
using namespace exx::gc;


#ifdef _WIN32
#ifdef _DEBUG
#ifndef _WIN64
#pragma comment(lib, "simplepp_debug.lib")
#else
#pragma comment(lib, "../x64/Debug/pretty_debug.lib")
#endif
#else
#ifndef _WIN64
#pragma comment(lib, "pretty_release.lib")
#else
#pragma comment(lib, "../x64/Debug/pretty_release.lib")
#endif
#endif    
#endif

static const int EPOLL_MAX_EVENTS = 20000;
static const uint32_t EV_SHOT_VALUE = EPOLLIN | EPOLLET | EPOLLONESHOT;
static const uint32_t EV_LEVEL_VALUE = EPOLLIN;
static const int EPOLL_MAX_OP_SIZE = 64;

class epoll_io_op;

template<typename _Handler>
class epoll_accept_op;

template<typename _Handler>
class epoll_recv_op;

class epoll_op_factory;
class epoll_io_service;

struct op_max_capacity
{
    char place[EPOLL_MAX_OP_SIZE];
};

extern object_pool<op_max_capacity> epoll_op_gc;

#endif

/*****************************************************************
** Copyright (c) 2012-2019 by X.D. Guo  ALL RIGHTS RESERVED.    **
** Consult your license regarding permissions and restrictions. **
*****************************************************************/

