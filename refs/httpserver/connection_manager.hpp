//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_CONNECTION_MANAGER_HPP
#define HTTP_CONNECTION_MANAGER_HPP

#include <set>
#include "connection.hpp"
#include "thelib/utils/thread_synch.h"

namespace http {
namespace server {

/// Manages open connections so that they may be cleanly stopped when the server
/// needs to shut down.
class connection_manager
{
public:
  connection_manager(const connection_manager&)/* = delete*/;
  connection_manager& operator=(const connection_manager&)/* = delete*/;

  /// Construct a connection manager.
  connection_manager(boost::asio::io_service&);

  /// Do session time check
  void do_timeout_check();

  /// Add the specified connection to the manager and start it.
  void start(connection_ptr c);

  /// Stop the specified connection.
  void stop(connection_ptr c);

  /// Stop the specified connection with locked
  void stop_locked(connection_ptr c);

  /// Stop all connections.
  void stop_all();

private:
  /// The managed connections.
  std::set<connection_ptr> connections_;

  /// timeout checker 
  boost::asio::deadline_timer timeout_check_timer_;

  /// set rw lock
  thelib::asy::thread_rwlock rwlock_;

};

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_MANAGER_HPP

