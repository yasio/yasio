#include "mempool.h"

thelib::gc::mempool thelib::gc::mempool::global_mp;

thelib::gc::mempool::mempool(size_t size)
{
    init(size);
}
thelib::gc::mempool::~mempool(void)
{
    free(memory_);
}

void thelib::gc::mempool::init(size_t size)
{
    size = sz_align(size, SZ(64, k));
    memory_ = malloc(size);

    memblock_head_t* header = (memblock_head_t*)(memory_);
    header->size = size;
    header->unused = true;
    header->prev = header;
    header->next = header;

    this->freelist_ = FOOT_LOC(header)->uplink = header;
    this->memory_size_ = size;
}

void* thelib::gc::mempool::allocate(size_t size)
{
#ifdef EANBLE_MULTI_THREAD
    std::unique_lock<std::mutex> guard(this->mtx_);
#endif

    size_t n = sz_align(size, min_size);

    memblock_head_t* blk = freelist_;
    for(; blk != nullptr && 
        blk->size - reserved_size < n && 
        blk->next != freelist_; blk = blk->next){;}

    if(blk == nullptr || blk->size - reserved_size < n) {
        return malloc(size);
    }
    else {
        blk->unused = false;
        char* p = (char*)blk + sizeof(memblock_head_t);

        if( REMAIN_SIZE(blk, n) < min_block_size ) /* can't construct the new memory block to respond any alloc request. */
        {
            // blk->size = 
            if(blk == freelist_) {
                freelist_ = nullptr;
            }
            else {
                freelist_->prev = blk->next; blk->next->prev = freelist_;
            }
        }
        else {
            /* remain block */
            memblock_head_t* reblock = (memblock_head_t*)(p + n + sizeof(memblock_foot_t));
            reblock->unused = true;
            reblock->size = (char*)blk + blk->size - (char*)reblock;
            UPDATE_FOOT(reblock);

            blk->size = (char*)reblock - (char*)blk;
            UPDATE_FOOT(blk);

            if(blk->next == blk)
            { // self
                blk->next = reblock;
                blk->prev = reblock;
                reblock->prev = blk;
                reblock->next = blk;
            }
            else { // insert
                reblock->prev = blk;
                reblock->next = blk->next;

                blk->next->prev = reblock;
                blk->next = reblock;
                // ptr->prev = reblock;
            }

            freelist_ = reblock;
        }
        return p;
    }
}

void thelib::gc::mempool::deallocte(void* pUserData)
{
    if(!belong_to(pUserData))
    {
        free(pUserData);
        return;
    }

#ifdef EANBLE_MULTI_THREAD
    std::unique_lock<std::mutex> guard(this->mtx_);
#endif
    memblock_head_t* curh = HEAD_LOC(pUserData);
    memblock_foot_t* lf = LEFT_FOOT_LOC(curh);
    memblock_head_t* rh = RIGHT_HEAD_LOC(curh);
    bool vlf = is_valid_leftf(lf);
    bool vrh = is_valid_righth(rh);

#ifdef _DEBUG
    memset(pUserData, 0x0, curh->size - reserved_size);
#endif

    if( ( vlf && lf->uplink->unused ) && (!vrh || !rh->unused) )
    { // merge curret to left block
        lf->uplink->next = curh->next;
        if(curh->next != nullptr)
            curh->next->prev = lf->uplink;

        lf->uplink->size += curh->size;

        UPDATE_FOOT(lf->uplink);
        this->freelist_ = lf->uplink;
    }
    else if( (vrh && rh->unused) && (!vlf || !lf->uplink->unused ) )
    { // merge right to current block
        curh->unused = true;
        curh->next = rh->next;
        if(rh->next != nullptr)
            rh->next->prev = curh;
        curh->size += rh->size;

        UPDATE_FOOT(curh);
        this->freelist_ = curh;
    }
    else if( (vlf && lf->uplink->unused) && (vrh && rh->unused) )
    { // merge current and right to left block
        lf->uplink->next = rh->next;
        if(rh->next != nullptr)
            rh->next->prev = lf->uplink;

        lf->uplink->size += (curh->size + rh->size);

        UPDATE_FOOT(lf->uplink);
        this->freelist_ = lf->uplink;
    }
    else {
        // no need merge
        curh->unused = true;
        this->freelist_ = curh;
    }
}

bool thelib::gc::mempool::belong_to(void* p)
{
    return p >= this->memory_ && p < ( (char*)this->memory_ + this->memory_size_ );
}

bool thelib::gc::mempool::is_valid_leftf(void* lf)
{
    return ((char*)lf > ( (char*)this->memory_ + sizeof(memblock_head_t)) );
}

bool thelib::gc::mempool::is_valid_righth(void* rh)
{
    return (char*)rh < ((char*)this->memory_ + memory_size_ - sizeof(memblock_foot_t) );
}

