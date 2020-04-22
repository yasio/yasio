//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
//
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
Copyright (c) 2008 Roelof Naude (roelof.naude at gmail dot com)

Distributed under the Boost Software License, Version 1.0. (See accompanying
file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

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
#ifndef YASIO__SOCKET_SELECT_INTERRUPTER_HPP
#define YASIO__SOCKET_SELECT_INTERRUPTER_HPP
#include "yasio/xxsocket.hpp"

namespace yasio
{
namespace inet
{
class socket_select_interrupter
{
public:
  // Constructor.
  inline socket_select_interrupter() { open_descriptors(); }

  // Destructor.
  inline ~socket_select_interrupter() { close_descriptors(); }

  // Recreate the interrupter's descriptors. Used after a fork.
  inline void recreate()
  {
    close_descriptors();

    write_descriptor_ = invalid_socket;
    read_descriptor_  = invalid_socket;

    open_descriptors();
  }

  // Interrupt the select call.
  inline void interrupt() { xxsocket::send(write_descriptor_, "\0", 1); }

  // Reset the select interrupt. Returns true if the call was interrupted.
  inline bool reset()
  {
    char buffer[1024];
    int bytes_read       = xxsocket::recv(read_descriptor_, buffer, sizeof(buffer), 0);
    bool was_interrupted = (bytes_read > 0);
    while (bytes_read == sizeof(buffer))
      bytes_read = xxsocket::recv(read_descriptor_, buffer, sizeof(buffer), 0);
    return was_interrupted;
  }

  // Get the read descriptor to be passed to select.
  socket_native_type read_descriptor() const { return read_descriptor_; }

private:
  // Open the descriptors. Throws on error.
  inline void open_descriptors()
  {
    xxsocket acceptor(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    acceptor.set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

    int error = 0;
    ip::endpoint ep("127.0.0.1", 0);

    error = acceptor.bind(ep);
    ep    = acceptor.local_endpoint();
    // Some broken firewalls on Windows will intermittently cause getsockname to
    // return 0.0.0.0 when the socket is actually bound to 127.0.0.1. We
    // explicitly specify the target address here to work around this problem.
    // addr.sin_addr.s_addr = socket_ops::host_to_network_long(INADDR_LOOPBACK);
    ep.ip("127.0.0.1");
    error = acceptor.listen();

    xxsocket client(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    error = client.connect(ep);

    auto server = acceptor.accept();

    client.set_nonblocking(true);
    client.set_optval(IPPROTO_TCP, TCP_NODELAY, 1);

    server.set_nonblocking(true);
    server.set_optval(IPPROTO_TCP, TCP_NODELAY, 1);

    read_descriptor_  = server.detach();
    write_descriptor_ = client.detach();
  }

  // Close the descriptors.
  inline void close_descriptors()
  {
    if (read_descriptor_ != invalid_socket)
      ::closesocket(read_descriptor_);

    if (write_descriptor_ != invalid_socket)
      ::closesocket(write_descriptor_);
  }

  // The read end of a connection used to interrupt the select call. This file
  // descriptor is passed to select such that when it is time to stop, a single
  // byte will be written on the other end of the connection and this
  // descriptor will become readable.
  socket_native_type read_descriptor_;

  // The write end of a connection used to interrupt the select call. A single
  // byte may be written to this to wake up the select which is waiting for the
  // other end to become readable.
  socket_native_type write_descriptor_;
};

} // namespace inet
} // namespace yasio

#endif // YASIO__SOCKET_SELECT_INTERRUPTER_HPP
