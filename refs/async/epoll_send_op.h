#ifndef _EPOLL_RECV_OP_H_
#define _EPOLL_RECV_OP_H_
#include "epoll_io_op.h"

template<typename _Handler>
class epoll_send_op : public epoll_io_op
{
public:
    epoll_send_op(
        xxsocket& sock, 
        void*    buffer,
        size_t   size,
        _Handler handler) 
        : epoll_io_op(&do_complete), 
        sock_(sock),
        handler_(handler)
    {
        this->buffer_ = buffer; 
        this->size_   = size;
    }

    static void do_complete(
        epoll_io_service* owner, 
        epoll_io_op* base,                                  
        u_long result_ec,                                                   
        std::size_t bytes_transferred)
    {
        auto op(static_cast<epoll_send_op*>(base));
        
        bytes_transferred = 0;
        int n = 0, nreq = op->size_;
        char* ptr = (char*)op->buffer_;
        while(bytes_transferred < nreq)
        {
            n = op->sock_.send_i(ptr, nreq);
            result_ec = errno;

            if(n > 0) // receive message sucessfully
            {
                bytes_transferred += n;
                ptr += n;
                nreq -= n;
            }
            else { // EAGAIN, socket recv buf is empty, or error occured, exit loop
               // result_ec = (0 == n || (result_ec != EAGAIN && result_ec != EINTR) );
                break;
            }
        }

        op->handler_(result_ec, bytes_transferred);
    }

private:
    xxsocket&      sock_;
    _Handler       handler_;
    void*          buffer_;
    size_t         size_;
};

#endif

