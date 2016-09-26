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
#include <boost/date_time/local_time/local_time.hpp>  


static const time_t conn_timeout_millisec = 10000;

namespace current {

boost::posix_time::time_duration::tick_type milliseconds_since_epoch()
{
    using boost::gregorian::date;
    using boost::posix_time::ptime;
    using boost::posix_time::microsec_clock;

    static ptime const epoch(date(1970, 1, 1));
    return (microsec_clock::universal_time() - epoch).total_milliseconds();
}

}

namespace http {
namespace server {

connection_manager::connection_manager(boost::asio::io_service& io_service) : timeout_check_timer_(io_service)
{
    /*
    // insert(sets.size() + 1, socket&timestamp)
    */
}

void connection_manager::start(connection_wrapper cw)
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);

    if(connections_.empty()) {
        do_timeout_check(conn_timeout_millisec);
    }

    cw.conn_->expire_time(current::milliseconds_since_epoch() + conn_timeout_millisec); 

    connections_.insert(cw);

    cw.conn_->start();
}

void connection_manager::stop(connection_wrapper cw)
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);
    stop_locked(cw);
}

void connection_manager::stop_locked(connection_wrapper cw)
{
    auto target = connections_.find(cw);
    if(target != connections_.end()) {
        connections_.erase(cw);
        cw.conn_->stop();
    }
}

void connection_manager::stop_all()
{
    thelib::asy::scoped_wlock<thelib::asy::thread_rwlock> guard(rwlock_);

    for (auto cw : connections_)
        cw.conn_->stop();
    connections_.clear();
}

void connection_manager::do_timeout_check(time_t interval)
{
    this->timeout_check_timer_.expires_from_now(boost::posix_time::milliseconds(interval));

    this->timeout_check_timer_.async_wait([this](const boost::system::error_code& ec) {
        if(!ec) {
            LOG_TRACE_ALL("check timout, thread id:%u\n", this_thread() );
            
            thelib::asy::scoped_rlock<thelib::asy::thread_rwlock> guard(rwlock_);

            if(!this->connections_.empty()) {
                auto cw = this->connections_.begin();
                auto delta = cw->conn_->expire_time() - current::milliseconds_since_epoch();
                if(delta <= 0) { // the connection already timeout
                    cw->conn_->stop(); // per thread per io_service, so no need lock
                    delta = 500;
                    if(this->connections_.size() > 1) {
                        this->do_timeout_check(delta);
                    }
                }
                else {
                    if(!this->connections_.empty()) {
                        this->do_timeout_check(delta);
                    }
                }  
            }
        }
    } );
}

} // namespace server
} // namespace http

