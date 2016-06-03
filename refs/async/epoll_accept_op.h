#ifndef _EPOLL_ACCEPT_OP_H_
#define _EPOLL_ACCEPT_OP_H_
#include "epoll_io_service.h"
#include "epoll_io_op.h"

template<typename _Handler>
class epoll_accept_op : public epoll_io_op
{
public:
    epoll_accept_op(xxsocket& listen_sock, _Handler handler) : epoll_io_op(&do_complete), 
        listen_sock_(listen_sock),
        handler_(handler)
    {
    }

    static void do_complete(epoll_io_service* owner, epoll_io_op* base,                                  
        u_long,                                                   
        std::size_t /*bytes_transferred*/)
    {
        epoll_accept_op* op(static_cast<epoll_accept_op*>(base));
        xxsocket news = op->listen_sock_.accept();
        owner->resume_handler(op->listen_sock_.native_handle(), op);
        news.set_nonblocking();
        op->handler_(news, errno);
    }

private:
    xxsocket&      listen_sock_;
    _Handler       handler_;
};
#endif

