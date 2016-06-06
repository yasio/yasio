//
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "thelib/utils/container_helper.h"
#include <thread>
#include "connection_manager.hpp"
#include "server_logger.hpp"

namespace http {
namespace server {

connection_manager::connection_manager(boost::asio::io_service& io_service) : timeout_check_timer_(io_service)
{
}

void connection_manager::start(connection_ptr c)
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);

    if(connections_.empty()) {
        // do_timeout_check();
    }
    connections_.insert(c);
    c->start();
}

void connection_manager::stop(connection_ptr c)
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);
    stop_locked(c);
}


void connection_manager::stop_locked(connection_ptr c)
{
    auto target = connections_.find(c);
    if(target != connections_.end()) {
        connections_.erase(c);
        c->stop();
    }
}

void connection_manager::stop_all()
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);

    for (auto c: connections_)
        c->stop();
    connections_.clear();
}

void connection_manager::do_timeout_check()
{
    this->timeout_check_timer_.expires_from_now(boost::posix_time::seconds(5));

    this->timeout_check_timer_.async_wait([this](const boost::system::error_code& ec) {
        if(!ec) {
            LOG_TRACE_ALL("check timout, thread id:%u\n", this_thread() );
            
            size_t timeoc = 0;

            thelib::asy::scoped_rlock<thelib::asy::thread_rwlock> guard(rwlock_);

            for(auto c:connections_)
            {
                if(c->is_timeout()) {
                    ++timeoc;
                    LOG_TRACE_ALL("A connection is timeout, detail:%s", c->get_address());
                    c->get_socket().cancel(); /*shutdown(boost::asio::ip::tcp::socket::shutdown_both);*/
                }
            }
            
            if(connections_.size() > timeoc) 
                do_timeout_check();
        }
    } );
}

} // namespace server
} // namespace http

