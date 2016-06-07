//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

/// TODO: code can by tidy
#include "connection.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <unordered_map>

#include "http_client.hpp"
#include "mime_types.hpp"
#include "reply.hpp"
#include "request.hpp"

#include "mysql_client.hpp"
#include "server_logger.hpp"

#include "errmsg.h"
#include "mysqld_error.h"

#include "thelib/utils/random.h"
#include "thelib/utils/aes_wrapper.h"
#include "thelib/utils/nsconv.h"

#include "request_handler.hpp"

extern const char* ios_verify_url;

#define sqld ( sqld2 )

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

enum mail_catagory
{
    mail_catagory_system,
    mail_catagory_friend,
    mail_catagory_challenge
};

enum mail_type {
    mail_type_send_power = 1,
    mail_type_send_diamond,
    mail_type_send_gold,
    mail_type_send_props,
    mail_type_send_challenge_request,
    mail_type_send_challenge_result,
    mail_type_send_accept_invite
};

enum challenge_type{
    challenge_type_request = 1,
    challenge_type_respond,
};

/// check session and time macro
#define CHECK_SESSION_RETURN(req,resp) \
    cjsonw::object params = req["params"]; \
    if(params.getInt("user_id", 0) == 0) { resp["ret"] = (int)invalid_user_id; return; } \
    if(!check_session_id(params.getInt("user_id", 0), params.getInt("session_id", 0)) ) \
{ \
    resp["ret"] = (int)session_expired; \
    return; \
    } \
    if( time(NULL) - params.getInt("time") > information_lifetime/* check time */) \
{ \
    resp["ret"] = (int)request_expired; \
    return; \
    }

#define CHECK_EXEC_SQL_RETURN_I(sqlstmt,resp) \
    if(!sqlstmt) \
{ \
    resp["ret"] = (int)exec_sql_failed; \
    return; \
    }

#define CHECK_EXEC_SQL_RETURN_WR(sqlstmt,resp) \
    if(!sqlstmt) \
{ \
    resp["ret"] = (int)exec_sql_failed; \
    return; \
    }

#define CHECK_EXEC_SQL_RETURN_RD(sqlstmt,resp) \
    if(!sqlstmt.validated()) \
{ \
    resp["ret"] = (int)exec_sql_failed; \
    return; \
    }

using namespace http::server;

