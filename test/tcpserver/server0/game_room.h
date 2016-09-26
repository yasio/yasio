/*********************************************************************
* game_room.h - Class game_room
*                Copyright (c) 2012 pholang.com. All rights reserved.
*
* Author: xml3see & kyla
*
* Purpose: Group player connection by room, every room has 6 player.
* 
*********************************************************************/
#ifndef _PLAYERCONNROOM_H_
#define _PLAYERCONNROOM_H_
#include "auto_resources.h"
#include "game_widget_table.h"
#include "protocol_messages.h"

class game_room
{
public:
    static const int GAME_DURATION_TIME = 60 * 3; // 3min
    game_room(game_hall&,boost::asio::io_service&);
    ~game_room(void);

    void                start_game(void);

    void                on_game_end(const boost::system::error_code&);

    /* 
    ** put a connect into room
    ** return: succeed send to player number.
    */
    int                 broadcast(player_message_handler* trigger, const void* msg, int len);

    int                 broadcast_to_all(const void* msg, int len);

    void                private_send(uint32_t player_id, const void* msg, int len);

    void                post_donate_info_for_all(void);

    void                broad_game_start_info(void);

    /* 
    ** @brief: interface for game_room to inform a player that game already end.
    */
    void                broad_game_end_info(void);

    void                reset_game_data(void);

    /* 
    ** put a connect into room
    ** return: >=0 index in the room, -1: falied
    */
    int                 enter(player_message_handler* const job);

    /* 
    ** Get other info in the room
    ** return: 0~5, not include requestor
    */
    int                 get_player_info_list(player_info infos[6]);
    int                 calc_player_game_results(messages::game_end_information& result);
    int                 get_effected_players(int collision_x, int collision_y, uint8_t issuer_iir, const widget_attr& wa_atk, bool is_vip, int& damage_count, item_valid_range_info infos[6]);
    /* 
    ** clear a connect from room
    */
    void                exit(player_message_handler* const player, int indexInRoom);

    int                 get_current_player_count(void) const;

    uint8_t             get_room_state(void)const;

    void                set_room_state(uint8_t room_state_);

    bool                is_widget_exists(uint16_t widgetSerialNum) const;

    // retval: nullptr: not exist
    simple_widget_info* get_widget_info_by_sn(uint32_t widgetSerialNum);

    void                add_widget(uint16_t id, uint16_t x, uint32_t sn, uint32_t holder = 0xffffffff, bool is_vip = false);
    void                remove_widget(uint32_t widgetSerialNum);

    bool                has_player_and_not_full(void) const;

    bool                empty(void) const;

    void                change_ready_count(int value);

    int                 get_ready_player_count(void) const;
    
    void                cancel_game_timer(void);

// private:
    game_hall&                   owner_;
    player_message_handler*      player_grp_[MAX_PLAYER_NUM_A_ROOM]; 
    thread_rwlock                player_grp_rwlock_;
    uint8_t                      room_state_;
    int                          cur_player_count_;
    int                          cur_ready_player_count_;
    mutable thread_rwlock        ready_count_rwlock_;

    boost::asio::deadline_timer  game_timer_;

    swi_table_t                  widget_tab_;
    mutable thread_rwlock        widget_tab_rwlock_;
    uint32_t                     donate_item_serial_num_;
    uint32_t                     donate_vip_item_serial_num_;
};

static const uint32_t WIDGETS_COUNT = 780;
static const int8_t WIDGET_DROP_SPEED = 1;
static const uint32_t GIFT_SERIAL_BASE_NUM = 0x80000000;
static const uint32_t GIFT_VIP_SERIAL_BASE_NUM = 0x90000000;

#endif

