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

#ifndef YASIO_PIPE_SELECT_INTERRUPTER_HPP
#define YASIO_PIPE_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

namespace purelib {
namespace inet {

class pipe_select_interrupter
{
public:
  // Constructor.
  _YASIO_INLINE pipe_select_interrupter();

  // Destructor.
  _YASIO_INLINE ~pipe_select_interrupter();

  // Recreate the interrupter's descriptors. Used after a fork.
  _YASIO_INLINE void recreate();

  // Interrupt the select call.
  _YASIO_INLINE void interrupt();

  // Reset the select interrupt. Returns true if the call was interrupted.
  _YASIO_INLINE bool reset();

  // Get the read descriptor to be passed to select.
  int read_descriptor() const
  {
    return read_descriptor_;
  }

private:
  // Open the descriptors. Throws on error.
  _YASIO_INLINE void open_descriptors();

  // Close the descriptors.
  _YASIO_INLINE void close_descriptors();

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
} // namespace purelib


# include "pipe_select_interrupter.ipp"

#endif // YASIO_PIPE_SELECT_INTERRUPTER_HPP
