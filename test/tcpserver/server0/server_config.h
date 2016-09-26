#ifndef _SERVER_CONFIG_H_
#define _SERVER_CONFIG_H_
#include "xxpiedef.h"

struct network_config_info
{
    int send_buffer_size_;
    int recv_buffer_size_;
};

struct game_config_info
{
    int   game_count_down_;
    int   game_time_;
    int   initial_evalue_toplimit_;
    int   initial_gold_capacity_;
    int   evalue_diff_;
    int   gold_capacity_diff_;
    int   level_exp_factor_0_;
    int   level_exp_factor_1_;
    int   level_exp_factor_2_;
    int   level_exp_factor_3_;
    int   best_player_award_base_;
    float hall_mark_up_factor_;
    int   injured_comfort_evalue_;
};

struct service_config_info
{
    int  item_use_cd_initial_;
    int  item_use_cd_min_;
    int  item_use_cd_reduce_;
    int  vip_item_cd_rebate_;
    int  vip_cost_rebate_;
    struct {
        int donate_count_;
    } common_donate_info_;
    struct {
        int week_donate_count_;
        int month_donate_count_;
        int year_donate_count_;
    } vip_donate_info_;
};

struct database_config_info
{
    std::string database_;
    std::string ip_;
    u_short     port_;
    std::string user_;
    std::string password_;
};

class server_config
{
public:
    server_config(void);
    bool load_config(void);
    int                  thread_count_;
    int                  thread_mpool_size_;
    int                  heartbeat_interval_;
    int                  heartbeat_timeout_;
    int                  heartbeat_check_interval_;
    network_config_info  nci_;       
    game_config_info     gci_;
    service_config_info  sci_;
    database_config_info dci_;
};

#endif
