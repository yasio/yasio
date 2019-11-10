//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
//
// detail/impl/pipe_select_interrupter.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef YASIO__PIPE_SELECT_INTERRUPTER_IPP
#define YASIO__PIPE_SELECT_INTERRUPTER_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace yasio
{
namespace inet
{
pipe_select_interrupter::pipe_select_interrupter() { open_descriptors(); }

void pipe_select_interrupter::open_descriptors()
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

pipe_select_interrupter::~pipe_select_interrupter() { close_descriptors(); }

void pipe_select_interrupter::close_descriptors()
{
  if (read_descriptor_ != -1)
    ::close(read_descriptor_);
  if (write_descriptor_ != -1)
    ::close(write_descriptor_);
}

void pipe_select_interrupter::recreate()
{
  close_descriptors();

  write_descriptor_ = -1;
  read_descriptor_  = -1;

  open_descriptors();
}

void pipe_select_interrupter::interrupt()
{
  char byte   = 0;
  auto result = ::write(write_descriptor_, &byte, 1);
  (void)result;
}

bool pipe_select_interrupter::reset()
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

} // namespace inet
} // namespace yasio

#endif // YASIO__PIPE_SELECT_INTERRUPTER_IPP
