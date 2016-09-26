/*********************************************************************
* player_server.h - Class player_server
*                Copyright (c) 2012 pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: The player connection process core. Contains all components
*          For processing player connection, such as:
*          player Connection Handler,
*          player Connection Handler manager ,
*          Game Hall Manager
* 
**********************************************************************/
#ifndef _PLAYER_SERVER_H_
#define _PLAYER_SERVER_H_

#include "player_message_handler.h"

typedef enum {
    server_state_stopped,
    server_state_starting,
    server_state_started,
    server_state_stopping
} server_state;

class player_server
{
public:
    player_server(void);
    ~player_server(void);


    bool                         start(const char* addr, u_short port);
    void                         post_stop_signals(void);
    void                         wait_all_workers(void);

    void                         prepare_accept(size_t number = 1);
    void                         on_accept(player_session*, const boost::system::error_code& error);

    void                         svc(void);

    player_session_manager&      get_session_manager(void);
    game_hall_manager&           get_hall_manager(void);

private:
    server_state                 state_;
    thread_mutex                 state_mutex_;
    game_hall_manager*           hall_manager_;
    player_session_manager*      session_manager_; 
    tcp::acceptor*               tcp_acceptor_;
    boost::asio::io_service      io_service_;
    std::list<std::shared_ptr<boost::thread>>  svc_worker_grp_;
};

#endif
