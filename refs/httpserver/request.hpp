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
#include "header.hpp"

namespace http {
namespace server {

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  size_t               header_length; //  header length equal to content offset
  std::vector<header>  headers;
  size_t               content_length; // content size in bytes
  std::string          content;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
