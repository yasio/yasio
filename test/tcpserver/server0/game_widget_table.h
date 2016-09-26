/***************************************************************
* game_widget_table.h - Class game_widget_table
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see & kyla
*
* Purpose: define all game widget table
* 
****************************************************************/
#ifndef _GAME_WIDGET_TABLE_H_
#define _GAME_WIDGET_TABLE_H_
#include "xxpiedef.h"
#include "player_message_handler.h"

using namespace exx::asy;

typedef std::map<uint32_t, simple_widget_info, std::less<uint32_t>, exx::gc::object_pool_alloctor<std::pair<uint32_t, simple_widget_info> > > swi_table_t;

namespace xml3c {
    class element;
};

class game_widget_table
{
public:
    ~game_widget_table(void);

    void               init(void);

    const item_cost_info* get_item_info_list(void) const;

    uint16_t           get_random_widget_id(void) const;

    const item_cost_info* get_item_cost_info_list(void) const;

    bool               is_known_widget(uint16_t widgetId) const;

    bool               is_item(uint16_t widgetId) const;

    int                get_widget_bounty(uint16_t widgetId) const;

    bool               is_item(const widget_attr& wa) const;
    const widget_attr& get_widget_attr_by_id(uint16_t widget_id) const;

    /*
    ** @brief: spawn game widgets list
    ** 
    */
    void               spawn_widgets(int widget_count, game_room& room, drop_widget_info* wlst);

protected:
    void               handle_config_loading(const xml3c::element& elem);
private:
    std::map<uint32_t, widget_attr>          widget_table_;
    std::vector<uint16_t>                    widget_sum_prob_list_;
    item_cost_info                           item_cost_info_list_[9];
};

#endif