/// initialize handler table
typedef std::map<request_type, void (request_handler::*)(const cjsonw::object& request, cjsonw::object& response)> request_handler_table_type;
static request_handler_table_type  local_handler_tab;
namespace {
    struct handler_table_initializer
    {
        handler_table_initializer(void)
        {
            local_handler_tab.insert(std::make_pair(request_type_reg_new_user_id, &request_handler::handle_reg_new_use_id));
            local_handler_tab.insert(std::make_pair(request_type_account_bind, &request_handler::handle_account_bind));
            local_handler_tab.insert(std::make_pair(request_type_account_login, &request_handler::handle_account_login));
            local_handler_tab.insert(std::make_pair(request_type_account_logout, &request_handler::handle_account_logout));
            local_handler_tab.insert(std::make_pair(request_type_answer_invite, &request_handler::handle_answer_invite));
            local_handler_tab.insert(std::make_pair(request_type_check_server_ready, &request_handler::handle_check_server_ready));
            local_handler_tab.insert(std::make_pair(request_type_collect_mail, &request_handler::handle_collect_mail));
            local_handler_tab.insert(std::make_pair(request_type_get_announcement, &request_handler::handle_get_announcement));
            local_handler_tab.insert(std::make_pair(request_type_get_friend_list, &request_handler::handle_get_friend_list));
            local_handler_tab.insert(std::make_pair(request_type_get_invite_list, &request_handler::handle_get_invite_list));
            local_handler_tab.insert(std::make_pair(request_type_get_mail_counts, &request_handler::handle_get_mail_counts));
            local_handler_tab.insert(std::make_pair(request_type_get_mail_list, &request_handler::handle_get_mail_list));
            local_handler_tab.insert(std::make_pair(request_type_get_product_list, &request_handler::handle_get_product_list));
            local_handler_tab.insert(std::make_pair(request_type_give_power, &request_handler::handle_give_power));
            local_handler_tab.insert(std::make_pair(request_type_verify_ios_receipt, &request_handler::handle_ios_receipt_verify));
            local_handler_tab.insert(std::make_pair(request_type_send_invite, &request_handler::handle_send_invite));
            local_handler_tab.insert(std::make_pair(request_type_submit_achievement, &request_handler::handle_submit_achievement));
            local_handler_tab.insert(std::make_pair(request_type_submit_challenge_result, &request_handler::handle_submit_challenge_result));
            local_handler_tab.insert(std::make_pair(request_type_get_game_info, &request_handler::handle_get_game_info));
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

void request_handler::handle_request(request& req, reply& rep) // read file only
{
    // LOG_TRACE_ALL("thread id:%u, sqld2:%#x,sqld:%#x", this_thread(), sqld2, sqld);

    // Decode url to path.
    std::string request_path;
    if (!url_decode(req.uri, request_path))
    {
        rep = reply::stock_reply(reply::bad_request);
        return;
    }

    if(request_path == "/notify_url.api")
    {
        handle_url_notify(req.content, rep);
        return;
    }

    // Fill out the reply to be sent to the client.
    rep.status = reply::ok;
    cjsonw::object request;
    //std::string plaintext = "";
    size_t len = 0;
    if(req.content.size() > 0 && req.content.size() % 16 == 0)
        aes_cbc_decrypt(req.content.c_str(), req.content.size(), (char*)req.content.c_str(), len, encrypt_key);
        // plaintext = crypto::aes::decrypt_v2( unmanaged_string(req.content.c_str(), req.content.size()) );
    
    req.content.resize(len);
    request.parseString(req.content.c_str());

    cjsonw::object response;
    auto target = local_handler_tab.find((request_type)request.getInt("request_type"));
    if(target != local_handler_tab.end())
    {
       ( this->*(target->second) )(request, response);
    }
    else {
        rep.content = "{ \"ret\":-2,\"params\":{\"message\":\"request illegal!\"}";
        return;
    }

    rep.content = response.toString();
    crypto::aes::pkcs(rep.content);

    aes_cbc_encrypt(rep.content.c_str(), rep.content.size(), (char*)rep.content.c_str(), rep.content.size(), encrypt_key);

    // rep.content = "ok";// send_http_request_sync("https://sandbox.itunes.apple.com/verifyReceipt", "POST", req.content);
    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    thelib::nsc::to_xstring(rep.content.size(), rep.headers[0].value);
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "application/json";
}

void request_handler::handle_reg_new_use_id(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    time_t t = time(NULL);
    CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user values(0,'','',%d,'%s',%d,'%s',0,1,1,0,0);", 
        (int)t, peer_.get_address(), 
        (int)t, peer_.get_address()), resp);

    int user_id = sqld->get_insert_id();

    resp["ret"] = 0;
    resp["params"]["user_id"] = user_id;
}

void request_handler::handle_account_bind(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    unsigned int user_id = req["params"].getInt("user_id");
    
    /// check whether user name exist firstly
    auto name_exists = sqld->exec_query1("select 1 from user where username='%s' limit 1;", req["params"].getString("username"));
       
    CHECK_EXEC_SQL_RETURN_RD(name_exists, resp);

    if(name_exists.has_one()) {
        resp["ret"] = (int)username_conflict;
        return;
    }
     
    if(user_id == 0) { // if a new user, register a new user id firstly
        /// check whether user name exist firstly
        time_t t = time(NULL);
        CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user values(0,'','',%d,'%s',%d,'%s',0, 1, 1, 0, 0);", (int)t, peer_.get_address(), (int)t, peer_.get_address()), resp);
        user_id = (unsigned int)sqld->get_insert_id();
    }

   // unregistered, execute register
    CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user set username='%s',password='%s' where user_id=%u",
        req["params"].getString("username"),
        crypto::hash::md5(req["params"].getString("password")).c_str(),
        user_id), resp);
      
    int new_session_id = update_session_id(user_id);
    CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user set session_id=%u where user_id=%u", new_session_id, user_id), resp);

    resp["ret"] = (int)success;
    resp["params"]["user_id"] = (int)user_id;
    resp["params"]["session_id"] = (int)new_session_id;
}

