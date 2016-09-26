/***************************************************************
* game_hall.h - Class game_hall
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see & kyla
*
* Purpose: game hall definination
* 
****************************************************************/
#ifndef _GAME_HALL_H_
#define _GAME_HALL_H_
#include "player_message_handler.h"

class game_hall
{
public:
    game_hall(game_hall_manager& ownerRef, boost::asio::io_service&);
    ~game_hall(void);

    void         cancel_games(void);

    //// @brief: enter hall, only record player count in a all
    //// @return: this pointer
    //void         enter(void);

    //// leave hall
    //void         leave(void);

    void         count_player(int value);

    game_room*   locate_room(uint8_t roomNumber); 
	game_room*   search_room(uint8_t& roomNumber, const player_message_handler& player); // find a room

    uint32_t     get_current_player_count(void) const;

private:
    game_hall_manager&  owner_;
    game_room*          room_list_[TOTAL_ROOM_NUM_A_HALL];
    volatile int        cur_player_count_;  // current player number in this hall
};

#endif

