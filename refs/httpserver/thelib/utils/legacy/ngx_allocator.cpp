#include "ngx_allocator.h"

ngx::mempool ngx::_Mempool;

using namespace ngx;

mempool::mempool(size_t pool_size)
{
	u_char 	*space;

	pool_size = 40960;  //4M 
	space = (u_char *)malloc(pool_size);
	slab_ = (ngx_slab_pool_t*) space;
    slab_->addr = space;
	slab_->min_shift = 3;
	slab_->end = space + pool_size;
    ngx_slab_init(slab_);
    /*this->slab_ = (ngx_slab_pool_t*)malloc(pool_size + sizeof(ngx_slab_pool_t));
    if(this->slab_ != nullptr)
    {
        memset(this->slab_, 0, sizeof(ngx_slab_pool_t));

        slab_->addr = slab_;
        slab_->min_shift = 3;
        slab_->end = (u_char*)slab_ + pool_size;

        ngx_slab_init(this->slab_);
    }*/
}

mempool::~mempool(void)
{
    if(this->slab_ != nullptr)
    {
        free(this->slab_);
    }
}

void* mempool::get(size_t size)
{
    //std::unique_lock<std::mutex> guard(mtx_);
    return malloc(size);//ngx_slab_alloc_locked(this->slab_, size);
}

void  mempool::release(void* pUserData)
{
    //std::unique_lock<std::mutex> guard(mtx_);
    (void) free(pUserData);//ngx_slab_free_locked(this->slab_, pUserData);
}