/// handle user login
void request_handler::handle_account_login(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    auto params = req["params"];

    auto results = sqld->exec_query1("select user_id,password,highest_score,furthest_distance from user where username='%s'", params.getString("username"));

    CHECK_EXEC_SQL_RETURN_RD(results, resp);

    if(results.has_one())
    {
        char** row = results.fetch_row();
        int user_id = atoi(row[0]);
        char* password = row[1];

        if(strcmp(password, crypto::hash::md5(params.getString("password")).c_str()) == 0)
        {
            int new_session_id = update_session_id(user_id);
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user set session_id=%u,latest_time=%d,latest_ip='%s' where user_id=%u", 
                new_session_id, time(NULL), peer_.get_address(), user_id), resp);
            
            resp["ret"] = (int)success;
            resp["params"]["user_id"] = user_id;
            resp["params"]["highest_score"] = atoi(row[2]);
            resp["params"]["furthest_distance"] = atoi(row[3]);
            resp["params"]["session_id"] = new_session_id;
        }
        else {
            resp["ret"] = (int)password_incorrect;
        }
    }
    else {
        resp["ret"] = (int)username_incorrect;
    }
}

/// handle user logon
void request_handler::handle_account_logout(const cjsonw::object& req, cjsonw::object& resp)
{ // do nothing
    resp["ret"] = (int)feature_not_develop;
}

/// handle submit achievement
void request_handler::handle_submit_achievement(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req,resp);

    int user_id = params.getInt("user_id");

    auto achievements = sqld->exec_query1("select highest_score,furthest_distance from user where user_id=%u", user_id);
    CHECK_EXEC_SQL_RETURN_RD(achievements, resp);

    if(!achievements.has_one())
    {
        resp["ret"] = unknown_error;
        return;
    }

    char** row = achievements.fetch_row();
    int highest_score = atoi(row[0]);
    int furthest_distance = atoi(row[1]);
    int new_highest_score =  params.getInt("highest_score");
    int new_furthest_distance = params.getInt("furthest_distance");

    if(highest_score < new_highest_score) {
        highest_score = new_highest_score;
        resp["params"]["highest_score"] = highest_score;
    }

    if(furthest_distance < new_furthest_distance) {
        furthest_distance = new_furthest_distance;
        resp["params"]["furthest_distance"] = furthest_distance;
    }

    CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user set role_id=%d,role_level=%d,highest_score=%d,furthest_distance=%d where user_id=%d",
        params.getInt("role_id"),
        params.getInt("role_level"),
        highest_score,
        furthest_distance,
        params.getInt("user_id")), resp);

    resp["ret"] = (int)success;
}

/// handle submit challenge result
void request_handler::handle_submit_challenge_result(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    if(params.getInt("usre_id") == params.getInt("other_id"))
    { // just ignore
        resp["ret"] = 0;
        return;
    }
 
    /// user_id,mail_category,mail_type,mail_sender_id,value1,value2
    /// notify challenge request to challenged mailbox
    
    switch(params.getInt("challenge_type"))
    {
    case challenge_type_request: // user_id is challenger's Id, other_id is challenged's Id.
        {
            // check is challenging
            auto challenging = sqld->exec_query1("select 1 from user_mails where mail_type=%d and mail_receiver_id=%u and mail_sender_id=%u limit 1;", mail_type_send_challenge_request, params.getInt("other_id"), params.getInt("user_id"));
            CHECK_EXEC_SQL_RETURN_RD(challenging, resp);
            
            if(challenging.has_one())
            {
                resp["ret"] = (int)you_are_in_challenging;
                return;
            }

            // params.getInt("user_id") is challenger
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user_mails values(0,%d,%d,%d,%d,%d,0,%d)",
                mail_catagory_challenge,
                mail_type_send_challenge_request,
                params.getInt("other_id"),
                params.getInt("user_id"),
                params.getInt("score"),
                time(NULL) ), resp);

            //// update field 'is_challenging' to 'true' of challenger's friend list
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user_friends set is_challenging=1 where user_id=%d and friend_id=%d",
                params.getInt("user_id"),
                params.getInt("other_id")), resp);

            resp["ret"] = (int)success;
        }

        break;

    case challenge_type_respond:
        { // params.getInt("user_id") is challenged's Id, other_id is challenger's Id.

            auto result = sqld->exec_query1("select value1 from user_mails where mail_type=%d and mail_receiver_id=%d and mail_sender_id=%d limit 1;",
                mail_type_send_challenge_request,
                params.getInt("user_id"),
                params.getInt("other_id")
                );

            CHECK_EXEC_SQL_RETURN_RD(result, resp);

            // int sender_id = 0;
            int challenger_score = 0;
            if(result.has_one()) { // delete challenged's mail
                char** row = result.fetch_row();
                challenger_score = atoi(row[0]);
                CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("delete from user_mails where mail_type=%d and mail_receiver_id=%d and mail_sender_id=%d", 
                    mail_type_send_challenge_request,
                    params.getInt("user_id"), 
                    params.getInt("other_id")), resp);
            }
            else { // no such challenge record
                resp["ret"] = (int)unknown_error;
                return;
            }

            // notify challenge result to challenger's mailbox
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user_mails (mail_id,mail_category,mail_type,mail_receiver_id,mail_sender_id,value1,value2,timestamp) values(0,%d,%d,%d,%d,%d,%d,%d)",
                mail_catagory_challenge,
                mail_type_send_challenge_result,
                params.getInt("other_id"), // become receiver
                params.getInt("user_id"),
                challenger_score,
                params.getInt("score"),
                time(NULL)), resp);

            //  update field 'is_challenging' of challenger's friend list 
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user_friends set is_challenging=0 where user_id=%d and friend_id=%d", params.getInt("other_id"), params.getInt("user_id") ), resp);

            resp["ret"] = (int)success;
        }
        break;
    default:
        resp["ret"] = invalid_request;
        break;
    }
}

