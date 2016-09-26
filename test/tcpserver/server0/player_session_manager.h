/*********************************************************************
* player_session_manager.h - Class player_session_manager
*                Copyright (c) 2012 pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: manage logged player_session, for duplicate id detect
* 
*********************************************************************/
#ifndef _PLAYER_SESSION_MANAGER_H_
#define _PLAYER_SESSION_MANAGER_H_
#include "xxpiedef.h"

typedef std::set<player_session*, std::less<player_session*>, exx::gc::object_pool_alloctor<player_session*> > session_set_t;
typedef std::map<uint32_t, player_session*, std::less<uint32_t>, exx::gc::object_pool_alloctor<std::pair<uint32_t, player_session*> > > session_table_t;

class player_session_manager
{
    friend class player_session;
public:
    player_session_manager(player_server& owner, boost::asio::io_service&);
    ~player_session_manager(void);

    void         add(player_session* session);

    bool         tryregister(player_session* first);

    /*
    ** @brief: login force and replace previous session
    ** @return: previous session replaced by new session
    **         nullptr: previous session already exit
    */
    player_session*  reregister(player_session* second);


    /*
    ** @brief: unregister the session
    **
    */
    void             unregister(player_session* session);

    /*
    ** @brief: Cleanup the session socket resource and prepare for next accept op
    **
    */
    void             cleanup(player_session* session);

    /*
    ** @brief: Post the session timeout async check op
    **
    */
    void             post_session_timeout_check(void);

    /*
    ** @brief: Session timing check handler
    **
    */
    void             on_session_timeout_checking(const boost::system::error_code& ec);
    
    void             cancel_session_timeout_check(void);

private:
    player_server&              owner_;
    session_set_t               session_set_unlogin_;
    mutable thread_rwlock       session_set_unlogin_rwlock_;

    session_table_t             session_tab_login_;
    mutable thread_rwlock       session_tab_login_rwlock_;
    
    boost::asio::deadline_timer session_check_timer_;
};

#endif

