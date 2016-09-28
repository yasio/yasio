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

enum { send_power_interval = tt(6, h) };

#define HWP_PACKET_HEADER_LENGTH 16
#define _DEFINE_KEYS \
const unsigned char information_key[] = { 0xe8, 0xef, 0xd7, 0x24, 0x20, 0x2d, 0xd3, 0x19, 0x28, 0x02, 0x7c, 0x07, 0xed, 0x69, 0x2a, 0xfa };

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
typedef std::unordered_map<request_type, void (request_handler::*)(request&)> request_handler_table_type;
static request_handler_table_type  local_handler_tab;
namespace {
    struct handler_table_initializer
    {
        handler_table_initializer(void)
        {
            //local_handler_tab.insert(std::make_pair(request_type_reg_new_user_id, &request_handler::handle_vsdata_change));
            //local_handler_tab.insert(std::make_pair(request_type_account_bind, &request_handler::handle_heartbeat));

        }
        ~handler_table_initializer(void)
        {
            local_handler_tab.clear();
        }
    };

    handler_table_initializer initializer;
};


request_handler::request_handler(connection& client)
    : client_(client)
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
#if 0 // 转发服务器, 无需解密包体
    size_t len = 0;
    if (req.packet_.size() > 0 && req.packet_.size() % 16 == 0)
        crypto::aes::decrypt(req.packet_, encrypt_key);

    req.packet_.resize(len);
    auto target = local_handler_tab.find((request_type)req.header_.command_id);
    if (target != local_handler_tab.end())
    {
        (this->*(target->second))(nullptr/*msg*/);


        return true;
    }
#endif

    auto obs = req.header_.encode();
    memcpy(&req.packet_.front(), obs.buffer().data(), obs.buffer().size());

    _DEFINE_KEYS;
    crypto::aes::overlapped::decrypt(req.packet_, information_key, 128/*密钥长度128bits*/, 16/*偏移*/);

    switch (req.header_.command_id)
    {
    case CID_ENTERB_BATTLESCENE_NOTIFY:
        handle_enter_battle(req);
        break;
    case CID_BATTLE_HEARTBEAT_REQ:
        handle_heartbeat(req);
        break;
        /////// 所有转发协议
    case 11021: // 支援
    case CID_SOLDIER_MOVED_NOTIFY:
    case CID_SOLDIER_REMOVED_NOTIFY:
    case CID_TURN_OVER_NOTIFY:
    case CID_VERSUSDATA_CHANGED_NOTIFY:
    case CID_REALTIME_BATTLE_INFO_NOTIFY:
        handle_forward(req);
        break;
    default:
        ; // return false;
    }

    req.reset();

    return true;
}

void request_handler::handle_enter_battle(request& req)
{
    client_.do_timeout_wait();
}

void request_handler::handle_forward(request& req)
{
    _DEFINE_KEYS;
    crypto::aes::encrypt(req.packet_, information_key, 128);

    client_.send_to_peer(req.packet_);
}

void request_handler::handle_heartbeat(request& req)
{
    LOG_WARN_ALL("received heartbeat from:%s, : %.3lf", client_.get_address(), req.header_.reserved2 / 1000.0);
    client_.cancel_timeout_wait();
}

