#include "MemcachedAgent.h"
#pragma comment(lib, "libmemcached.lib")

MemcachedAgent::MemcachedAgent(const unreal_string<char>& hostname, u_short port)
{
    this->memc = memcached_create(nullptr); 
    if(this->memc != nullptr)
    {
        memcached_server_add(this->memc, hostname.c_str(), port); 
    }
}

MemcachedAgent::~MemcachedAgent(void)
{
    memcached_free(this->memc);
}

bool MemcachedAgent::set(const unreal_string<char>& key, const unreal_string<char>& value)
{
    time_t expiration = std::numeric_limits<long>::max();
    return MEMCACHED_SUCCESS == memcached_set(this->memc, 
        key.c_str(), 
        key.length(), 
        value.c_str(), 
        value.length(), expiration,
        0);
}

bool MemcachedAgent::get(const unreal_string<char>& key, unreal_string<char, pod_free>& value)
{
    uint32_t flags = 0;
    memcached_return ret;
    value.ptr_ref() = memcached_get(this->memc, key, key.length(), &value.length_ref(), &flags, &ret);
    return MEMCACHED_SUCCESS == ret;
}

bool MemcachedAgent::remove(const unreal_string<char>& key)
{
    time_t expiration = 0;
    return MEMCACHED_SUCCESS == memcached_delete(this->memc, key.c_str(), key.length(), 1);
}


