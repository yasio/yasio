#include "server_config.h"
#include "player_server.h"
#include "player_session.h"
#include "game_hall.h"
#include "player_message_handler.h"
#include "mysql3c.h"

#include "game_room.h"

using namespace messages;

#ifndef xxmax
#define xxmax(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef xxmin
#define xxmin(a,b)            (((a) < (b)) ? (a) : (b))
#endif

game_room::game_room(game_hall& owner,boost::asio::io_service& svcref) : owner_(owner), cur_player_count_(0), game_timer_(svcref)
{
    ::memset(this->player_grp_, 0x0, sizeof(this->player_grp_));
    this->room_state_ = room_state_idle;
    this->cur_player_count_ = 0;
    this->cur_ready_player_count_ = 0;
    this->donate_item_serial_num_ = GIFT_SERIAL_BASE_NUM;
    this->donate_vip_item_serial_num_ = GIFT_VIP_SERIAL_BASE_NUM;
}

game_room::~game_room(void)
{
}

void game_room::start_game(void)
{
    if(room_state_idle == this->room_state_) {

        this->room_state_ = room_state_playing;

        this->post_donate_info_for_all();
        this->broad_game_start_info();

        // start a timerd
        this->game_timer_.expires_from_now(boost::posix_time::seconds(ar_server_config.gci_.game_time_));
        this->game_timer_.async_wait(
            boost::bind(
            &game_room::on_game_end,
            this,
            boost::asio::placeholders::error)
            );
    }
}

void game_room::on_game_end(const boost::system::error_code& error)
{
    if(!error) 
    {
        this->broad_game_end_info();
        this->reset_game_data();
    }
}

int game_room::broadcast(player_message_handler* trigger, const void* msg, int len)
{
    int count = 0;

    this->player_grp_rwlock_.lock_shared();

    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        if(this->player_grp_[i] != trigger && this->player_grp_[i] != nullptr && IN_A_ROOM == this->player_grp_[i]->proto_state_) 
        {
            this->player_grp_[i]->get_session().send(msg, len);
            ++count;
        }
    }

    this->player_grp_rwlock_.unlock_shared();

    return count;
}

int game_room::broadcast_to_all(const void* msg, int len)
{
    int i = 0;
    this->player_grp_rwlock_.lock_shared();
    for(; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    { 
        if(this->player_grp_[i] != nullptr && IN_A_ROOM == this->player_grp_[i]->proto_state_) 
        {
            this->player_grp_[i]->get_session().send(msg, len);
        }
    }
    this->player_grp_rwlock_.unlock_shared();
    return i;
}

void game_room::private_send(uint32_t player_id, const void* msg, int len)
{
    scoped_rlock<thread_rwlock> shared_guard(this->player_grp_rwlock_);

    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        if(this->player_grp_[i] != nullptr && player_id == this->player_grp_[i]->player_info_.ci_.id_)
        {
            return this->player_grp_[i]->get_session().send(msg, len);
        }
    }
}

void game_room::post_donate_info_for_all(void)
{
    scoped_rlock<thread_rwlock> shared_guard(this->player_grp_rwlock_);

    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        if(this->player_grp_[i] != nullptr)
        {
            this->player_grp_[i]->player_info_.ci_.ready_state_ = player_state_playing;
            this->player_grp_[i]->post_donate_info();
        }
    }
}

void game_room::broad_game_start_info(void)
{
    game_start_information gsi;

    gsi.second_ = ar_server_config.gci_.game_count_down_;

    // players service
    player_message_handler* iter = nullptr;
    gsi.player_count_ = 0;
    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        iter = this->player_grp_[i];
        if(iter != nullptr)
        {
            int player_index = gsi.player_count_++;
            gsi.player_service_infos_[player_index].player_id_ = iter->player_info_.ci_.id_;
            gsi.player_service_infos_[player_index].service_count_ = iter->player_info_.si_.service_count_;
            memcpy(gsi.player_service_infos_[player_index].service_infos_, iter->player_info_.si_.service_infos_, sizeof(iter->player_info_.si_.service_infos_));
        }
    }

    gsi.drop_widget_count_ = WIDGETS_COUNT;
    gsi.widgets_drop_speed_ = WIDGET_DROP_SPEED;

    size_t drop_wi_size = WIDGETS_COUNT * sizeof(drop_widget_info);
    gsi.drop_widget_infos_ = (decltype(gsi.drop_widget_infos_))mpool_alloc(drop_wi_size);

    ar_game_widget_table.spawn_widgets(gsi.drop_widget_count_, *this, gsi.drop_widget_infos_);

    PROTO_LOG(gsi.get_formated_string().c_str(),"Server","All Players");

    size_t bssize = sizeof(gsi) + WIDGETS_COUNT * sizeof(drop_widget_info);
    char* bstream = (char*)mpool_alloc(bssize);
    int len = 0;
    gsi.encode(bstream, len);
    this->broadcast_to_all(bstream, len);

    mpool_free(bstream);
    mpool_free(gsi.drop_widget_infos_);
}

