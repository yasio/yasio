//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
//
// detail/poll_reactor.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2012-2022 HALX99 (halx99 at live dot com)
#pragma once

#include <vector>
#include "yasio/detail/socket.hpp"

namespace yasio
{
YASIO__NS_INLINE
namespace inet
{
class select_fd_set {
public:
  select_fd_set() { 
    reset();
  }
  // TODO: cb
  int poll_events(int timeout, select_fd_set& revents)
  {
    // std::copy(this->fd_set_.begin(), this->fd_set_.end(), std::back_inserter(revents.fd_set_));
    // return ::poll(revents, revents.count(), timeout);
  }

  int has_events(socket_native_type fd, int events) const
  {
    
  }

  void reset()
  {
    FD_ZERO(&fd_set_[read_op]);
    FD_ZERO(&fd_set_[write_op]);
    FD_ZERO(&fd_set_[except_op]);
    max_nfds_ = 0;
  }

  void register_descriptor(socket_native_type fd, int events)
  {
    if (yasio__testbits(events, YEM_POLLIN))
      FD_SET(fd, &(fd_set_[read_op]));

    if (yasio__testbits(events, YEM_POLLOUT))
      FD_SET(fd, &(fd_set_[write_op]));

    if (yasio__testbits(events, YEM_POLLERR))
      FD_SET(fd, &(fd_set_[except_op]));

    if (max_nfds_ < static_cast<int>(fd) + 1)
      max_nfds_ = static_cast<int>(fd) + 1;
  }

  void deregister_descriptor(socket_native_type fd, int events)
  {
    if (yasio__testbits(events, YEM_POLLIN))
      FD_CLR(fd, &(fd_set_[read_op]));

    if (yasio__testbits(events, YEM_POLLOUT))
      FD_CLR(fd, &(fd_set_[write_op]));

    if (yasio__testbits(events, YEM_POLLERR))
      FD_CLR(fd, &(fd_set_[except_op]));
  }

protected:

protected:
  enum
  {
    read_op,
    write_op,
    except_op,
    max_ops,
  };
  fd_set fd_set_[max_ops];
  int max_nfds_ = 0;
};
} // namespace inet
} // namespace yasio
