//
// io_service_pool.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c)  2013-2016 halx99
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_IO_SERVICE_POOL_HPP
#define HTTP_SERVER2_IO_SERVICE_POOL_HPP

#include <boost/asio.hpp>
#include <vector>
#include <atomic>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

namespace tcp {
namespace server {

/// A pool of io_service objects.
class io_service_pool
  : private boost::noncopyable
{
public:
  /// Construct the io_service pool.
  explicit io_service_pool(size_t pool_size);

  void start(void);

  /// Run all io_service objects in the pool.
  void run(boost::asio::io_service& service);

  /// Stop all io_service objects in the pool.
  void stop();

  static void thread_init();

  static void thread_cleanup();

  /// Get an io_service to use.
  boost::asio::io_service& get_io_service();

private:
  typedef std::shared_ptr<boost::asio::io_service> io_service_ptr;
  typedef std::shared_ptr<boost::asio::io_service::work> work_ptr;

  /// The pool of io_services.
  std::vector<io_service_ptr> io_services_;

  /// The work that keeps the io_services running.
  std::vector<work_ptr> work_;

  /// The next io_service to use for a connection.
  std::size_t next_io_service_;

  std::atomic<size_t> counter_;
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_IO_SERVICE_POOL_HPP
