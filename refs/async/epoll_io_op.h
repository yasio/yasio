#ifndef _EPOLL_IO_OP_H_
#define _EPOLL_IO_OP_H_
#include "epoll_fwd.h"

class epoll_io_op
{
    typedef void (*func_type)(epoll_io_service*, 
        epoll_io_op*, 
        u_long ec, 
        size_t bytes_transferred);
public:
    epoll_io_op(func_type func) : func_(func)
    {
    }

    void complete(epoll_io_service& owner, u_long ec, size_t bytes_transferred)
    {
        this->func_(&owner, this, ec, bytes_transferred);
    }

private:
    func_type func_;
};

#endif
