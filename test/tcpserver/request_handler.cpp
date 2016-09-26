//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
// 处理转发和响应

/// TODO: code can by tidy
#include "connection.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>
#include "request.hpp"
#include "server_logger.hpp"
#include "request_handler.hpp"

static const time_t __1S__ = 1;
static const time_t __1M__ = 60 * __1S__;
static const time_t __1H__ = 60 * __1M__;
static const time_t __1D__ = 24 * __1H__;
#define __1s__ __1S__
#define __1m__ __1M__
#define __1h__ __1H__
#define __1d__ __1D__
#define tt(n,u) ( (n) * __1##u##__ )

static const char* encrypt_key = "ZQnNQmA1iIQ3z3ukoPoATdE88OJ0qsMm";

enum { information_lifetime = 60 };

enum { send_power_interval = tt(6,h) };

enum request_type {
    request_type_reg_new_user_id = 1,
    request_type_account_bind,
    request_type_account_login,
    request_type_account_logout,
    request_type_submit_achievement,
    request_type_submit_challenge_result,
    request_type_send_invite,
    request_type_answer_invite,
    request_type_get_invite_list,
    request_type_get_friend_list,
    request_type_get_mail_counts,
    request_type_get_mail_list,
    request_type_collect_mail,
    request_type_give_power,
    request_type_get_product_list = 20080,
    request_type_verify_ios_receipt,
    request_type_get_game_info,
    request_type_get_announcement,
    request_type_check_server_ready = 60000,
};

enum response_number {
    success = 0,
    username_conflict = 2001,
    username_incorrect,
    password_incorrect,
    session_expired,
    request_expired,
    invited_user_not_exist,
    peer_already_invite_you,
    you_are_already_friends, 
    donot_reinvite_friend, 
    cannot_invite_self,
    invalid_request, 
    invalid_user_id, /* the user_id must be validate except account bind */
    ios_verify_receipt_failed,
    you_are_in_challenging,
    exec_sql_failed,
    unknown_error,
    feature_not_develop,
    illegal_transaction = -2001,
};

using namespace tcp::server;

/// initialize handler table
typedef std::unordered_map<request_type, void (request_handler::*)(message*)> request_handler_table_type;
static request_handler_table_type  local_handler_tab;
namespace {
    struct handler_table_initializer
    {
        handler_table_initializer(void)
        {
            local_handler_tab.insert(std::make_pair(request_type_reg_new_user_id, &request_handler::handle_vsdata_change));
            local_handler_tab.insert(std::make_pair(request_type_account_bind, &request_handler::handle_heartbeat));
            
        }
        ~handler_table_initializer(void)
        {
            local_handler_tab.clear();
        }
    };

    handler_table_initializer initializer;
};


request_handler::request_handler(const connection& peer)
    : peer_(peer)
{
}

bool request_handler::handle_request(request& req) // read file only
{
    // LOG_TRACE_ALL("thread id:%u, sqld2:%#x,sqld:%#x", this_thread(), sqld2, sqld);

    // Decode url to path.
    // std::string request_path;
    /*if (!url_decode(req.uri, request_path))
    {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }*/

    /*if(request_path == "/notify_url.api")
    {
        handle_url_notify(req.content, rep);
        return;
    }*/

    // Fill out the reply to be sent to the client.
 
    //std::string plaintext = "";
    size_t len = 0;
    if(req.content_.size() > 0 && req.content_.size() % 16 == 0)
        crypto::aes::decrypt(req.content_,  encrypt_key);
    
    req.content_.resize(len);
   
    auto target = local_handler_tab.find((request_type)11010/*msg_id*/);
    if(target != local_handler_tab.end())
    {
       ( this->*(target->second) )(nullptr/*msg*/);

       /*std::string& buffer = rep.content;
       buffer.resize(sizeof(uint32_t));
       buffer.append(response.toString());
       char* sizebuf = &buffer.front();
       crypto::aes::pkcs(buffer, buffer.size() - sizeof(uint32_t), AES_BLOCK_SIZE);
       crypto::aes::cbc_encrypt(buffer.c_str() + sizeof(uint32_t),
           buffer.size() - sizeof(uint32_t),
           &buffer.front() + sizeof(uint32_t),
           buffer.size() - sizeof(uint32_t), 
           encrypt_key);
       *( (uint32_t*)(sizebuf) ) = htonl(buffer.size() - sizeof(uint32_t));*/

       return true;
    }

    return false;
}