/// handle send invite
void request_handler::handle_send_invite(const cjsonw::object& req, cjsonw::object& resp)
{ // client need comfirm don't invite self
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    if(params.getInt("user_id") == params.getInt("invited_id"))
    {
        resp["ret"] = (int)cannot_invite_self;
        return;
    }

    /// check invited user id validate
    auto user_exist = sqld->exec_query1("select 1 from user where user_id=%d limit 1;",
        params.getInt("invited_id"));
    CHECK_EXEC_SQL_RETURN_RD(user_exist, resp);
    if(!user_exist.has_one()) {
        resp["ret"] = (int)invited_user_not_exist;
        return;
    }

    /// check is friend exist
    auto friend_exist = sqld->exec_query1("select 1 from user_friends where user_id=%d and friend_id=%d limit 1;", params.getInt("user_id"), params.getInt("invited_id"));
    CHECK_EXEC_SQL_RETURN_RD(friend_exist, resp);
    if(friend_exist.has_one()) {
        resp["ret"] = (int)you_are_already_friends;
        return;
    }

    /// check whether peer invited
    auto peer_invite_you = sqld->exec_query1("select 1 from user_invites where invited_id=%d and inviter_id=%d limit 1;", params.getInt("user_id"), params.getInt("invited_id"));
    CHECK_EXEC_SQL_RETURN_RD(peer_invite_you, resp);
    if(peer_invite_you.has_one())
    {
        //// delete peer invite firstly
        //CHECK_EXEC_SQL_RETURN_WR(
        //    sqld->exec_query0("delete from user_invites where invited_id=%d and inviter_id=%d",
        //    params.getInt("user_id"),
        //    params.getInt("invited_id")), 
        //    resp);

        ///// add to friend list of each other.
        //CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user_friends (user_id,friend_id,send_power_time,is_challenging) values(%d,%d,0,0);",
        //    params.getInt("user_id"),
        //    params.getInt("invited_id")), resp);

        //CHECK_EXEC_SQL_RETURN_WR( sqld->exec_query0("insert into user_friends (user_id,friend_id,send_power_time,is_challenging) values(%d,%d,0,0);",
        //    params.getInt("invited_id"),
        //    params.getInt("user_id")), resp);

        resp["ret"] = (int)peer_already_invite_you;
        return;
    }

    /// user_id,inviter_id
    bool succeed = sqld->exec_query0("insert into user_invites (invited_id,inviter_id) values(%d,%d);",
        params.getInt("invited_id"),
        params.getInt("user_id"));
    
    if(sqld->get_last_errno() == ER_DUP_ENTRY) // primary key conflicit
    {
        resp["ret"] = (int)donot_reinvite_friend;
        return;
    }

    CHECK_EXEC_SQL_RETURN_WR(succeed, resp);

    resp["ret"] = (int)success; // send invite success
}

