//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2013-2016 halx99
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "server.hpp"
#include <signal.h>
#include <utility>
#include <thread>
//#include <boost/exception/all.hpp>
// #include "utils/container_helper.h"
// #include "mysql_client.hpp"
#include "server_logger.hpp"
#include "utils/object_pool.h"
#include "tls.hpp"

namespace tcp {
namespace server {

static void deleter (connection* ptr) 
{  
    delete ptr;
    // gls_release_connection(ptr);
}

server::server(const std::string& address, const std::string& port,
      size_t io_service_pool_size)
  : io_service_pool_(io_service_pool_size),
    io_service_(),
    signals_(io_service_pool_.get_io_service()),
    acceptor_(io_service_pool_.get_io_service()),
    connection_manager_(io_service_pool_.get_io_service()),
    socket_(io_service_pool_.get_io_service()),
    counter_(0)
{
  // Register to handle the signals that indicate when the server should exit.
  // It is safe to register for the same signal multiple times in a program,
  // provided all registration for the specified signal is made through Asio.
  signals_.add(SIGINT);
  signals_.add(SIGTERM);
#if defined(SIGQUIT)
  signals_.add(SIGQUIT);
#endif // defined(SIGQUIT)

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
    //mysql_server_end();
}

void server::start(void)
{
    // start all work threads
    io_service_pool_.start();
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
                new/*(gls_get_connection())*/ connection(std::move(socket_), connection_manager_), 
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

} // namespace server
} // namespace http

