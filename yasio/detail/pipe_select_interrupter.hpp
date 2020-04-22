//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
//
// detail/pipe_select_interrupter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#ifndef YASIO__PIPE_SELECT_INTERRUPTER_HPP
#define YASIO__PIPE_SELECT_INTERRUPTER_HPP

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace yasio
{
namespace inet
{

class pipe_select_interrupter
{
public:
  // Constructor.
  inline pipe_select_interrupter() { open_descriptors(); }

  // Destructor.
  inline ~pipe_select_interrupter() { close_descriptors(); }

  // Recreate the interrupter's descriptors. Used after a fork.
  inline void recreate()
  {
    close_descriptors();

    write_descriptor_ = -1;
    read_descriptor_  = -1;

    open_descriptors();
  }

  // Interrupt the select call.
  inline void interrupt()
  {
    char byte   = 0;
    auto result = ::write(write_descriptor_, &byte, 1);
    (void)result;
  }

  // Reset the select interrupt. Returns true if the call was interrupted.
  inline bool reset()
  {
    for (;;)
    {
      char data[1024];
      auto bytes_read = ::read(read_descriptor_, data, sizeof(data));
      if (bytes_read < 0 && errno == EINTR)
        continue;
      bool was_interrupted = (bytes_read > 0);
      while (bytes_read == sizeof(data))
        bytes_read = ::read(read_descriptor_, data, sizeof(data));
      return was_interrupted;
    }
  }

  // Get the read descriptor to be passed to select.
  int read_descriptor() const { return read_descriptor_; }

private:
  // Open the descriptors. Throws on error.
  inline void open_descriptors()
  {
    int pipe_fds[2];
    if (pipe(pipe_fds) == 0)
    {
      read_descriptor_ = pipe_fds[0];
      ::fcntl(read_descriptor_, F_SETFL, O_NONBLOCK);
      write_descriptor_ = pipe_fds[1];
      ::fcntl(write_descriptor_, F_SETFL, O_NONBLOCK);

#if defined(FD_CLOEXEC)
      ::fcntl(read_descriptor_, F_SETFD, FD_CLOEXEC);
      ::fcntl(write_descriptor_, F_SETFD, FD_CLOEXEC);
#endif // defined(FD_CLOEXEC)
    }
  }

  // Close the descriptors.
  inline void close_descriptors()
  {
    if (read_descriptor_ != -1)
      ::close(read_descriptor_);
    if (write_descriptor_ != -1)
      ::close(write_descriptor_);
  }

  // The read end of a connection used to interrupt the select call. This file
  // descriptor is passed to select such that when it is time to stop, a single
  // byte will be written on the other end of the connection and this
  // descriptor will become readable.
  int read_descriptor_;

  // The write end of a connection used to interrupt the select call. A single
  // byte may be written to this to wake up the select which is waiting for the
  // other end to become readable.
  int write_descriptor_;
};

} // namespace inet
} // namespace yasio

#endif // YASIO__PIPE_SELECT_INTERRUPTER_HPP
