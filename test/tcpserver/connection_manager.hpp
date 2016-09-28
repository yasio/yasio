//
// connection_manager.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c)  2013-2016 halx99
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// Purpose: handle timeout connections

#ifndef HTTP_CONNECTION_MANAGER_HPP
#define HTTP_CONNECTION_MANAGER_HPP

#include <set>
#include "connection.hpp"
#include <mutex>
namespace tcp {
namespace server {

struct connection_wrapper
{
    connection_wrapper(connection_ptr conn) : conn_(conn) {};
    connection_ptr conn_;
};

inline bool operator<(const connection_wrapper& lhs, const connection_wrapper& rhs)
{
    return lhs.conn_->expire_time() < rhs.conn_->expire_time() ||
        (lhs.conn_->expire_time() == rhs.conn_->expire_time() && lhs.conn_ < rhs.conn_);
}

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
    // void do_timeout_check(time_t interval);

    /// Add the specified connection to the manager and start it.
    void start(connection_wrapper cw);

    /// Stop the specified connection.
    void stop(connection_wrapper cw);

    /// Stop the specified connection with locked
    void stop_locked(connection_wrapper cw);

    /// Stop all connections.
    void stop_all();

private:
    /// The managed connections.
    std::mutex mtx_;
    std::set<connection_wrapper> connections_;

    /// timeout checker 
    // boost::asio::deadline_timer timeout_check_timer_;

    /// set rw lock
    // thelib::asy::thread_rwlock rwlock_;

};

} // namespace server
} // namespace http

#endif // HTTP_CONNECTION_MANAGER_HPP