/// handle answer invite
void request_handler::handle_answer_invite(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    // delete invite firstly
    CHECK_EXEC_SQL_RETURN_WR(
        sqld->exec_query0("delete from user_invites where invited_id=%d and inviter_id=%d",
        params.getInt("user_id"),
        params.getInt("inviter_id")), 
        resp);

    bool accepted = params.getBool("accepted");
    if(accepted) 
    { // accepted, add inviter and invited to each other's friend list
        bool succeed = sqld->exec_query0("insert into user_friends (user_id,friend_id,send_power_time,is_challenging) values(%d,%d,0,0);",
            params.getInt("user_id"),
            params.getInt("inviter_id"));

        CHECK_EXEC_SQL_RETURN_WR(succeed, resp);

        succeed = sqld->exec_query0("insert into user_friends (user_id,friend_id,send_power_time,is_challenging) values(%d,%d,0,0);",
            params.getInt("inviter_id"),
            params.getInt("user_id"));

        CHECK_EXEC_SQL_RETURN_WR(succeed, resp);

    }
    
    // notify challenge result to challenger's mailbox
    CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user_mails (mail_id,mail_category,mail_type,mail_receiver_id,mail_sender_id,value1,value2,timestamp) values(0,%d,%d,%d,%d,%d,0,%d)",
        mail_catagory_friend,
        mail_type_send_accept_invite,
        params.getInt("inviter_id"),
	    params.getInt("user_id"),
        accepted,
        time(NULL)), resp);

    resp["ret"] = (int)success;
}

/// handle get invite list
void request_handler::handle_get_invite_list(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    /// query all users who invite current user
    auto results = sqld->exec_query1("SELECT user.user_id,user.username,user.role_id,user.role_level,user.highest_score,user.furthest_distance FROM user,user_invites WHERE user_invites.invited_id=%d and user.user_id=user_invites.inviter_id limit 20;", params.getInt("user_id"));

    CHECK_EXEC_SQL_RETURN_RD(results, resp);

    auto count = results.count_of_rows();
    auto user_list = resp["params"]["user_list"];
    char** row = results.fetch_row();
    for(auto i = 0; i < count; ++i)
    {
        cjsonw::object user_info;
        user_info["user_id"] = atoi(row[0]);
        user_info["username"] = row[1];
        user_info["role_id"] = atoi(row[2]);
        user_info["role_level"] = atoi(row[3]);
        user_info["highest_score"] = atoi(row[4]);
        user_info["furthest_distance"] = atoi(row[5]);
        user_list.append(user_info);
        row = results.fetch_row();
    }

    resp["ret"] = (int)success;
}

/// handle get friend list
void request_handler::handle_get_friend_list(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    auto friendIds = sqld->exec_query1("select friend_id,send_power_time,is_challenging from user_friends where user_id=%d limit 50;", params.getInt("user_id"));

    CHECK_EXEC_SQL_RETURN_RD(friendIds, resp);

    /// construct identifiers second query
    auto count = friendIds.count_of_rows();
    auto row = friendIds.fetch_row();
    
    std::map<int, std::pair<bool,bool> > table;
    std::stringstream ss;
    ss << "(";

    std::pair<bool, bool> flags; // first: is_allow_send_power second: is_challenging
    for(auto i = 0; i < count; ++i)
    {
        ss << row[0] << ",";
        flags.first = ( ( time(NULL) / tt(1,d) ) > atoi(row[1]) / tt(1,d) );
        flags.second = atoi(row[2]);
        table.insert(std::make_pair(atoi(row[0]), flags));
        row = friendIds.fetch_row();
    }

    ss << params.getInt("user_id") << ")";

    auto results = sqld->exec_query1("select user_id,username,role_id,role_level,highest_score,furthest_distance from user where user_id in %s;",
        ss.str().c_str());

    CHECK_EXEC_SQL_RETURN_RD(results,resp);

    resp["ret"] = (int)success;

    count = results.count_of_rows();
    auto friends = resp["params"]["friends"];
    row = results.fetch_row();
    for(auto i = 0; i < count; ++i)
    {
        cjsonw::object f;
        f["user_id"] = atoi(row[0]);
        f["username"] = row[1];
        f["role_id"] = atoi(row[2]);
        f["role_level"] = atoi(row[3]);
        f["highest_score"] = atoi(row[4]);
        f["furthest_distance"] = atoi(row[5]);

        auto target = table.find(atoi(row[0]));
        if(target != table.end()) {
            f["is_allow_send_power"] = target->second.first;
            f["is_challenging"] = target->second.second;
        }
        friends.append(f);

        row = results.fetch_row(); // fetch next row
    }
}

