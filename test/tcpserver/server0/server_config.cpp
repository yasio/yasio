#include "server_config.h"
#include "xml3c_api.h"

server_config::server_config(void)
{
    this->gci_.game_count_down_ = 5;
    this->gci_.game_time_ = 180;
    this->sci_.item_use_cd_initial_ = 3000;
    this->sci_.item_use_cd_min_ = 1000;
    this->sci_.item_use_cd_reduce_ = 20;
    this->gci_.hall_mark_up_factor_ = 0.1f;
    this->thread_count_ = 32;
}

bool server_config::load_config(void)
{
    xml3c::document xmlconf("server_config.xml");
    if(xmlconf.is_open())
    {
        xmlconf.get_element("/server_config/thread_count").get_value(this->thread_count_);
        xmlconf.get_element("/server_config/thread_mpool_size").get_value(this->thread_mpool_size_);
        xmlconf.get_element("/server_config/heartbeat_interval").get_value(this->heartbeat_interval_);
        xmlconf.get_element("/server_config/heartbeat_timeout").get_value(this->heartbeat_timeout_);
        xmlconf.get_element("/server_config/heartbeat_check_interval").get_value(this->heartbeat_check_interval_);

        xmlconf.get_element("/server_config/network_config/send_buffer_size").get_value(this->nci_.send_buffer_size_);
        xmlconf.get_element("/server_config/network_config/recv_buffer_size").get_value(this->nci_.recv_buffer_size_);

        xmlconf.get_element("/server_config/game_config/game_count_down").get_value(this->gci_.game_count_down_);
        xmlconf.get_element("/server_config/game_config/game_time").get_value(this->gci_.game_time_);
        xmlconf.get_element("/server_config/game_config/initial_evalue_toplimit").get_value(this->gci_.initial_evalue_toplimit_);
        xmlconf.get_element("/server_config/game_config/initial_gold_capacity").get_value(this->gci_.initial_gold_capacity_);
        xmlconf.get_element("/server_config/game_config/evalue_diff").get_value(this->gci_.evalue_diff_);
        xmlconf.get_element("/server_config/game_config/gold_capacity_diff").get_value(this->gci_.gold_capacity_diff_);
        xmlconf.get_element("/server_config/game_config/level_exp_factor_0").get_value(this->gci_.level_exp_factor_0_);
        xmlconf.get_element("/server_config/game_config/level_exp_factor_1").get_value(this->gci_.level_exp_factor_1_);
        xmlconf.get_element("/server_config/game_config/level_exp_factor_2").get_value(this->gci_.level_exp_factor_2_);
        xmlconf.get_element("/server_config/game_config/level_exp_factor_3").get_value(this->gci_.level_exp_factor_3_);
        xmlconf.get_element("/server_config/game_config/best_player_award_base").get_value(this->gci_.best_player_award_base_);
        xmlconf.get_element("/server_config/game_config/hall_mark_up_factor").get_value(this->gci_.hall_mark_up_factor_);
        xmlconf.get_element("/server_config/game_config/injured_comfort_evalue").get_value(this->gci_.injured_comfort_evalue_);

        xmlconf.get_element("/server_config/service_def/item_use_cd_initial").get_value(this->sci_.item_use_cd_initial_);
        xmlconf.get_element("/server_config/service_def/item_use_cd_min").get_value(this->sci_.item_use_cd_min_);
        xmlconf.get_element("/server_config/service_def/item_use_cd_reduce").get_value(this->sci_.item_use_cd_reduce_);
        xmlconf.get_element("/server_config/service_def/vip_item_cd_rebate").get_value(this->sci_.vip_item_cd_rebate_);
        xmlconf.get_element("/server_config/service_def/vip_cost_rebate").get_value(this->sci_.vip_cost_rebate_);
        xmlconf.get_element("/server_config/service_def/common_donate_def/donate_count").get_property_value("value", this->sci_.common_donate_info_.donate_count_);
        xmlconf.get_element("/server_config/service_def/vip_donate_def/week_vip").get_property_value("donate_count", this->sci_.vip_donate_info_.week_donate_count_);
        xmlconf.get_element("/server_config/service_def/vip_donate_def/month_vip").get_property_value("donate_count", this->sci_.vip_donate_info_.month_donate_count_);
        xmlconf.get_element("/server_config/service_def/vip_donate_def/year_vip").get_property_value("donate_count", this->sci_.vip_donate_info_.year_donate_count_);
        
        xml3c::element edb = xmlconf.get_element("/server_config/database_config");
        edb.get_property_value("database", this->dci_.database_);
        edb.get_property_value("ip", this->dci_.ip_);
        edb.get_property_value("port", this->dci_.port_);
        edb.get_property_value("user", this->dci_.user_);
        edb.get_property_value("password", this->dci_.password_);

        xmlconf.close();
        return true;
    }
    else
    {
        xmlconf.close();
        std::cerr << "missing config file, please check!\n";
        return false;
    }
}

