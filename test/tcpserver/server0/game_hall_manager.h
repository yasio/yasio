/***************************************************************
* game_hall_manager.h - Class game_hall_manager
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see & kyla
*
* Purpose: manage game hall
* 
****************************************************************/
#ifndef _GAME_HALL_MANAGER_H_
#define _GAME_HALL_MANAGER_H_
#include "player_message_handler.h"

class game_hall_manager
{
    friend class game_hall;
public:
    game_hall_manager(player_server& owner, boost::asio::io_service&);
    ~game_hall_manager(void);

    void               cancel_all_games(void);

    /*
    ** @brief: Locate a hall by hall id
    ** @params:
    **         hall_number_: a serial number from 1~5
    **                 but internal use: 0~4
    */
    game_hall*         locate_hall(uint8_t hall_number_);
 
    /*
    ** @brief: Get total player of all hall
    ** @params:
    */
    uint32_t           get_total_player_count(void) const;

    uint32_t           get_hall_player_count(uint8_t hallNr) const;

private:
    player_server&     owner_;
    game_hall*         hall_list_[TOTAL_HALL_NUM];
};

#endif

