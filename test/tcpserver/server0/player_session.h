/*********************************************************************
* player_session.h - Class player_session
*                Copyright (c) 2012 pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: work for player connection through holder a conn,proto pair.
*          a important object, provide all interface for message handler.
* 
*********************************************************************/
#ifndef _PLAYER_SESSION_H_
#define _PLAYER_SESSION_H_
#include "player_message_handler.h"

class player_session
{
    friend class player_session_manager;
public:
    player_session(player_session_manager& owner, boost::asio::io_service&);
    ~player_session(void);

    void                close(void);

    tcp::socket&        get_socket(void);

    uint32_t            get_id(void) const;

    /*
    ** @brief: get peer address information: example: 127.0.0.1:8080
    **
    */
    const char*         get_addr_info(void) const;

    /*
    ** @brief: set socket options, keepalive
    **
    */
    void                set_options(void);   

    bool                is_valid(void) const;

    bool                is_kicked(void) const;

     /*
    ** @brief: logoff a session when double player who have the same id try logon.
    **
    */
    void                kick(const char* detail);

    bool                trylogon(void);

    player_session*     relogon(void);

    /*
    ** @brief: clear client state
    **
    */
    void                logoff(void);

    void                init(void);

    /*
    ** @brief: start to process client bussiness. 
    **
    */
    void                start(void); 
        
    /*
    ** @brief: send a message to the peer(player)
    **
    */
    void                send(const void* msg, int len);

protected:

    void                handle_tgw_header_part1(const boost::system::error_code& error, size_t bytes_transferred);
    void                handle_tgw_header_part2(const boost::system::error_code& error, size_t bytes_transferred);

    void                post_timeout_checking(void);

    /*
    ** @brief: call boost ansync_read to post a receive request to io service
    ** @params:
    **         bytes_unconsumed:  bytes unconsumed in the player_session::recv_buf_
    */
    void                post_recv(char* buffer, size_t size, size_t bytes_at_least);

	void                post_send(const void* bytestream, size_t size);

    /*
    ** @brief: a io_service callback function when io operation is complete. 
    ** @params:
    **         bytes_transferred: bytes received from socket
    **         bytes_unconsumed:  bytes unconsumed in the player_session::recv_buf_
    ** @remark:
    **         The only enter for remove a logon or unlogon session
    */
    void                on_recv_completed(const boost::system::error_code& error, size_t bytes_transferred);

    void                on_send_completed(char* buffer, const boost::system::error_code&, size_t);

    void                on_timeout_checking(const boost::system::error_code& error);

    bool                execute_timeout_check(void);

private:
    player_session_manager&  manager_;
    player_message_handler   message_handler_;                   // message handler, core protocol processor.
    mutable tcp::socket      s_;                          // socket
    thread_mutex             send_lock_;                  // for send operation
    char                     recv_buf_[MAX_IOBUF_SIZE];   // recv recv_buf_
    size_t                   bytes_to_consume_;           // bytes unconsumed in the buffer
    char                     detail_[IPV4_MAX_LEN];       // address information
    bool                     is_kicked_;
    bool                     is_logged_off_;
    thread_mutex             logoff_mutex_;
    boost::asio::deadline_timer timeout_check_timer_;
};

static const int sz = sizeof(player_session);

#endif