void game_room::broad_game_end_info(void)
{
    game_end_information info;
    /*info.best_player_gold_award_ = ar_server_config.gci_.best_player_award_base_ * (this->cur_player_count_ - 1);
    info.best_player_exp_award_ = ar_server_config.gci_.best_player_award_base_ * (this->cur_player_count_ - 1);
    info.player_count_ = this->get_player_game_results(info.player_game_results_, info.best_player_id_);*/
    this->calc_player_game_results(info);
    char info_data[ sizeof(info) ];
    int  data_len = 0;
    info.encode(info_data, data_len);
    this->broadcast_to_all(info_data, data_len);
    PROTO_LOG(info.get_formated_string().c_str(), "Server", "All");

    //  write players': exp, level,history_highest_round_exp_ to database
    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        player_message_handler* iter = this->player_grp_[i];
        if(iter != nullptr)
        {
            char sqlstr[512];

            sprintf_s(sqlstr, 
                sizeof(sqlstr),
                "update user_exp set level = %u,"
                "total_exp = %u,"
                "total_gold = %d,"
                "history_max_exp = %u,"
				"history_max_gold = %u,"
                "max_exp_date = sysdate() "
                "where uid = %u", 
                iter->player_info_.si_.level_,
                (uint32_t)eval_exp_by_level(iter->player_info_.si_.level_) + iter->player_info_.si_.level_cur_exp_,
                iter->player_info_.si_.total_gold_,
                iter->player_info_.si_.history_highest_round_exp_,
				iter->player_info_.si_.history_highest_round_gold_,
                iter->player_info_.ci_.id_);

            ar_db.exec_query(sqlstr);

            iter->clear_game_temp_values();
        }
    }
}

void game_room::reset_game_data(void)
{
    this->room_state_ = room_state_idle;
    this->cur_ready_player_count_ = 0;
    this->donate_item_serial_num_ = GIFT_SERIAL_BASE_NUM;
    this->donate_vip_item_serial_num_ = GIFT_VIP_SERIAL_BASE_NUM;
    this->game_timer_.cancel();

    scoped_wlock<thread_rwlock> guard(this->widget_tab_rwlock_);
    this->widget_tab_.clear();
}

int game_room::get_player_info_list(player_info infos[6])
{
    int i = 0;
    for(int j = 0; j < MAX_PLAYER_NUM_A_ROOM; ++j)
    {
        player_message_handler* iter = this->player_grp_[i];
        if(iter != nullptr) 
        {
            infos[i++] = iter->player_info_;
        }
    }
    return i;
}

int game_room::calc_player_game_results(game_end_information& result)
{
    int i = 0;
    int32_t gold = 0;
    player_message_handler* best_player = nullptr;
    int best_player_index = 0;
    for(int j = 0; j < MAX_PLAYER_NUM_A_ROOM; ++j)
    {
        player_message_handler* iter = this->player_grp_[i];
        if(iter != nullptr) 
        {
            iter->get_game_result(result.player_game_results_[i]);
            if(gold < iter->round_of_game_gold_)
            {
                best_player_index = i;
                gold = iter->round_of_game_gold_;
                result.best_player_id_ = iter->player_info_.ci_.id_;
                best_player = iter;
            }
            ++i;
        }
    }

    if(best_player != nullptr)
    {
        result.best_player_gold_award_ = ar_server_config.gci_.best_player_award_base_ * (this->cur_player_count_ - 1);
        result.best_player_exp_award_ = ar_server_config.gci_.best_player_award_base_ * (this->cur_player_count_ - 1);
        best_player->round_of_game_gold_ += result.best_player_gold_award_;
        best_player->round_of_game_exp_ += result.best_player_exp_award_;
        result.player_game_results_[best_player_index].player_round_gold_ = best_player->round_of_game_gold_;
        result.player_game_results_[best_player_index].player_round_exp_ = best_player->round_of_game_exp_;
    }

    result.player_count_ = i;
    return i;
}

int game_room::enter(player_message_handler* conn)
{
    if(room_state_playing == this->room_state_ || this->cur_player_count_ >= MAX_PLAYER_NUM_A_ROOM)
    {
        return -1;
    }
 
    scoped_wlock<thread_rwlock> guard(this->player_grp_rwlock_);
    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        if(nullptr == this->player_grp_[i])
        {
            this->player_grp_[i] = conn;
            ++this->cur_player_count_;
            return i;
        }
        else if(this->player_grp_[i] == conn)
        {
            return i;
        }
    }
    
    return -1;
}

