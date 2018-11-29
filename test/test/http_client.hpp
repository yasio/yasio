#ifndef _HTTP_CLIENT_HPP_
#define _HTTP_CLIENT_HPP_
#include <string>

extern std::string send_http_req(const std::string& uri, const std::string& method = "POST", const std::string& content = "");

#endif

