#ifndef _MEMCACHEDAGENT_H_
#define _MEMCACHEDAGENT_H_
#include <libmemcached/memcached.h>
#ifdef _WIN32
#include <libmemcached/memcached_util.h>
#else
#include <libmemcached/util.h>
#endif
#include "CommonHeader.h"

#include <iostream>
using namespace epp::gc;
class MemcachedAgent
{
public:
    typedef unreal_string<char,pod_free> output_string;

    MemcachedAgent(const unreal_string<char>& hostname, u_short port = 11211);

    ~MemcachedAgent(void);
   
    bool set(const unreal_string<char>& key, const unreal_string<char>& value);

    bool get(const unreal_string<char>& key, unreal_string<char,pod_free>& value);

    bool remove(const unreal_string<char>& key);

private:
    memcached_st *memc;
};

#endif


