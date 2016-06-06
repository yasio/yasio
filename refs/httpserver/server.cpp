//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <signal.h>
#include <utility>
#include <thread>
#include <boost/exception/all.hpp>
#include "thelib/utils/container_helper.h"
#include "mysql_client.hpp"
#include "server_logger.hpp"
#include "thelib/utils/object_pool_with_mutex.h"

const char* ios_verify_url = "https://sandbox.itunes.apple.com/verifyReceipt";


typedef object_pool<http::server::connection, SZ(2,M) / sizeof(http::server::connection)> connection_pool_t;
connection_pool_t connection_pool;

namespace http {
namespace server {

static void deleter (connection* ptr) 
{  
    connection_pool.release(ptr);
} 

server::server(const std::string& address, const std::string& port,
    const std::string& doc_root)
  : io_service_(),
    signals_(io_service_),
    acceptor_(io_service_),
    connection_manager_(io_service_),
    socket_(io_service_),
    counter_(0),
    thread_count_(0)/*,
    request_handler_(*this)*/
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

  // check is sandbox
  cjsonw::document config("config.json");
  bool is_sandbox = config.getRoot().getBool("is_sandbox", true);
  LOG_TRACE_ALL("server config --> is_sandbox=%s", is_sandbox ? "true" : "false"); 
  if(!is_sandbox) {
      ios_verify_url = "https://buy.itunes.apple.com/verifyReceipt";
  }

  do_await_stop();

  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
  boost::asio::ip::tcp::resolver resolver(io_service_);
  boost::asio::ip::tcp::resolver::query query(address, port);
  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
  acceptor_.open(endpoint.protocol());

  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  do_accept();
}

server::~server(void)
{
    mysql_server_end();
}

void server::start(int thread_count)
{
    // start all work threads
    thread_count_ = thread_count;
    LOG_WARN_ALL("%s", "starting tinyflower server...");

    for(int i = 0; i < thread_count; ++i)
    {
        work_threads_.push_back( std::shared_ptr<std::thread>(new std::thread( std::bind(&server::run, this) ) ) );
    }

    // join all works
    std::for_each(work_threads_.begin(), work_threads_.end(), [](std::shared_ptr<std::thread>& the_thread){
        the_thread->join();
    });
}

void server::run()
{
    // The io_service::run() call will block until all asynchronous operations
    // have finished. While the server is running, there is always at least one
    // asynchronous operation outstanding: the asynchronous accept call waiting
    // for new incoming connections.
    thread_init();

    LOG_WARN_ALL("the thread started, id:%u", this_thread());

    if(++counter_ == thread_count_)
    {
        LOG_WARN_ALL("the tinyflower server started.");
    }

    try {
        io_service_.run();
    }
    catch(const std::exception& e)
    {
        LOG_ERROR("the thread:%u exit exception, detail:%s, boost::asio say:%s", this_thread(), e.what(), boost::diagnostic_information(e).c_str());
    }

    thread_cleanup();

    LOG_WARN_ALL("the thread stopped, id:%u", this_thread());

    if(--counter_ == 0) 
    {
        LOG_WARN_ALL("%s", "the tinyflower server stopped.");
    }
}

void server::do_accept()
{
    acceptor_.async_accept(socket_,
        [this](boost::system::error_code ec) {
        // Check whether the server was stopped by a signal before this
        // completion handler had a chance to run.
        if (!acceptor_.is_open())
        {
            return;
        }

        if (!ec)
        {
            connection_manager_.start(
                std::shared_ptr<connection>(
                new(connection_pool.get()) connection(std::move(socket_), connection_manager_), 
                deleter ) );
        }

        do_accept();
    });
}

void server::do_await_stop()
{
    signals_.async_wait(
        [this](boost::system::error_code /*ec*/, int /*signo*/) {
        // The server is stopped by cancelling all outstanding asynchronous
        // operations. Once all operations have finished the io_service::run()
        // call will exit.

        acceptor_.close();
        connection_manager_.stop_all();
    });
}

void server::thread_init(void)
{
    cJSON_thread_init();
    mysql_client::thread_init();
}

void server::thread_cleanup(void)
{
    mysql_client::thread_end();
    cJSON_thread_end();
}

} // namespace server
} // namespace http