/// handle get mail total count
void request_handler::handle_get_mail_counts(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    auto friends = sqld->exec_query1("select count(*) from user_mails where mail_receiver_id=%d and mail_category=%d limit 30", 
        params.getInt("user_id"), mail_catagory_friend
        );
    CHECK_EXEC_SQL_RETURN_RD(friends, resp);

    auto systems = sqld->exec_query1("select count(*) from user_mails where mail_receiver_id=%d and mail_category=%d limit 30", 
        params.getInt("user_id"), mail_catagory_system
        );
    CHECK_EXEC_SQL_RETURN_RD(systems, resp);

    auto challenges = sqld->exec_query1("select count(*) from user_mails where mail_receiver_id=%d and mail_category=%d limit 30", 
        params.getInt("user_id"), mail_catagory_challenge
        );
    CHECK_EXEC_SQL_RETURN_RD(challenges, resp);

    if(!friends.has_one() || !systems.has_one() || !challenges.has_one())
    {
        resp["ret"] = (int)unknown_error;
        return;
    }

    resp["params"]["friends_mail_count"] = atoi(friends.fetch_row()[0]);
    resp["params"]["system_mail_count"] = atoi(systems.fetch_row()[0]);
    resp["params"]["challenge_mail_count"] = atoi(challenges.fetch_row()[0]);
    resp["ret"] = (int)success;
}

/// handle get mail list
void request_handler::handle_get_mail_list(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    /// check session retrun
    CHECK_SESSION_RETURN(req, resp);

    auto results = sqld->exec_query1("select user_mails.mail_id,user_mails.mail_type,user_mails.mail_sender_id,user.username,user.role_id,user_mails.value1,user_mails.value2 from user_mails,user where user_mails.mail_receiver_id=%d and user_mails.mail_category=%d and user.user_id=user_mails.mail_sender_id limit 30;",
        params.getInt("user_id"),
        params.getInt("mail_category")
        );

    CHECK_EXEC_SQL_RETURN_RD(results, resp);

    auto mail_list = resp["params"]["mail_list"];
    auto count = results.count_of_rows();
    char** row = results.fetch_row();
    for(auto i = 0; i < count; ++i)
    {
        cjsonw::object mail;
        mail["mail_id"] = atoi(row[0]);
        mail["mail_type"] = atoi(row[1]);
        mail["mail_sender_id"] = atoi(row[2]);
        mail["mail_sender_name"] = row[3];
        mail["mail_sender_role_id"] = atoi(row[4]);
        mail["value1"] = atoi(row[5]);
        mail["value2"] = atoi(row[6]);

        mail_list.append(mail);

        row = results.fetch_row();
    }

    resp["ret"] = (int)success;
}

void request_handler::handle_collect_mail(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    /// check session retrun
    CHECK_SESSION_RETURN(req, resp);

    CHECK_EXEC_SQL_RETURN_WR(
        sqld->exec_query0("delete from user_mails where mail_id=%d and mail_receiver_id=%d", 
        params.getInt("mail_id"), 
        params.getInt("user_id")), 
        resp);

    resp["ret"] = (int)success;
}

void request_handler::handle_give_power(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    /// check session retrun
    CHECK_SESSION_RETURN(req, resp);

    /// get send power time
    auto results = sqld->exec_query1("select send_power_time from user_friends where user_id=%d and friend_id=%d", params.getInt("user_id"), params.getInt("receiver_id"));

    CHECK_EXEC_SQL_RETURN_RD(results, resp);

    if(results.has_one()) {
        auto last_send_power_time = atoll(results.fetch_row()[0]);
        if( time(NULL) - last_send_power_time > send_power_interval)
        { // comment: client need check this
            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("update user_friends set send_power_time=%d where user_id=%d and friend_id=%d;", 
                (int)time(NULL), 
                params.getInt("user_id"), 
                params.getInt("receiver_id")),
                resp);

            CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into user_mails values(0,%d,%d,%d,%d,1,0,%d)",
                mail_catagory_friend,
                mail_type_send_power,
                params.getInt("receiver_id"),
                params.getInt("user_id"),
                time(NULL)),
                resp);

            resp["ret"] = (int)success;
            return;
        }
    }
    resp["ret"] = invalid_request;
}

