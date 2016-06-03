#ifndef _IOCP_OP_FACTORY_H_
#define _IOCP_OP_FACTORY_H_
#include "iocp_accept_op.h"
#include "iocp_recv_op.h"
#include "iocp_send_op.h"

class iocp_io_op_factory
{
public:
    template<typename _Handler>
    static iocp_accept_op<_Handler>* make_accept_op(xxsocket& listen_sock, xxsocket& new_sock, _Handler handler)
    {
        return new(iocp_op_gc.get()) iocp_accept_op<_Handler>(listen_sock, new_sock, handler);
    }

    template<typename _Handler>
    static iocp_recv_op<_Handler>* make_recv_op(xxsocket& sock, void* buffer, size_t size, _Handler handler)
    {
        return new(iocp_op_gc.get()) iocp_recv_op<_Handler>(sock, buffer, size, handler);
    }

    template<typename _Handler>
    static iocp_send_op<_Handler>* make_send_op(xxsocket& sock, void* content, size_t size, _Handler handler)
    {
        return new(iocp_op_gc.get()) iocp_send_op<_Handler>(sock, content, size, handler);
    }
};


template<typename _Handler>
void async_recv(xxsocket& sock, void* buffer, size_t size, _Handler handler)
{
    auto op = iocp_io_op_factory::make_recv_op(sock,
        buffer, 
        size, 
        handler);

    DWORD flags = 0;
    ::WSARecv(sock.native_handle(),
        op->buffer(),
        1,
        nullptr,
        &flags,
        op,
        nullptr);
}

template<typename _Handler>
void async_send(xxsocket& sock, void* content, size_t length, _Handler handler)
{
    auto op = iocp_io_op_factory::make_send_op(sock,
        content, 
        length, 
        handler);

    DWORD flags = 0;
    ::WSASend(sock.native_handle(),
        op->buffer(),
        1,
        nullptr,
        flags,
        op,
        nullptr);
}

#endif
/*
* Copyright (c) 2012-2019 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

