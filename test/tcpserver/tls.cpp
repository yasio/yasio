#include "tls.hpp"
#include "utils/oslib.h"
#include "utils/object_pool.h"
#include "utils/mempool2.h"
#include "connection.hpp"
#include <mutex>
using namespace tcp::server;

__threadlocal object_pool<connection>* s_connection_pool;
__threadlocal mempool2* s_memory_pool;

void tls_ctx_init()
{
    s_connection_pool = new object_pool<connection>();
    s_memory_pool = new mempool2();
}

void tls_ctx_end()
{
    delete s_memory_pool;
    delete s_connection_pool;
}

void* tls_get_connection()
{
    return s_connection_pool->get();
}

void tls_release_connection(void* p)
{
    s_connection_pool->release(p);
}

void* tls_get_buffer(size_t size)
{
    return s_memory_pool->allocate(size);
}

void tls_release_buffer(void* p)
{
    s_memory_pool->deallocate(p);
}

//////////////////////// global ctx ////////////////////////////////
static std::mutex g_connection_pool_mtx_;
static std::mutex g_memory_pool_mtx;
static object_pool<connection>* g_connection_pool;
static mempool2* g_memory_pool;
void gls_ctx_init()
{
    g_connection_pool = new object_pool<connection>();
    g_memory_pool = new mempool2();
}

void gls_ctx_end()
{
    delete s_memory_pool;
    delete s_connection_pool;
}

void* gls_get_connection()
{
    g_connection_pool_mtx_.lock();
    void* p = s_connection_pool->get();
    g_connection_pool_mtx_.unlock();

    return p;
}

void gls_release_connection(void* p)
{
    g_connection_pool_mtx_.lock();
    s_connection_pool->release(p);
    g_connection_pool_mtx_.unlock();
}

void* gls_get_buffer(size_t size)
{
    g_memory_pool_mtx.lock();
    void* p = s_memory_pool->allocate(size);
    g_memory_pool_mtx.unlock();

    return p;
}

void gls_release_buffer(void*p)
{
    g_memory_pool_mtx.lock();
    s_memory_pool->deallocate(p);
    g_memory_pool_mtx.unlock();
}
