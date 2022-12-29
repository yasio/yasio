//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
//
// detail/poll_event_registry.hpp
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
class poll_fd_set {
public:
  poll_fd_set& operator=(poll_fd_set& rhs)
  {
    this->fd_set_ = rhs.fd_set_;
    return *this;
  }

  int do_poll(long long wait_duration) { return ::poll(this->fd_set_.data(), static_cast<int>(this->fd_set_.size()), wait_duration / 1000); }

  int has_events(socket_native_type fd, int events) const
  {
    int underlying_events = 0;
    if (events & socket_event::read)
      underlying_events |= POLLIN;
    if (events & socket_event::write)
      underlying_events |= POLLOUT;
    if (events & socket_event::error)
      underlying_events |= (POLLERR | POLLHUP | POLLNVAL);
    auto it = std::find_if(this->fd_set_.begin(), this->fd_set_.end(), [fd](const pollfd& pfd) { return pfd.fd == fd; });
    return it != this->fd_set_.end() ? (it->revents & underlying_events) : 0;
  }

  void reset() { this->fd_set_.clear(); }

  void register_descriptor(socket_native_type fd, int events)
  {
    int underlying_flags = 0;
    if (yasio__testbits(events, socket_event::read))
      underlying_flags |= POLLIN;

    if (yasio__testbits(events, socket_event::write))
      underlying_flags |= POLLOUT;

    if (yasio__testbits(events, socket_event::error))
      underlying_flags |= POLLERR;
    pollfd_mod(this->fd_set_, fd, underlying_flags, 0);
  }

  void deregister_descriptor(socket_native_type fd, int events)
  {
    int underlying_flags = 0;
    if (yasio__testbits(events, socket_event::read))
      underlying_flags |= POLLIN;

    if (yasio__testbits(events, socket_event::write))
      underlying_flags |= POLLOUT;

    if (yasio__testbits(events, socket_event::error))
      underlying_flags |= POLLERR;

    pollfd_mod(this->fd_set_, fd, 0, underlying_flags);
  }

protected:
  static void pollfd_mod(std::vector<pollfd>& fdset, socket_native_type fd, int add_flags, int remove_flags)
  {
    auto it = std::find_if(fdset.begin(), fdset.end(), [fd](const pollfd& pfd) { return pfd.fd == fd; });
    if (it != fdset.end())
    {
      // POLLIN
      it->events |= add_flags;
      it->events &= ~remove_flags;
      if (it->events == 0)
        fdset.erase(it);
    }
    else
    {
      auto combined_flags = add_flags & ~remove_flags;
      if (combined_flags)
        fdset.push_back(pollfd{fd, static_cast<short>(combined_flags), 0});
    }
  }

protected:
  std::vector<pollfd> fd_set_;
};
} // namespace inet
} // namespace yasio
