/***************************************************************
* player_message_handler.h - Class player_message_handler
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see & kyla
*
* Purpose: player message requestions process core.
* 
****************************************************************/
#ifndef _PLAYERPROTOCOL_H_
#define _PLAYERPROTOCOL_H_
#include "protocol_structures.h"

typedef bool (player_message_handler::*PLAYER_MSG_HANDLER_PROC)(const char* message, int length);

typedef enum  {
    DISCONNECTED,
    CONNECTED,
    IN_A_HALL,
    IN_A_ROOM
} proto_state_t;

typedef enum  {
    player_state_not_ready = 0,
    player_state_ready,
    player_state_playing
} player_state;

typedef enum  {
    HEIGHT = 700,
    WIDTH = 560,
    MIN_STEP = 1,
    MAX_STEP = 5
} MoveRange;

typedef enum {
    WEEK = 7,
    MONTH = 30,
    YEAR = 365
}ServiceType;

typedef enum {
    MONTH_NUM = 7,
    YEAR_NUM = 15,
    WEEK_NUM = 4,
    GENERAL_NUM = 100
}GiftItemNumType;

typedef enum  {
    egr_normally,
    egr_quickly_handover,
    egr_abnormal
} exit_game_reason;

static const int MAX_NUM_PER_SERVICE = YEAR_NUM;
static const int MAX_SERVICE_COM_NUM = 36;
static const int MAX_VALID_ANGLE = 360;
using namespace structures;

class player_message_handler
{
    friend class game_room; // 
    friend class player_session;
public:
    static std::map<u_long, PLAYER_MSG_HANDLER_PROC> s_msg_handler_tab;

    player_message_handler(player_session&, game_hall_manager&);
    ~player_message_handler(void);

     // Called by session when connection established
    void         on_connection(void);

public:
    /// All mesasge handlers;
    bool         handle_room_list_refresh_req         (const char* bstream, int length);
    bool         handle_view_online_count_req         (const char* bstream, int length);
    bool         handle_enter_room_req                (const char* bstream, int length);
    bool         handle_exit_room_req                 (const char* bstream, int length);
    bool         handle_move_req                      (const char* bstream, int length);

    bool         handle_widget_collision_req          (const char* bstream, int length);

    bool         handle_use_item_req                  (const char* bstream, int length);
    bool         handle_issuing_item_collision_notify (const char* bstream, int length);

    bool         handle_change_ready_state_req        (const char* bstream, int length);

    bool         handle_hall_update_req               (const char* bstream, int length);
    bool         handle_chat_message                  (const char* bstream, int length);
    bool         handle_heartbeat_req                 (const char* bstream, int length);
    bool         handle_effect_change_req             (const char* bstream, int length);

    bool         handle_new_player_logon_request      (const char* bstream, int length);
    bool         handle_later_player_logon_request    (const char* bstream, int length);

public:
    /*
    ** @brief: interface for player_session to process message from network.
    */
    bool         on_message(const char* bstream, int length);

    /*
    ** @brief: interface for player_session to unpack bytestream from network.
    */
    size_t       decode_message_length(const char* bstream, int length);

    /*
    ** @brief: interface for player_session to when peer exit abnormally,
    **         such as: client-browser closed forced.
    */
    void         on_disconnect(void);

    /*
    ** @brief: interface for game_room to send message to a specified player.
    */
    player_session& get_session(void) const;

    /* 
    ** @brief: interface for game_room when search a avalible room for
    **         a player
    */
    bool         is_history_room(game_room* const room) const;

    /* 
    ** @brief: interface for game_room, get player game result
    */
    void         get_game_result(player_game_result& game_result); 

    bool         is_evalue_full(void) const;

    void         try_clear_injured_times(void);

    bool         is_heartbeat_timeout(void) const;
protected:
    /*
    ** @brief: enter a hall
    ** @params: 
    **         hall_number_: request hall id
    ** @retval: false: failed, hall was full
    */
    bool          enter_hall(uint8_t hall_number);

    /*
    ** @brief: exit the hall
    **
    */
    void          exit_hall(void);

    /* 
    ** @brief: enter a room
    ** 
    ** returns: 0~5: index in room
    **          -2 : failed, room already full.
    **			-1 : the player is not in a hall
    */
    int          enter_room(uint8_t roomNumber);

    /*
    ** @brief: exit a room if in the room
    **
    */
    void         exit_room(void);

    uint32_t     add_evalue(uint32_t post_evalue);

    /* 
    ** @brief: quik enter a room
    ** 
    ** returns: 0~5: index in room
    ** roomNumber: record the number of room the player entered
    **          -2 : failed, all rooms has already full.
    **			-1 ：the player is not in a hall
    */
    uint8_t	     quick_enter_room(uint8_t& index_in_room);

    void         notify_exit_room(exit_game_reason reason, const char* log);

    void         cycling_append_histroy_room(void);

    int          calc_gold_award(uint16_t widget_id, const widget_attr& attr);
    void         calc_game_result(uint32_t& exp_mark_up, int32_t& gold_mark_up);
    void         update_data_based_on_level(void);
    void         clear_game_temp_values(void);

    void         post_donate_info(void);
    int          get_donate_item_count(void);
    int          count_of_donate_item(uint32_t serviceId);

    void         read_level_info(uint32_t player_id);
    void         read_service_info(uint32_t player_id);

    uint8_t      calc_dodge_item_attack_code(const widget_attr& wa);

private:
    player_session&        session_;
    game_hall_manager&     hall_mgr_;
    proto_state_t          proto_state_;        // palyer protocol state

    game_hall*             hall_;               // the hall where this player at.
    game_room*             room_;               // the room where this player at.
    uint8_t                hall_number_;        // hall number (0..4)
    player_info            player_info_;        // player basic infomation
    
    uint8_t                strength_;           // 玩家力量值
    uint16_t               dodge_;              // 玩家闪避
    uint32_t               round_of_game_exp_;  // player a round of game exp
    int32_t                round_of_game_gold_; // player a round gold
    seq_got_widget_info    seq_got_wi_;         // player got sequently widget info

    //uint8_t                donate_item_count_;  // 赠送玩家的物品的个数
    game_room*             history_rooms_[5];   // 玩家历史进入房间
    size_t                 history_room_index_; // 玩家最后进入房间索引
    uint32_t               player_effect_mask_; // 玩家使用道具后效果代码
    float                  anti_addiction_factor_; // 玩家防沉迷因数
    uint32_t               injured_times_;
    time_t                 heartbeat_timestamp_;   //
};

#endif

