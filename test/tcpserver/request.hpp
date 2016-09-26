//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>

namespace http {
namespace server {

/// A request received from a client.
struct request
{
  size_t               expecting_size_;
  std::string          content_;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
