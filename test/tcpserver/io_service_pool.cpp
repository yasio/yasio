//
// io_service_pool.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "io_service_pool.hpp"
#include <stdexcept>
#include <functional>
#include <thread>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
// #include <boost/exception/all.hpp>
//#include "thelib/utils/container_helper.h"
//#include "mysql_client.hpp"
#include "server_logger.hpp"
// #include "thelib/utils/cjson.h"

namespace tcp {
namespace server {

io_service_pool::io_service_pool(size_t pool_size)
  : next_io_service_(0), 
    counter_(0)
{
    if (pool_size == 0)
        throw std::runtime_error("io_service_pool size is 0");

    // Give all the io_services work to do so that their run() functions will not
    // exit until they are explicitly stopped.
    for (std::size_t i = 0; i < pool_size; ++i)
    {
        io_service_ptr io_service(new boost::asio::io_service);
        work_ptr work(new boost::asio::io_service::work(*io_service));
        io_services_.push_back(io_service);
        work_.push_back(work);
    }
}

void io_service_pool::start(void)
{
  // Create a pool of threads to run all of the io_services.
  std::vector<std::shared_ptr<std::thread> > threads;
  for (std::size_t i = 0; i < io_services_.size(); ++i)
  {
    std::shared_ptr<std::thread> thread(
        new std::thread( std::bind(&io_service_pool::run, 
        this, 
        std::reference_wrapper<boost::asio::io_service>(*io_services_[i]) )) );

    threads.push_back(thread);
  }

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
    threads[i]->join();
}

void io_service_pool::run(boost::asio::io_service& io_service)
{
    thread_init();

    LOG_WARN_ALL("the thread started, id:%u", thread_self());

    if(++counter_ == io_services_.size())
    {
        LOG_WARN_ALL("the tinyflower server started.");
    }

    try {
        io_service.run();
    }
    catch(const std::exception& e)
    { // TODO: boost::diagnostic_information: need all #include <boost/exception/all.hpp>
        // LOG_ERROR("the thread:%u exit exception, detail:%s, boost::asio say:%s", thread_self(), e.what(), boost::diagnostic_information(e).c_str());
    }

    thread_cleanup();

    LOG_WARN_ALL("the thread stopped, id:%u", thread_self());

    if(--counter_ == 0) 
    {
        LOG_WARN_ALL("%s", "the tinyflower server stopped.");
    }
}

void io_service_pool::stop()
{
  // Explicitly stop all io_services.
  for (std::size_t i = 0; i < io_services_.size(); ++i)
    io_services_[i]->stop();
}

void io_service_pool::thread_init(void)
{
}

void io_service_pool::thread_cleanup(void)
{
}

boost::asio::io_service& io_service_pool::get_io_service()
{
  // Use a round-robin scheme to choose the next io_service to use.
  boost::asio::io_service& io_service = *io_services_[next_io_service_];
  ++next_io_service_;
  if (next_io_service_ == io_services_.size())
    next_io_service_ = 0;
  return io_service;
}

} // namespace server2
} // namespace http