void request_handler::handle_get_product_list(const cjsonw::object& req, cjsonw::object& resp)
{
#define SELECT_PRODUCTS_SQL "select product_id,product_name,product_desc,product_price from iap_info"
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    mysqlcc::result_set results;
    CHECK_EXEC_SQL_RETURN_I(sqld->exec_query_i(SELECT_PRODUCTS_SQL, sizeof(SELECT_PRODUCTS_SQL), &results), resp);

    resp["ret"] = (int)success;
    cjsonw::object product_list;
    char** row = nullptr;
    for(int i = 0; i < results.count_of_rows(); ++i)
    {
        cjsonw::object product;
        row = results.fetch_row();
        product["product_id"] = row[0];
        const char* name = row[1];
        product["product_name"] = row[1];
        product["product_desc"] = row[2];
        product["product_price"] = (float)atof(row[3]); // ios ignore this field
        product_list.append(product);
    }
    resp["params"]["product_list"] = product_list;
}

void request_handler::handle_ios_receipt_verify(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    CHECK_SESSION_RETURN(req, resp);

    int user_id = params.getInt("user_id");

    auto cheat_check = sqld->exec_query1("select 1 from cheat_log where user_id=%d limit 1;", user_id);

    CHECK_EXEC_SQL_RETURN_RD(cheat_check, resp);

    if(cheat_check.has_one()) 
    {
        resp["ret"] = illegal_transaction;
        return;
    }

    cjsonw::object verify_result;
    cjsonw::object receipt;
    receipt["receipt-data"] = req["params"].getString("receipt-data");
    std::string receipt_data_stream = receipt.toString();

    LOG_TRACE_ALL("ios_verify_url:%s", ios_verify_url);
    std::string verify_result_stream = send_http_req("https://buy.itunes.apple.com/verifyReceipt", "POST", receipt_data_stream);
    // LOG_TRACE_ALL("verify stream: %s", verify_result_stream.c_str());
    verify_result.parseString(verify_result_stream.c_str());
    int status = verify_result.getInt("status", -1);
    if(status == 21007) { // For sandbox verify
       verify_result_stream = send_http_req("https://sandbox.itunes.apple.com/verifyReceipt", "POST", receipt_data_stream);
       // LOG_TRACE_ALL("verify stream: %s", verify_result_stream.c_str());
       verify_result.parseString(verify_result_stream.c_str());
       status = verify_result.getInt("status", -1);
    }

    if(0 == status)
    { // TODO: LOG Recharge
        resp["ret"] = 0;
        auto receipt = verify_result["receipt"];
        resp["params"]["product_id"] = receipt.getString("product_id");

        CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into transaction_log (platform_id,transaction_id,product_id,purchase_time) values(%d,'%s','%s',%d)",
            params.getInt("platform_id"),
            receipt.getString("transaction_id"),
            receipt.getString("product_id"),
            atoi(receipt.getString("purchase_date_ms"))), resp);

        LOG_TRACE_ALL("verify succeed, product_id:%s", verify_result["receipt"].getString("product_id"));
    }
    else {
        CHECK_EXEC_SQL_RETURN_WR(sqld->exec_query0("insert into cheat_log (user_id,cheat_type,cheat_detail,timestamp) values(%d,%d,'%s',%d)",
            params.getInt("user_id"),
            1,
            receipt.getString("product_id"),
            time(NULL)), resp);

        LOG_TRACE_ALL("verify failed,status code:%d", verify_result.getInt("status", -1));
        resp["ret"] = (int)ios_verify_receipt_failed;
    }
}

void request_handler::handle_get_announcement(const cjsonw::object& request, cjsonw::object& response)
{
}

void request_handler::handle_check_server_ready(const cjsonw::object& req, cjsonw::object& resp)
{
    resp["ret"] = (int)success;
}

/// void check server ready
void request_handler::handle_get_game_info(const cjsonw::object& req, cjsonw::object& resp)
{
    LOG_TRACE_ALL("%s...", __FUNCTION__);

    // CHECK_SESSION_RETURN(req, resp);

    auto result = sqld->exec_query1("select update_available,download_url from game_info where platform_id=%d", req["params"].getInt("platform_id"));

    CHECK_EXEC_SQL_RETURN_RD(result, resp);

    char** row;
    if(result.has_one() && (row = result.fetch_row()) )
    {
        resp["params"]["update_available"] = (bool)atoi(row[0]);
        resp["params"]["download_url"] = row[1];
        resp["ret"] = (int)success;
    }
    else {
        resp["ret"] = (int)unknown_error;
    }
}

