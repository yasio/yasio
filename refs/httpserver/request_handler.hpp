//
// request_handler.hpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HANDLER_HPP
#define HTTP_REQUEST_HANDLER_HPP

#include <string>
#include <unordered_map>
#include "thelib/utils/crypto_wrapper.h"
#include "thelib/utils/cjson_wrapper.h"

namespace http {
namespace server {

struct reply;
struct request;

class connection;

/// The common handler for all incoming requests.
class request_handler
{
public:
    request_handler(const request_handler&)/* = delete*/;
    request_handler& operator=(const request_handler&)/* = delete*/;

    /// Construct with a directory containing files to be served.
    explicit request_handler(const connection& peer);

    /// Handle a request and produce a reply.
    void handle_request(request& req, reply& rep);

public:
    static std::string get_trade_number(void);

public:

    // register new user id
    void handle_reg_new_use_id(const cjsonw::object& request, cjsonw::object& response);

    /// handle bind account
    void handle_account_bind(const cjsonw::object& request, cjsonw::object& response);

    /// handle user login
    void handle_account_login(const cjsonw::object& request, cjsonw::object& response);

    /// handle user logon
    void handle_account_logout(const cjsonw::object& request, cjsonw::object& response);

    /// handle submit achievement
    void handle_submit_achievement(const cjsonw::object& request, cjsonw::object& response);

    /// handle submit challenge result
    void handle_submit_challenge_result(const cjsonw::object& request, cjsonw::object& response);

    /// handle send invite
    void handle_send_invite(const cjsonw::object& request, cjsonw::object& response);

    /// handle answer invite
    void handle_answer_invite(const cjsonw::object& request, cjsonw::object& response);

    /// handle get invite list
    void handle_get_invite_list(const cjsonw::object& request, cjsonw::object& response);

    /// handle get friend list
    void handle_get_friend_list(const cjsonw::object& request, cjsonw::object& response);

    /// handle get mail total count
    void handle_get_mail_counts(const cjsonw::object& request, cjsonw::object& response);

    /// handle get mail list
    void handle_get_mail_list(const cjsonw::object& request, cjsonw::object& response);

    /// handle collect mail
    void handle_collect_mail(const cjsonw::object& request, cjsonw::object& response);

    /// handle give power
    void handle_give_power(const cjsonw::object& request, cjsonw::object& response);

    /// handle get product list
    void handle_get_product_list(const cjsonw::object& request, cjsonw::object& response);

    /// Handle receipt verify receipt
    void handle_ios_receipt_verify(const cjsonw::object& request, cjsonw::object& response);

    /// haldle get announcement
    void handle_get_announcement(const cjsonw::object& request, cjsonw::object& response);

    /// void check server ready
    void handle_check_server_ready(const cjsonw::object& request, cjsonw::object& response);

    /// handle get game info
    void handle_get_game_info(const cjsonw::object& request, cjsonw::object& response);

    /// Update session id
    unsigned int update_session_id(unsigned int user_id);

    /// Check session id
    bool check_session_id(unsigned int user_id, unsigned int session_id);

    void handle_url_notify(const std::string& raw_args, reply& rep);

    /// The directory containing the files to be served.
    // std::string doc_root_;
    const connection& peer_;

    /// Perform URL-decoding on a string. Returns false if the encoding was
    /// invalid.
    static bool url_decode(const std::string& in, std::string& out);
};

} // namespace server
} // namespace http

#endif // HTTP_REQUEST_HANDLER_HPP

