//
// detail/pipe_select_interrupter.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef XXSOCKET_PIPE_SELECT_INTERRUPTER_HPP
#define XXSOCKET_PIPE_SELECT_INTERRUPTER_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

//#include <boost/asio/detail/config.hpp>
//
//#if !defined(BOOST_ASIO_WINDOWS)
//#if !defined(BOOST_ASIO_WINDOWS_RUNTIME)
//#if !defined(__CYGWIN__)
//#if !defined(__SYMBIAN32__)
//#if !defined(BOOST_ASIO_HAS_EVENTFD)
//
//#include <boost/asio/detail/push_options.hpp>

namespace purelib {
namespace inet {

class pipe_select_interrupter
{
public:
  // Constructor.
  _XXSOCKET_INLINE pipe_select_interrupter();

  // Destructor.
  _XXSOCKET_INLINE ~pipe_select_interrupter();

  // Recreate the interrupter's descriptors. Used after a fork.
  _XXSOCKET_INLINE void recreate();

  // Interrupt the select call.
  _XXSOCKET_INLINE void interrupt();

  // Reset the select interrupt. Returns true if the call was interrupted.
  _XXSOCKET_INLINE bool reset();

  // Get the read descriptor to be passed to select.
  int read_descriptor() const
  {
    return read_descriptor_;
  }

private:
  // Open the descriptors. Throws on error.
  _XXSOCKET_INLINE void open_descriptors();

  // Close the descriptors.
  _XXSOCKET_INLINE void close_descriptors();

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


//#endif // !defined(BOOST_ASIO_HAS_EVENTFD)
//#endif // !defined(__SYMBIAN32__)
//#endif // !defined(__CYGWIN__)
//#endif // !defined(BOOST_ASIO_WINDOWS_RUNTIME)
//#endif // !defined(BOOST_ASIO_WINDOWS)

#endif // BOOST_ASIO_DETAIL_PIPE_SELECT_INTERRUPTER_HPP
