#ifndef _IOCP_ACCEPT_OP_H_
#define _IOCP_ACCEPT_OP_H_
#include "iocp_io_op.h"

template<typename _Handler>
class iocp_accept_op : public iocp_op
{
public:
    iocp_accept_op(
        xxsocket& listen_sock, 
        xxsocket& new_sock, 
        _Handler handler) 
        : iocp_op(&do_complete), 
        listen_sock_(listen_sock),
        new_sock_(new_sock),
        handler_(handler)
    {
    }

    static void do_complete(iocp_io_service* owner, iocp_io_op* base,                                  
        u_long result_ec,                                                   
        std::size_t /*bytes_transferred*/)
    {
        iocp_accept_op* op(static_cast<iocp_accept_op*>(base));

        op->new_sock_.set_optval(SOL_SOCKET, 
            SO_UPDATE_ACCEPT_CONTEXT, 
            op->listen_sock_.native_handle());

        owner->register_handle(op->new_sock_.native_handle());

        op->handler_(result_ec);
    }

private:
    xxsocket&      listen_sock_;
    xxsocket&      new_sock_;
    _Handler       handler_;
};



#endif

/*
* Copyright (c) 2012 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/
