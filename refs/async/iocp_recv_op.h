#ifndef _IOCP_RECV_OP_
#define _IOCP_RECV_OP_
#include "iocp_io_op.h"

template<typename _Handler>
class iocp_recv_op : public iocp_io_op
{
public:
    iocp_recv_op(
        xxsocket& sock, 
        void*    buffer,
        size_t   size,
        _Handler handler) 
        : iocp_io_op(&do_complete), 
        sock_(sock),
        handler_(handler)
    {
        this->wsabuf_.buf = (CHAR*)buffer;
        this->wsabuf_.len = size;
    }

    static void do_complete(
        iocp_io_service* owner, 
        iocp_io_op* base,                                  
        u_long result_ec,                                                   
        std::size_t bytes_transferred)
    {
        iocp_recv_op* op(static_cast<iocp_recv_op*>(base));

        op->handler_(result_ec, bytes_transferred);
    }

    WSABUF*        buffer(void)
    {
        return &this->wsabuf_;
    }

private:
    xxsocket&      sock_;
    _Handler       handler_;
    WSABUF         wsabuf_;
    
};



#endif

/*
* Copyright (c) 2012 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
