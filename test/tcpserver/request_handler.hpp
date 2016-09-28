//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c)  2013-2016 halx99
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <unordered_map>
#include "utils/crypto_wrapper.h"
// #include "utils/cjson_wrapper.h"

namespace tcp {
namespace server {

struct reply;
struct request;

class connection;
class message;

/// The common handler for all incoming requests.
class request_handler
{
public:
    request_handler(const request_handler&)/* = delete*/;
    request_handler& operator=(const request_handler&)/* = delete*/;

    /// Construct with a directory containing files to be served.
    explicit request_handler(connection& peer);

    /// Handle a request and produce a reply.
    bool handle_request(request& req);

public:
    static std::string get_trade_number(void);

public:

    // 进入战场, 启动心跳检查
    void handle_enter_battle(request& req);

    // 战斗数据改变
    void handle_vsdata_change(request& req);

    /// 战斗心跳
    void handle_heartbeat(request& req);

    // 切换回合
    void handle_turn_over_notify(request& req);

    /// The directory containing the files to be served.
    // std::string doc_root_;
    connection& client_;
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP

