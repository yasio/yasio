#include "game_hall.h"
#include "game_hall_manager.h"

game_hall_manager::game_hall_manager(player_server& owner, boost::asio::io_service& svc) : owner_(owner)
{
    for(int i = 0; i < TOTAL_HALL_NUM; ++i)
    {
        this->hall_list_[i] = new game_hall(*this, svc);
    }
}

game_hall_manager::~game_hall_manager(void)
{
    for(int i = 0; i < TOTAL_HALL_NUM; ++i)
    {
        delete this->hall_list_[i];
    }
}

void  game_hall_manager::cancel_all_games(void)
{
    for(int i = 0; i < TOTAL_HALL_NUM; ++i)
    {
        this->hall_list_[i]->cancel_games();
    }
}

uint32_t game_hall_manager::get_total_player_count(void) const
{
    uint32_t totalPlayerNum = 0;
    for(int i = 0; i < TOTAL_HALL_NUM; ++i)
    {
        totalPlayerNum += this->hall_list_[i]->get_current_player_count();
    }
    return totalPlayerNum;
}

game_hall*  game_hall_manager::locate_hall(uint8_t hall_number_)
{
    // hall_number_ 0..4
    if(hall_number_ < TOTAL_HALL_NUM)
    {
        return this->hall_list_[hall_number_];
    }
    return nullptr;
}

uint32_t game_hall_manager::get_hall_player_count(uint8_t hallIndex) const
{
    return this->hall_list_[hallIndex]->get_current_player_count();
}


