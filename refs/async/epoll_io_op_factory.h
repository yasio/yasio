#include "epoll_recv_op.h"
#include "epoll_accept_op.h"

class epoll_io_op_factory 
{
public:
    template<typename _Handler> inline
    static epoll_accept_op<_Handler>* make_accept_op(xxsocket& listen_sock, _Handler handler)
    {
        return new epoll_accept_op<_Handler>(listen_sock, handler);
    }

    template<typename _Handler> inline
    static epoll_recv_op<_Handler>*   make_recv_op(xxsocket& sock, void* buffer, size_t size, _Handler handler)
    {
        return new epoll_recv_op<_Handler>(sock, buffer, size, handler);
    }

};