void game_room::exit(player_message_handler* const conn, int indexInRoom)
{
    this->player_grp_rwlock_.lock_exclusive();

    if(this->player_grp_[indexInRoom] != nullptr)
    {
        this->player_grp_[indexInRoom] = nullptr;
        --this->cur_player_count_;
    }
    if(0 == this->cur_player_count_)
    {
        this->reset_game_data();
    }

    this->player_grp_rwlock_.unlock_exclusive();
}

int game_room::get_current_player_count(void) const
{
    return this->cur_player_count_;
}

uint8_t game_room::get_room_state(void)const
{
    return this->room_state_;
}
void   game_room::set_room_state(uint8_t room_state_)
{
    this->room_state_ = room_state_;
}

bool  game_room::is_widget_exists(uint16_t widgetSerialNum) const
{
    scoped_rlock<thread_rwlock> guard(this->widget_tab_rwlock_);

    return this->widget_tab_.find(widgetSerialNum) != this->widget_tab_.end();
}

simple_widget_info* game_room::get_widget_info_by_sn(uint32_t widgetSerialNum)
{ // maybe need to lock
    scoped_rlock<thread_rwlock> guard(this->widget_tab_rwlock_);

    auto iter = this->widget_tab_.find(widgetSerialNum);
    if(iter != this->widget_tab_.end())
    {
        return &(iter->second);
    }
    return nullptr;
}

void game_room::add_widget(uint16_t id, uint16_t x, uint32_t sn, uint32_t holder, bool is_vip)
{
    simple_widget_info swi;
    swi.id_ = id;
    swi.x_ = x;
    swi.holder_ = holder;
    swi.serial_number_ = sn;
    swi.is_vip_ = is_vip;
    this->widget_tab_.insert(std::make_pair(sn, swi));
}

void game_room::remove_widget(uint32_t widgetSerialNum)
{
    scoped_wlock<thread_rwlock> guard(this->widget_tab_rwlock_);

    this->widget_tab_.erase(widgetSerialNum);
}

static bool is_rect_circle_collision( float rw, float rh, float R, float rx, float ry)
{
	float dx = xxmin( rx, rw * 0.5f);
	dx = xxmax( dx, -rw * 0.5f);

    float dy = xxmin( ry, rh * 0.5f);
	dy = xxmax( dy, -rh * 0.5f);

	return (rx - dx)*(rx - dx) + (ry - dy)*(ry - dy) <= R * R;
}

int game_room::get_effected_players(int collision_x, int collision_y, uint8_t issuer_iir, const widget_attr& wa_atk, bool is_vip, int& damage_count, item_valid_range_info infos[6])
{
    int valid_index = 0;
    for(int i = 0; i < MAX_PLAYER_NUM_A_ROOM; ++i)
    {
        if(nullptr != this->player_grp_[i] && issuer_iir != this->player_grp_[i]->player_info_.si_.index_in_room_) 
        {
            int rx = collision_x - (this->player_grp_[i]->player_info_.ci_.x_ + ROLE_WIDTH / 2);
            int ry = collision_y - (this->player_grp_[i]->player_info_.ci_.y_ + ROLE_HEIGHT / 2);
            
            if(is_rect_circle_collision(ROLE_WIDTH, ROLE_HEIGHT, (int)(wa_atk.affect_range_ * (1.0f + 0.5f * is_vip)), rx, ry))
            {
                infos[valid_index].player_id_ = this->player_grp_[i]->player_info_.ci_.id_;
                infos[valid_index].dodge_code_ = this->player_grp_[i]->calc_dodge_item_attack_code(wa_atk);
                infos[valid_index].injured_comfort_evalue_ = this->player_grp_[i]->add_evalue(ar_server_config.gci_.injured_comfort_evalue_ * this->player_grp_[i]->injured_times_);
                this->player_grp_[i]->try_clear_injured_times();
                damage_count += (dodge_code_null == infos[valid_index].dodge_code_);
                ++valid_index;
            }
        }
    }
    return valid_index;
}

bool game_room::has_player_and_not_full(void) const
{
    return this->cur_player_count_ > 0 && this->cur_player_count_ < MAX_PLAYER_NUM_A_ROOM;
}

bool game_room::empty(void) const
{
    return 0 == this->cur_player_count_;
}

void  game_room::change_ready_count(int value)
{
    scoped_wlock<thread_rwlock> guard(this->ready_count_rwlock_);
    this->cur_ready_player_count_ += value;
}

int game_room::get_ready_player_count(void) const
{
    scoped_rlock<thread_rwlock> shared_guard(this->ready_count_rwlock_);
    return this->cur_ready_player_count_;
}

void game_room::cancel_game_timer(void)
{
    this->game_timer_.cancel();
}

