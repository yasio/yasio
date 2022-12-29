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
  select_fd_set() { reset(); }

  select_fd_set& operator=(select_fd_set& rhs)
  {
    ::memcpy(this->fd_set_, rhs.fd_set_, sizeof(rhs.fd_set_));
    max_nfds_ = rhs.max_nfds_;
    return *this;
  }

  int do_poll(long long wait_duration)
  {
    timeval waitd_tv = {(decltype(timeval::tv_sec))(wait_duration / 1000000), (decltype(timeval::tv_usec))(wait_duration % 1000000)};
    return ::select(this->max_nfds_, &(fd_set_[read_op]), &(fd_set_[write_op]), nullptr, &waitd_tv);
  }

  int has_events(socket_native_type fd, int events) const
  {
    int retval = 0;
    if (events & socket_event::read)
      retval |= FD_ISSET(fd, &fd_set_[read_op]);
    if (events & socket_event::write)
      retval |= FD_ISSET(fd, &fd_set_[write_op]);
    if (events & socket_event::error)
      retval |= FD_ISSET(fd, &fd_set_[except_op]);
    return retval;
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
    if (yasio__testbits(events, socket_event::read))
      FD_SET(fd, &(fd_set_[read_op]));

    if (yasio__testbits(events, socket_event::write))
      FD_SET(fd, &(fd_set_[write_op]));

    if (yasio__testbits(events, socket_event::error))
      FD_SET(fd, &(fd_set_[except_op]));

    if (max_nfds_ < static_cast<int>(fd) + 1)
      max_nfds_ = static_cast<int>(fd) + 1;
  }

  void deregister_descriptor(socket_native_type fd, int events)
  {
    if (yasio__testbits(events, socket_event::read))
      FD_CLR(fd, &(fd_set_[read_op]));

    if (yasio__testbits(events, socket_event::write))
      FD_CLR(fd, &(fd_set_[write_op]));

    if (yasio__testbits(events, socket_event::error))
      FD_CLR(fd, &(fd_set_[except_op]));
  }

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
