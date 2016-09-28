//
// connection_manager.cpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// #include "utils/container_helper.h"
#include <thread>
#include "connection_manager.hpp"
#include "server_logger.hpp"
// #include <boost/date_time/local_time/local_time.hpp>  


namespace tcp {
namespace server {

connection_manager::connection_manager(boost::asio::io_service& io_service)/* : timeout_check_timer_(io_service)*/
{
    /*
    // insert(sets.size() + 1, socket&timestamp)
    */
}

void connection_manager::start(connection_wrapper cw)
{
    mtx_.lock();
    if (!connections_.empty()) { // 绑定已有连接关系
        auto exist = connections_.begin();
        exist->conn_->peer_ = cw.conn_;
        cw.conn_->peer_ = exist->conn_;
    }
    connections_.insert(cw);
    mtx_.unlock();

    if (connections_.size() >= 2)
    {
        LOG_WARN_ALL("total connection size:%d", connections_.size());
    }

    cw.conn_->start();
}

void connection_manager::stop(connection_wrapper cw)
{
    std::lock_guard<std::mutex> guard(mtx_);
    stop_locked(cw);
}

void connection_manager::stop_locked(connection_wrapper cw)
{
    auto target = connections_.find(cw);
    if(target != connections_.end()) {
        connections_.erase(cw);
    }
}

void connection_manager::stop_all()
{
    /*for (auto cw : connections_)
        cw.conn_->stop();*/

    std::lock_guard<std::mutex> guard(mtx_);
    connections_.clear();
}

} // namespace server
} // namespace http

