//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
// version: 2.2
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2017 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef _IMPL_XXSOCKET_SELECT_INTERRUPTER_IPP_
#define _IMPL_XXSOCKET_SELECT_INTERRUPTER_IPP_

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)
// #include "select_interrupter.h"

namespace purelib {
namespace inet {

socket_select_interrupter::socket_select_interrupter()
{
    open_descriptors();
}

void socket_select_interrupter::open_descriptors()
{
    xxsocket acceptor(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    acceptor.set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

    int error = 0;
    ip::endpoint ep("127.0.0.1", 0);

    error = acceptor.bind(ep);
    ep = acceptor.local_endpoint();
    // Some broken firewalls on Windows will intermittently cause getsockname to
    // return 0.0.0.0 when the socket is actually bound to 127.0.0.1. We
    // explicitly specify the target address here to work around this problem.
    // addr.sin_addr.s_addr = socket_ops::host_to_network_long(INADDR_LOOPBACK);
    // cp.address("127.0.0.1");
    ep.address("127.0.0.1");
    error = acceptor.listen();

    xxsocket client(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    error = client.connect(ep);

    auto server = acceptor.accept();

    client.set_nonblocking(true);
    client.set_optval(IPPROTO_TCP, TCP_NODELAY, 1);

    server.set_nonblocking(true);
    client.set_optval(IPPROTO_TCP, TCP_NODELAY, 1);

    read_descriptor_ = server.release();
    write_descriptor_ = client.release();
}

socket_select_interrupter::~socket_select_interrupter()
{
    close_descriptors();
}

void socket_select_interrupter::close_descriptors()
{
    if (read_descriptor_ != invalid_socket)
        ::closesocket(read_descriptor_);

    if (write_descriptor_ != invalid_socket)
        ::closesocket(write_descriptor_);
}

void socket_select_interrupter::recreate()
{
    close_descriptors();

    write_descriptor_ = invalid_socket;
    read_descriptor_ = invalid_socket;

    open_descriptors();
}

void socket_select_interrupter::interrupt()
{
    xxsocket::send_i(write_descriptor_, "\0", 1);
}

bool socket_select_interrupter::reset()
{
    char buffer[1024];
    int bytes_read = xxsocket::recv_i(read_descriptor_, buffer, sizeof(buffer), 0);
    bool was_interrupted = (bytes_read > 0);
    while (bytes_read == sizeof(buffer))
        bytes_read = xxsocket::recv_i(read_descriptor_, buffer, sizeof(buffer), 0);
    return was_interrupted;
}

} // namespace inet
} // namespace purelib

#endif 
