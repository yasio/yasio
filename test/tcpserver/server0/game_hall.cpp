#include "game_room.h"
#include "player_message_handler.h"
#include "game_hall.h"

game_hall::game_hall(game_hall_manager& ownerRef, boost::asio::io_service& svc) : owner_(ownerRef)
{
    this->cur_player_count_ = 0;
    for(int i = 0; i < TOTAL_ROOM_NUM_A_HALL; ++i)
    {
        this->room_list_[i] = new game_room(*this, svc);
    }
}

game_hall::~game_hall(void)
{
    for(int i = 0; i < TOTAL_ROOM_NUM_A_HALL; ++i)
    {
        delete this->room_list_[i];
    }
}

void game_hall::cancel_games(void)
{
    for(int i = 0; i < TOTAL_ROOM_NUM_A_HALL; ++i)
    {
        this->room_list_[i]->cancel_game_timer();
    }
}

void game_hall::count_player(int value)
{
    atomic_add32(this->cur_player_count_,value);
}

uint32_t game_hall::get_current_player_count(void) const
{
    return this->cur_player_count_;
}

game_room* game_hall::locate_room(uint8_t roomNumber)
{
    if(roomNumber < TOTAL_ROOM_NUM_A_HALL)
    {
        return this->room_list_[roomNumber];
    }
    return nullptr;
}

game_room* game_hall::search_room(uint8_t& roomNumber, const player_message_handler& target)
{   
	int emptyRoomNum = -1;
	for(int i = 0; i < TOTAL_ROOM_NUM_A_HALL; i++)
	{
        if(!target.is_history_room(this->room_list_[i])) {
            if(!this->room_list_[i]->room_state_)
            {
                if(this->room_list_[i]->has_player_and_not_full())
                {
                    roomNumber = i;
                    return this->room_list_[i];
                }	
                else if(-1 == emptyRoomNum && this->room_list_[i]->empty())
                {
                    emptyRoomNum = i;
                }
            }
        }
	}
	if(emptyRoomNum != -1)
	{
		roomNumber = emptyRoomNum;
		return this->room_list_[emptyRoomNum];
	}
    return nullptr;
}

