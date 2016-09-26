#include <stdlib.h>
#include <time.h>
#include "server_config.h"
#include "xxpie_logger.h"
#include "game_widget_table.h"
#include "mysql3c.h"
#include "xml3c_api.h"
#include "player_message_handler.h"
#include "protocol_contants.h"
#include "player_session.h"
#include "auto_resources.h"
using namespace constants;

server_config     auto_resources::ar_server_config;
game_widget_table auto_resources::ar_game_widget_table;
ngx_pool_t*       auto_resources::ar_ngx_pool = nullptr;
thread_mutex      auto_resources::ar_ngx_pool_mutex;
mysql3c::database auto_resources::ar_db;
exx::gc::object_pool<player_session, MAX_CONN_COUNT + 1> auto_resources::ar_conn_pool;
std::map<u_long, PLAYER_MSG_HANDLER_PROC> player_message_handler::s_msg_handler_tab;
unsigned int      auto_resources::ar_random_seed = 0;

namespace auto_resources {
    class auto_resources_manager
    {
    public:
        auto_resources_manager(void)
        {
            xml3c::initialize();
            this->init_random_seed();
            this->init_config();
            this->init_logger();
            this->init_widget_table();
            this->init_mpool();
            this->init_database();
            this->init_message_map_table();
        }

        ~auto_resources_manager(void)
        {
            player_message_handler::s_msg_handler_tab.clear();
            ar_db.close();
            ngx_destroy_pool(ar_ngx_pool);
            xml3c::cleanup();
        }

    protected:
        void init_random_seed(void)
        {
            ar_random_seed = (unsigned int)time(nullptr);
        }

        void init_config(void)
        {
            if(!ar_server_config.load_config())
            {
                exit(0);
            }
        }

        void init_logger(void)
        {
            (void) singleton<xxpie_logger>::instance();
        }

        void init_widget_table(void)
        {
            ar_game_widget_table.init();
        }

        void init_mpool(void)
        {
            ar_ngx_pool = ngx_create_pool(SZ(ar_server_config.thread_mpool_size_, M));
            if(nullptr == ar_ngx_pool)
            {
                LOG_ERROR("create memory pool failed!\n");
                exit(0);
            }
        }

        void init_database(void)
        {
            if(!ar_db.open(ar_server_config.dci_.database_.c_str(), 
                ar_server_config.dci_.ip_.c_str(), 
                ar_server_config.dci_.port_, 
                ar_server_config.dci_.user_.c_str(), 
                ar_server_config.dci_.password_.c_str()))
            {
                LOG_ERROR("open database failed, please check your database config!\n");
                exit(0);
            }
        }

        void init_message_map_table(void)
        {
            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_ROOM_LIST_REFRESH_REQUEST_MT_VALUE, 
                &player_message_handler::handle_room_list_refresh_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_VIEW_ONLINE_COUNT_REQUEST_MT_VALUE, 
                &player_message_handler::handle_view_online_count_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_ENTER_ROOM_REQUEST_MT_VALUE, 
                &player_message_handler::handle_enter_room_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_MOVE_REQUEST_MT_VALUE, 
                &player_message_handler::handle_move_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_USE_ITEM_REQUEST_MT_VALUE, 
                &player_message_handler::handle_use_item_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(ISSUING_ITEM_COLLISION_INFO_MT_VALUE, 
                &player_message_handler::handle_issuing_item_collision_notify));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(WIDGET_COLLISION_REQUEST_MT_VALUE, 
                &player_message_handler::handle_widget_collision_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_EXIT_ROOM_REQUEST_MT_VALUE, 
                &player_message_handler::handle_exit_room_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_CHANGE_READY_REQUEST_MT_VALUE, 
                &player_message_handler::handle_change_ready_state_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_HALL_UPDATE_REQUEST_MT_VALUE,
                &player_message_handler::handle_hall_update_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_CHAT_MESSAGE_MT_VALUE, 
                &player_message_handler::handle_chat_message));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_HEARTBEAT_REQUEST_MT_VALUE, 
                &player_message_handler::handle_heartbeat_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(PLAYER_EFFECT_CHANGE_REQUEST_MT_VALUE,
                &player_message_handler::handle_effect_change_req));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(LATER_PLAYER_LOGON_REQUEST_MT_VALUE,
                &player_message_handler::handle_later_player_logon_request));

            player_message_handler::s_msg_handler_tab.insert(std::make_pair(NEW_PLAYER_LOGON_REQUEST_MT_VALUE,
                &player_message_handler::handle_new_player_logon_request));
        }
    };

    // auto_resources_manager __auto_resources_manager;
};

//  The common secondary arithmetic progression's General term formula
//int64_t csapgtf(int64_t n, int64_t a1 = 0, int64_t a2 = 305, int64_t a3 = 810)
//{
//    return (int64_t)( a1 + (a2 - a1) * (n - 1) +  ( ( (a3 - a2) - (a2 - a1) ) / 2.0 ) * (n - 1) * (n - 2) );
//}

// 一元二次方程(quadratic equation with one unknown) 求根公式: ( -b + sqrt( (b^2 - 4ac) ) ) / 2a

static int64_t eval_exp_by_level_i(double level, int64_t a1, int64_t a2, int64_t a3)
{
    double a = (a1 + a3) / 2.0 - a2;
    double b = 4 * a2 - 5 / 2.0 * a1 - 3 / 2.0 * a3;
    double c = a3 - (a2 - a1) * 3;
    return (int64_t)(a * level * level + b * level + c);
    /*return (int64_t)(
        (a1 - 2 * a2 + a3) / 2.0 * level * level + (8 * a2 + 5 * a1 - 3 * a3) / 2.0 * level + (3 * a1 - 3 * a2 + a3)
        );*/
}

static double eval_level_by_exp_i(int64_t exp, int64_t a1, int64_t a2, int64_t a3)
{
    double a = (a1 + a3) / 2.0 - a2;
    double b = 4 * a2 - 5 / 2.0 * a1 - 3 / 2.0 * a3;
    double c = a3 - (a2 - a1) * 3 - exp;
    return ( -b + sqrt( (b * b - 4 * a * c) ) ) / (2 * a);
}

int64_t eval_exp_by_level(double level) 
{
    return eval_exp_by_level_i(level, 
        ar_server_config.gci_.level_exp_factor_1_,
        ar_server_config.gci_.level_exp_factor_2_,
        ar_server_config.gci_.level_exp_factor_3_);
    //return (int) ( (100 * level + 105) * (level - 1) );
}

double eval_level_by_exp(int64_t exp)
{
    return eval_level_by_exp_i(exp, 
        ar_server_config.gci_.level_exp_factor_1_,
        ar_server_config.gci_.level_exp_factor_2_,
        ar_server_config.gci_.level_exp_factor_3_);
    //return ( sqrt( ( 25.0 + 400.0 * (105 + exp) ) ) / 200.0 - 0.025 );
}

void* xmpool_alloc_atom(size_t size)
{
    void* user_data;
    ar_ngx_pool_mutex.lock();
    user_data = ngx_palloc(ar_ngx_pool, size);
    ar_ngx_pool_mutex.unlock();
    return user_data;
}

void  xmpool_free_atom(void* ptr)
{
    ar_ngx_pool_mutex.lock();
    ngx_pfree(ar_ngx_pool, ptr);
    ar_ngx_pool_mutex.unlock();
}

int next_rand_int(void)
{
    return ( ((ar_random_seed = ar_random_seed * 214013L
        + 2531011L) >> 16) & 0x7fff );
}

void xxpie_server_initialize(void)
{
    static auto_resources::auto_resources_manager __auto_resources_manager;
}