unsigned int request_handler::update_session_id(unsigned int user_id)
{
    LOG_TRACE_ALL("%s", __FUNCTION__);

    if(user_id == 0)
        return 0;

    auto results = sqld->exec_query1("select session_id from user where user_id=%u limit 1;", user_id);
    if(results.validated() && results.has_one())
    {
        unsigned int session_id = atoi(results.fetch_row()[0]);
        if(session_id < 0x7fff - 1)
        {
            session_id = rrand(session_id + 1, 0x7fff);
        }
        else if(session_id < 0x7fffffff - 1) {
            session_id = rrand(session_id + 1, 0x7fffffff);
        }
        else {
            session_id = 1;
        }
        // LOG_TRACE_ALL("update session_id to: %d", session_id);
        return session_id;
    }
    // LOG_TRACE_ALL("%s", "exec sql failed");
    return 0;
}

bool request_handler::check_session_id(unsigned int user_id, unsigned int session_id)
{
    // LOG_TRACE_ALL("%s", __FUNCTION__);
    auto result = sqld->exec_query1("select session_id from user where user_id=%u", user_id);
    if(result.validated() && result.has_one())
    {
        return session_id == atoi(result.fetch_row()[0]);
    }
    LOG_ERROR_ALL("exec sql failed when check session id, detail:user_id:%d, session_id:%d, ip:%s", user_id, session_id, peer_.get_address());
    return false;
}

static bool parse_args(const std::string& in, std::unordered_map<std::string, std::string>& args)
{
    std::string name;
    std::string value;

    enum {
        param_start,
        param_name,
        param_value,
    } state;

    state = param_start;
    for(size_t i = 0; i < in.size(); ++i)
    {
        switch(state)
        {
        case param_start:
            if(in[i] == '=' || in[i] == '&')
            {
                return false; // error
            }
            else {
                state = param_name;
            }
        case param_name:
            if(in[i] != '=' && in[i] != '&')
            {
                name.push_back(in[i]);
            }
            else if(in[i] == '=')
            {
                state = param_value;
            }
            else if(in[i] == '&') {
                return false; // error
            }
            break;
        case param_value:
            if(in[i] != '=' && in[i] != '&')
            {
                value.push_back(in[i]);
            }
            else if(in[i] == '=')
            {
                return false; // error
            }
            else if(in[i] == '&')
            {
                args[name] = crypto::http::urldecode(value);
                name.clear();
                value.clear();
                state = param_name;
            }
            
            break;
        }
    }

    if(!name.empty() && !value.empty())
        args[name] = value;

    return true;
}

void request_handler::handle_url_notify(const std::string& raw_args, reply& rep)
{
    LOG_TRACE("alipay receipt:%s", raw_args.c_str());
    std::unordered_map<std::string, std::string> args;
    if(parse_args(raw_args, args))   
    {
        rep.content = "success";
    }
    else {
        rep.content = "fail";
    }

    rep.headers.resize(2);
    rep.headers[0].name = "Content-Length";
    thelib::nsc::to_xstring(rep.content.size(), rep.headers[0].value);
    rep.headers[1].name = "Content-Type";
    rep.headers[1].value = "application/html";
}

std::string request_handler::get_trade_number(void)
{   
#define NUMBER_FORMAT "%d%02d%02d%02d%02d%02d%03d%d"
    char tms[128];
#ifdef _WIN32
    SYSTEMTIME t;
    GetLocalTime(&t);
    sprintf_s(tms, sizeof(tms), NUMBER_FORMAT, t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, rrand(10000, 99999));
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm* t = localtime(&tv.tv_sec);
    sprintf(tms, NUMBER_FORMAT, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, (int)tv.tv_usec / 1000, rrand(10000, 99999));
#endif
    return tms;
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
    out.clear();
    out.reserve(in.size());
    for (std::size_t i = 0; i < in.size(); ++i)
    {
        if (in[i] == '%')
        {
            if (i + 3 <= in.size())
            {
                int value = 0;
                std::istringstream is(in.substr(i + 1, 2));
                if (is >> std::hex >> value)
                {
                    out += static_cast<char>(value);
                    i += 2;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
            }
        }
        else if (in[i] == '+')
        {
            out += ' ';
        }
        else
        {
            out += in[i];
        }
    }
    return true;
}

