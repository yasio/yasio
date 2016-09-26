#include "player_session.h"
#include "game_room.h"
#include "game_hall.h"
#include "game_hall_manager.h"
#include "game_widget_table.h"
#include "server_config.h"
#include "protocol_contants.h"
#include "protocol_messages.h"
#include "mysql3c.h"
#include "auto_resources.h"
using namespace messages;
using namespace constants;
using namespace auto_resources;

#include "player_message_handler.h"

#define SEND_MSG(msg)                            \
{                                                \
        int real_len = 0;                        \
        char net_data[sizeof(msg)];              \
        msg.encode(net_data, real_len);          \
        this->session_.send(net_data, real_len); \
}

#define SEND_MSG_WITH_LEN(msg,real_len)          \
{                                                \
        char net_data[sizeof(msg)];              \
        msg.encode(net_data, real_len);          \
        this->session_.send(net_data, real_len); \
}

#define BROADCAST_MSG(msg)                       \
{                                                \
        int real_len = 0;                        \
        char net_data[sizeof(msg)];              \
        msg.encode(net_data, real_len);          \
        this->room_->broadcast(this, net_data, real_len); \
}


#define BROADCAST_MSG_WITH_LEN(msg,real_len)     \
{                                                \
        char net_data[sizeof(msg)];              \
        msg.encode(net_data, real_len);          \
        this->room_->broadcast(this, net_data, real_len); \
}

#define BROADCAST_MSG_TOALL(msg)                 \
{                                                \
        int real_len = 0;                        \
        char net_data[sizeof(msg)];              \
        msg.encode(net_data, real_len);          \
        this->room_->broadcast_to_all(net_data, real_len); \
}

#define BROADCAST_MSG_TOALL_WITH_LEN(msg,real_len) \
{                                                  \
        char net_data[sizeof(msg)];                \
        msg.encode(net_data, real_len);            \
        this->room_->broadcast_to_all(net_data, real_len); \
}

player_message_handler::player_message_handler(player_session& connRef, game_hall_manager& hallManager) : session_(connRef), hall_mgr_(hallManager)
{ 
}

player_message_handler::~player_message_handler(void)
{
    // clear temp data
    memset(this, 0x00, sizeof(*this));
}

void player_message_handler::on_connection(void)
{
    this->hall_ = nullptr;
    this->room_ = nullptr;
    this->hall_number_ = 0xff;
    this->player_info_.si_.index_in_room_ = 0xff;
    this->player_info_.si_.level_ = 1;
    this->seq_got_wi_.widget_id_ = 0xff;
    this->anti_addiction_factor_ = 1.0f;
    this->update_data_based_on_level();
    this->injured_times_ = 0;
    this->proto_state_ = CONNECTED;
    this->heartbeat_timestamp_ = time(nullptr);
}

bool player_message_handler::handle_room_list_refresh_req(const char* buffer, int length)
{
    player_room_list_refresh_request req;
    player_room_list_refresh_response resp;
    if(this->proto_state_ == IN_A_HALL)
    {
        //message decoding

        req.decode(buffer, length);

        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

        //message handling and constructing
        if(IS_VALID_PAGE_NUM(req.page_num_))
        {
            int roomNumber = (req.page_num_ - 1) * 18;
            game_room* room;
            for(int i = 0 ;i < 18 ;i++ )
            {//获取当前大厅当前页面的房间列表信息
                room = this->hall_->locate_room(roomNumber++);
                resp.room_infos_[i].room_num_ = roomNumber;
                resp.room_infos_[i].player_count_ = room->get_current_player_count();
                resp.room_infos_[i].room_status_ = room->get_room_state();
            }

            resp.retcode_ = 0;
            resp.hall_num_ = TO_NET_RGN_VAL(this->hall_number_);
            resp.page_num_ = req.page_num_;

            //message encoding and sending
            char net_data[sizeof(resp)];
            int len = 0;
            resp.encode(net_data, len);
            this->session_.send(net_data, len);

            PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());
            return true;
        }
        else
        {
            LOG_ERROR("Error PageNum  from %s !\n", this->session_.get_addr_info());

            resp.retcode_ = 1;
            resp.hall_num_ = TO_NET_RGN_VAL(this->hall_number_);
            resp.page_num_ = req.page_num_;
            //message encoding and sending
            char net_data[sizeof(resp)];
            int len = 0;
            resp.encode(net_data, len);
            this->session_.send(net_data, len);
            PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());
            return true;
        } 
    }
    else
    {
        LOG_ERROR("handle_room_list_refresh_req: this player from %s is not in a hall, real proto state(%d)\n", this->session_.get_addr_info(), this->proto_state_);

        resp.retcode_ = 1;
        //res.body.hall_number_ = this->getHallId();
        resp.hall_num_ = -1;
        resp.page_num_ = req.page_num_;
        //message encoding and sending
        char net_data[sizeof(resp)];
        int len = 0;
        resp.encode(net_data, len);
        this->session_.send(net_data,len);
        PROTO_LOG(resp.get_formated_string().c_str(),"Server", this->session_.get_addr_info());
        return true;
    }
}

bool player_message_handler::handle_view_online_count_req(const char* buffer, int length)
{
    if(this->proto_state_ == IN_A_HALL)
    {
        player_view_online_count_request req;
        req.decode(buffer, length);
        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

        player_view_online_count_response resp;

        resp.count_of_hall_ = this->hall_mgr_.get_hall_player_count(this->hall_number_);
        resp.count_of_total_ = this->hall_mgr_.get_total_player_count();

        int sz = 0;
        char data[sizeof(resp)];
        resp.encode(data, sz);
        this->session_.send(data, sz);

        PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());  
        return true;
    }

    LOG_ERROR("except player connection, the player is not in a hall, proto state(%d), detail: %s \n", this->proto_state_, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_enter_room_req(const char* buffer, int length)
{
    proto_state_t state = this->proto_state_;
    uint8_t req_room_number = *(buffer + PLAYER_MSG_HDR_SIZE);
    uint8_t real_room_number = req_room_number - 1;
    uint8_t index_in_room = -1;
    if( ( state == IN_A_HALL || state == IN_A_ROOM ) 
        && IS_VALID_ROOM_NUM2(req_room_number) )
    {
        if(req_room_number != 0)
        {
            index_in_room = this->enter_room(TO_SVR_RGN_VAL(req_room_number)); 
        }
        else // quick enter or quick handover
        {
            if(IN_A_ROOM == state)
            {
                this->notify_exit_room(egr_quickly_handover, "Server(Quick Handover)");
            }
            real_room_number = this->quick_enter_room(index_in_room);
        }

        player_enter_room_response resp;
        player_enter_room_request req;
        player_enter_room_information info;
        resp.hall_number_ = TO_NET_RGN_VAL(this->hall_number_);

        if( IS_VALID_INDEX_IN_ROOM(index_in_room) )
        {
            // saving player state
            this->proto_state_ = IN_A_ROOM;
            
            // save new player information
            req.decode(buffer, length);
            PROTO_LOG(req.get_formated_string().c_str(), (std::string("Player") + this->session_.get_addr_info()).c_str(), "Server");
            DUMP_HEX("PLAYER ENTER ROOM REQUEST:", buffer, length);

            this->player_info_.ci_ = req.player_info_from_client_;
            this->player_info_.si_.index_in_room_ = index_in_room;
            //req.player_info_.index_in_room_ = index_in_room;
            //this->player_info_ = //req.player_info_; 

            // init player other attr
            this->dodge_ = this->player_info_.si_.level_ * 5;
            this->strength_ = this->player_info_.si_.level_;

            this->cycling_append_histroy_room();

            // Get other player info, and send to requestor
            resp.room_number_ = TO_NET_RGN_VAL(real_room_number);
            resp.player_count_ = this->room_->get_player_info_list(resp.all_player_infos_);
            resp.retcode_ = 0;

            // 6 is the mandatory fields total size, should define a constant.
            // here just alloc enough stack memory
            char resp_data[sizeof(resp)]; 
            int enc_len = 0;
            resp.encode(resp_data, enc_len);

            this->session_.send(resp_data, enc_len);
            PROTO_LOG(resp.get_formated_string().c_str(), "Server", (std::string("Player") + this->session_.get_addr_info()).c_str() );
            DUMP_HEX("PLAYER ENTER ROOM RESPONSE:", resp_data, enc_len);

            if(resp.player_count_ > 1)
            {
                // send this player information to others           
                info.new_player_info_ = this->player_info_;
                char info_data[ sizeof(info) ];
                int infoLen = 0;
                info.encode(info_data, infoLen);

                this->room_->broadcast(this, info_data, infoLen);

                PROTO_LOG(info.get_formated_string().c_str(),  "Server", "All Other Players");            
            }
        } 
        else
        { // enter room failed
            int len = 0;
            resp.room_number_ = req_room_number;
            resp.player_count_ = 0;
            char resp_data[sizeof(resp)];
            resp.retcode_ = 1;
            resp.encode(resp_data, len);	

            this->session_.send(resp_data, len);	
            PROTO_LOG(resp.get_formated_string().c_str(),"Server", this->session_.get_addr_info());
        }
        return true;        
    }

    /// invalid player
    LOG_ERROR("handle_enter_room_req: player state invalid(%d, %d or %d needed) or room number error(%d,should be 1~90), detail: %s.\n", this->proto_state_, IN_A_HALL, IN_A_ROOM, req_room_number, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_move_req(const char* buffer, int length)
{
    /*static const char const_resp_data[3] = {PLAYER_MOVE_RESPONSE_MT_VALUE, 0x00, 0x00};*/
    player_move_request req;
    /*player_move_information info;*/
    DUMP_HEX("PLAYER MOVE REQUEST:", buffer, length);

    if(IN_A_ROOM == this->proto_state_) {
        // message decoding
        req.decode(buffer, length);
        PROTO_LOG(req.get_formated_string().c_str(), (std::string("Player") + this->session_.get_addr_info()).c_str(), "Server");

        // send this player move information to others  
        /*info.player_id_ = this->player_info_.ci_.id_;*/
        this->player_info_.ci_.x_ = req.new_x_;
        this->player_info_.ci_.y_ = req.new_y_;
        this->player_info_.ci_.move_type_ = req.move_type_;
        this->player_info_.ci_.direction_ = req.direction_;

        *(uint8_t*)const_cast<char*>(buffer) = PLAYER_MOVE_INFORMATION_MT_VALUE;
        this->room_->broadcast(this, buffer, length);

        //PROTO_LOG(info.get_formated_string().c_str(), "Server", "All others players");
    }

    return true;
}

bool player_message_handler::handle_use_item_req(const char* buffer, int length)
{
    player_use_item_request req;
    player_use_item_information info;

    if(IN_A_ROOM == this->proto_state_) {
        //message decoding and  handling
        req.decode(buffer,length);

        // TODO: log
        PROTO_LOG(req.get_formated_string().c_str(), (std::string("Player") + this->session_.get_addr_info()).c_str(), "Server");

        info.item_issuer_iir_ = req.item_issuer_iir_;
        info.item_widget_id_ = req.item_widget_id_;
        info.item_serial_number_ = req.item_serial_number_;
        info.item_issue_x_ = req.item_issue_x_;
        info.item_issue_y_ = req.item_issue_y_;
        info.item_issue_direction_ = req.item_issue_direction_;
        info.item_issuer_reality_evalue_ = this->player_info_.si_.evalue_;
        simple_widget_info* pswi = this->room_->get_widget_info_by_sn(req.item_serial_number_);
        if(pswi != nullptr)
        {
            if(pswi->holder_ == this->player_info_.ci_.id_) { // 防止客户端作弊偷取他人道具
                const widget_attr& wa = ar_game_widget_table.get_widget_attr_by_id(req.item_widget_id_);
                int32_t reality_evalue_cost = wa.evalue_cost_;

                if(IS_VIP_ITEM(req.item_serial_number_))
                {
                    reality_evalue_cost = reality_evalue_cost * ar_server_config.sci_.vip_cost_rebate_ / 100;
                }

                if(this->player_info_.si_.evalue_ >= reality_evalue_cost)
                {
                    this->player_info_.si_.evalue_ -= reality_evalue_cost;
                    if(damage_type_null == wa.damage_type_)
                    {
                        this->player_effect_mask_ |= wa.effect_mask_;
                    }
                    info.use_item_retcode_ = item_use_rc_succeed;
                    info.item_issuer_evalue_cost_ = reality_evalue_cost;
                    info.item_issuer_reality_evalue_ = this->player_info_.si_.evalue_;
                }
                else
                {
                    info.use_item_retcode_ = item_use_rc_gold_not_enough;
                }
            }
            else {
                info.use_item_retcode_ = item_use_rc_illegal_widget;
            }
        }
        else
        {
            LOG_ERROR("use item exception,detail --> [user: player id: %u, item serial number: %u]\n", this->player_info_.ci_.id_, req.item_serial_number_);
            info.use_item_retcode_ = item_use_rc_not_exist;
        }

        //messsage coding and sending
        int len = 0;
        char info_data[sizeof(info)];
        info.encode(info_data, len);
        this->room_->broadcast_to_all(info_data, len);

        // TODO: log
        PROTO_LOG(info.get_formated_string().c_str(),  "Server", "All Other Players"); 

        return true;
    }
    LOG_ERROR("handle_use_item_req, invalid proto state(%d, %d needed), detail: %s\n", this->proto_state_, IN_A_ROOM, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_issuing_item_collision_notify(const char* buffer, int length)
{
    issuing_item_collision_info req;
    player_use_item_result info;

    if(IN_A_ROOM == this->proto_state_) {
        req.decode(buffer,length);

        // TODO: log
        PROTO_LOG(req.get_formated_string().c_str(), (std::string("Player") + this->session_.get_addr_info()).c_str(), "Server");

        if( 0 < req.item_collision_angle_ && req.item_collision_angle_ < MAX_VALID_ANGLE)
        {
            info.item_collision_angle_ = req.item_collision_angle_;
        }
        else
        {
            //TODO:
            info.item_collision_angle_ = 0;
        }

        info.item_issuer_iir_ = req.item_issuer_iir_;
        info.item_hitter_iir_ = req.item_hitter_iir_;
        info.item_widget_id_ = req.item_widget_id_;
        info.item_serial_number_ = req.item_serial_number_;
        info.item_collision_x_ = req.item_collision_x_;
        info.item_collision_y_ = req.item_collision_y_;
        info.item_collision_angle_ = req.item_collision_angle_;
        simple_widget_info* pswi = this->room_->get_widget_info_by_sn(req.item_serial_number_);
        if(pswi != nullptr) {
            const widget_attr& wa = ar_game_widget_table.get_widget_attr_by_id(req.item_widget_id_);
            int damage_count = 0;
            if(damage_type_group == wa.damage_type_)
            {
                info.item_valid_range_player_count_ = this->room_->get_effected_players(info.item_collision_x_,
                    info.item_collision_y_,
                    info.item_issuer_iir_,
                    wa,
                    pswi->is_vip_,
                    damage_count,
                    info.item_valid_range_players_);
            }
            else
            {
                if(IS_VALID_INDEX_IN_ROOM(info.item_hitter_iir_)) 
                {
                    info.item_valid_range_player_count_ = 1;
                    info.item_valid_range_players_[0].player_id_ = this->player_info_.ci_.id_;
                    info.item_valid_range_players_[0].dodge_code_ = this->calc_dodge_item_attack_code(wa);
                    damage_count = (dodge_code_null == info.item_valid_range_players_[0].dodge_code_);
                }
            }

            // hit gold plus for item issuer
            player_message_handler* item_issuer = nullptr;
            if(IS_VALID_INDEX_IN_ROOM(req.item_issuer_iir_) && 
                (item_issuer = this->room_->player_grp_[req.item_issuer_iir_]) != nullptr
                && req.item_hitter_iir_ != req.item_issuer_iir_)
            {
                info.item_issuer_gold_bounty_ = wa.hit_gold_plus_ * damage_count * this->anti_addiction_factor_;
                info.item_issuer_cur_round_gold_ = (item_issuer->round_of_game_gold_ += info.item_issuer_gold_bounty_);
                info.item_issuer_cur_total_gold_ = (item_issuer->player_info_.si_.total_gold_ += info.item_issuer_gold_bounty_);
            }

            //LOG_TRACE("remove a normal widget(a item dispappered by used by player), serial number: %u\n", req.item_serial_number_);
            this->room_->remove_widget(req.item_serial_number_);

            // messsage coding and sending
            int len = 0;
            char info_data[sizeof(info)];
            info.encode(info_data, len);
            this->room_->broadcast_to_all(info_data, len);

            // TODO: log
            PROTO_LOG(info.get_formated_string().c_str(), "Server", "All Other Players"); 
        }
        return true;
    }
    LOG_ERROR("handle_issuing_item_collision_notify, invalid proto state(%d, %d needed), detail: %s\n", this->proto_state_, IN_A_ROOM, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_widget_collision_req(const char* buffer, int length)
{
    widget_collision_request req;
    widget_collision_information info;

    if(IN_A_ROOM == this->proto_state_) {
        req.decode(buffer, length);

        PROTO_LOG(req.get_formated_string().c_str(), (std::string("Player") + this->session_.get_addr_info()).c_str(), "Server");

        info.widget_id_ = req.widget_id_;
        info.widget_serial_number_ = req.widget_serial_number_;
        info.widget_x_ = req.widget_x_;
        info.widget_y_ = req.widget_y_;
        info.widget_collision_player_ = req.widget_collision_player_;

        info.wcp_cur_evalue_ = this->player_info_.si_.evalue_;
        info.wcp_evalue_toplimit_ = this->player_info_.si_.evalue_toplimit_;

        info.wcp_cur_total_gold_ = this->player_info_.si_.total_gold_;
        info.wcp_cur_round_gold_ = this->round_of_game_gold_;
        info.wcp_gold_capacity_ =  this->player_info_.si_.gold_capacity_;

        info.wcp_level_cur_exp_ = this->player_info_.si_.level_cur_exp_;
        info.wcp_levelup_total_exp_ = this->player_info_.si_.levelup_total_exp_;
        info.wcp_cur_round_exp_ = this->round_of_game_exp_;

        info.wcp_evalue_plus_ = 0;
        info.wcp_exp_plus_ = 0;
        info.wcp_gold_plus_ = 0;

        simple_widget_info* wi = this->room_->get_widget_info_by_sn(info.widget_serial_number_);
        info.succeed_ = (wi != nullptr);

        if(info.succeed_)
        {
            const widget_attr& wa = ar_game_widget_table.get_widget_attr_by_id(wi->id_);

            if(ar_game_widget_table.is_item(wa))
            { // A item, check if already hold by somebody.
                uint32_t old_holder = wi->holder_;
                atomic_cmpxchg32(wi->holder_, req.widget_collision_player_, INVALID_PLAYER_ID_VALUE);
                info.succeed_ = (wi->holder_ != old_holder);
            }
            else { // A normal widget, firstly, remove it from the widget table immdiately
                //LOG_TRACE("remove a normal widget which fall to the ground, detail[widget_id:%d,serial_number:%d]\n",wi->id_, wi->serial_number_);
                this->room_->remove_widget(wi->serial_number_);

                // if request is a valid player, calculate take bounty for him/her.
                if(info.widget_collision_player_ != INVALID_PLAYER_ID_VALUE)
                {
                    // exp:
                    info.wcp_cur_round_exp_ = ( this->round_of_game_exp_ += (info.wcp_exp_plus_ = (wa.exp_plus_ * this->anti_addiction_factor_)) );

                    // evalue:
                    info.wcp_evalue_plus_ = this->add_evalue(wa.evalue_plus_ * this->anti_addiction_factor_);
                    info.wcp_cur_evalue_ += info.wcp_evalue_plus_;

                    // gold:
                    info.wcp_gold_plus_ = this->calc_gold_award(req.widget_id_, wa) * this->anti_addiction_factor_;
                    info.wcp_cur_round_gold_ = (this->round_of_game_gold_ += info.wcp_gold_plus_);
                    info.wcp_cur_total_gold_ = (this->player_info_.si_.total_gold_ += info.wcp_gold_plus_);
                    info.seq_got_same_widget_times_ = this->seq_got_wi_.got_times_;

                } // else do nothing
                else {
                    info.succeed_ = false;
                }
            }
        }

        int len = 0;
        char info_data[sizeof(info)];
        info.encode(info_data, len);
        this->room_->broadcast_to_all(info_data, len);

        PROTO_LOG(info.get_formated_string().c_str(), "Server", (std::string("Player") + this->session_.get_addr_info()).c_str());

        return true;
    }
    LOG_ERROR("handle_widget_collision_req: invalid proto state: %d(%d needed), detail: %s\n", this->proto_state_, IN_A_ROOM, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_exit_room_req(const char* buffer, int length)
{
    if(this->proto_state_ == IN_A_ROOM)
    {
        player_exit_room_request req;
        req.decode(buffer,length);

        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");
        DUMP_HEX("EXIT ROOM REQUEST:", buffer, length);

        this->notify_exit_room(egr_normally, "Server(Normally)");

        this->proto_state_ = IN_A_HALL;
        return true;   
    }
    else
    { // Just ignore request.
        LOG_ERROR("handle_exit_room_req: the player %s is not in a room, current proto_state: %d\n", this->session_.get_addr_info(), this->proto_state_);
        return true;
    }
}

bool player_message_handler::handle_change_ready_state_req(const char* buffer, int length)
{
    player_change_ready_request req;
    player_change_ready_response resp;
    player_change_ready_info info;

    
    if(this->proto_state_ == IN_A_ROOM)
    {
        // message decoding
        req.decode(buffer,length);

        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

        resp.retcode_ = 1;
        info.new_ready_state_ = resp.new_ready_state_ = req.new_ready_state_; 
        info.player_id_ = this->player_info_.ci_.id_;

        if(req.new_ready_state_ != this->player_info_.ci_.ready_state_)
        {
            this->player_info_.ci_.ready_state_ = req.new_ready_state_;
            if(player_state_ready == req.new_ready_state_ || player_state_playing == req.new_ready_state_)
            {
                this->room_->change_ready_count(1);
            }
            else {
                this->room_->change_ready_count(-1);
            }
            resp.retcode_ = 0;
        }

        int len = 0;

        SEND_MSG_WITH_LEN(resp, len);
        PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info()); 

        BROADCAST_MSG_WITH_LEN(info, len);
        PROTO_LOG(info.get_formated_string().c_str(),"Server","All Other Players"); 

        // to inform the user the game is starting
        
        int player_count = this->room_->get_current_player_count();
        int ready_count = this->room_->get_ready_player_count();
        bool is_start = (1 == player_count && player_state_playing == req.new_ready_state_) ||
            (player_count >= 2 && ready_count == player_count);

        if(is_start)
        {
            this->room_->start_game(); // start game timer
        }
        return true;
    }
    else
    {
        LOG_ERROR("handle_change_ready_state_req: the player from %s is not in a room, state: %d\n",this->session_.get_addr_info(), this->proto_state_);
        resp.retcode_ = 1;
        resp.new_ready_state_ = req.new_ready_state_ ;

        int len = 0;
        char resp_data[sizeof(resp)];
        resp.encode(resp_data,len);
        this->session_.send(resp_data, len);
        return true;
    }
}

bool player_message_handler::handle_hall_update_req(const char* buffer, int length)
{
    player_hall_update_request req;
    player_hall_update_response resp;

    req.decode(buffer,length);
    PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");
    DUMP_HEX("HALL UPDATE REQUEST:",buffer, length);

    switch(this->proto_state_)
    {
    case IN_A_HALL:
        this->exit_hall();
    case CONNECTED:
        {
            if((resp.succeed_ = this->enter_hall(TO_SVR_RGN_VAL(req.new_hall_id_))))
            {
                this->proto_state_ = IN_A_HALL;
            }
            else {
                LOG_ERROR("handle_hall_update_req: invalid hall number(must be 1~5): %d, session: %s\n", req.new_hall_id_, this->session_.get_addr_info());
                return false;
            }
        }
        break;
    default:
        LOG_ERROR("handle_hall_update_req: invalid protocol state: %d, current hall(number: %d, address: %#x), session: %s\n", this->proto_state_, this->hall_, this->hall_number_, this->session_.get_addr_info());
        return false;
    }

    int len = 0;
    char resp_data[sizeof(resp) + 9 * sizeof(item_cost_info)];
 
    resp.encode(resp_data, len);
    this->session_.send(resp_data, len);

    PROTO_LOG(resp.get_formated_string().c_str(),"Server", this->session_.get_addr_info());
    DUMP_HEX("HALL UPDATE RESPONSE:", resp_data, len);
    return true;
}

bool player_message_handler::handle_chat_message(const char* bstream, int length)
{
    if(IN_A_ROOM == this->proto_state_) {
        uint32_t receiver_playerid = ntohl(*(uint32_t*)(bstream + PLAYER_MSG_HDR_SIZE + sizeof(uint32_t)));
        player_chat_message detail_;
        detail_.decode(bstream, length);

        PROTO_LOG(detail_.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");
        DUMP_HEX("CHAT MESSAGE:", bstream, length);

        if(INVALID_PLAYER_ID_VALUE == receiver_playerid)
        { // broadcast chat
            this->room_->broadcast_to_all(bstream, length);
        }
        else
        { // private chat
            this->room_->private_send(receiver_playerid, bstream, length);
        }
        return true;
    }
    LOG_ERROR("handle_chat_message, invalid protocol state(%d, %d needed), detail: %s\n", this->proto_state_, IN_A_ROOM, this->session_.get_addr_info());
    return true;
}

bool player_message_handler::handle_heartbeat_req(const char* bstream, int length)
{
    this->heartbeat_timestamp_ = time(nullptr);

    player_heartbeat_request req;
    player_heartbeat_response resp;
    req.decode(bstream, length);

    PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(),"Server");

    resp.sequence_number_ = req.sequence_number_;

    char resp_data[sizeof(resp)];
    resp.encode(resp_data, length);

    PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());
    this->session_.send(resp_data, length);
    return true;
}

bool player_message_handler::handle_effect_change_req(const char* bstream, int length)
{
    if(IN_A_ROOM == this->proto_state_) {
        player_effect_change_request req;
        req.decode(bstream, length);
        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

        switch(req.effect_status_) 
        {
        case effect_status_added:
            this->player_effect_mask_ |= req.effect_mask_;
            break;
        case effect_status_dispappered:
            this->player_effect_mask_ &= ~req.effect_mask_;
            break;
        default:;
        }

        *(uint8_t*)const_cast<char*>(bstream) = PLAYER_EFFECT_CHANGE_INFO_MT_VALUE;
        this->room_->broadcast(this, bstream, length);
        return true;
    }
    return true;
}

bool player_message_handler::handle_new_player_logon_request(const char* bstream, int length)
{
    DUMP_HEX("NEW PLAYER LOGON REQUEST:", bstream, length);
    if(0 == this->player_info_.ci_.id_) {
        new_player_logon_request req;
        new_player_logon_response resp;
        req.decode(bstream, length);
        PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

        this->player_info_.ci_.id_ = req.player_id_;
        resp.heartbeat_interval_ = ar_server_config.heartbeat_interval_;
        resp.logon_retcode_ = !this->session_.trylogon();

        if(0 == resp.logon_retcode_ || 1 == resp.logon_retcode_) 
        {
            this->read_level_info(req.player_id_);
            this->read_service_info(req.player_id_);
            this->update_data_based_on_level();
            resp.item_use_cd_initial_ = ar_server_config.sci_.item_use_cd_initial_;
            resp.item_use_cd_min_ = ar_server_config.sci_.item_use_cd_min_;
            resp.item_use_cd_reduce_ = ar_server_config.sci_.item_use_cd_reduce_;
            resp.vip_item_cd_rebate_ = ar_server_config.sci_.vip_item_cd_rebate_;
            resp.vip_cost_rebate_ = ar_server_config.sci_.vip_cost_rebate_;
            resp.item_count_ = 9;
            resp.item_cost_infos_ = const_cast<item_cost_info*>(ar_game_widget_table.get_item_cost_info_list());
        }

        int real_len = 0;                        
        char net_data[sizeof(resp) + 9 * sizeof(item_cost_info)];              
        resp.encode(net_data, real_len);          
        this->session_.send(net_data, real_len); 
        PROTO_LOG(resp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());
        DUMP_HEX("NEW PLAYER LOGON RESPONSE:", net_data, real_len);
        return true;
    }
    return true;
}

bool player_message_handler::handle_later_player_logon_request(const char* bstream, int length)
{
    later_player_logon_request req;
    req.decode(bstream, length);
    PROTO_LOG(req.get_formated_string().c_str(), this->session_.get_addr_info(), "Server");

    player_session* old_session = nullptr;
    if( req.logon_forcibly_) 
    {
        static const unsigned char const_notify_to_older[] = {LATER_PLAYER_LOGGED_ON_NOTIFY_MT_VALUE, 0x00, 0x00};
        static const unsigned char const_notify_to_later[] = {LATER_PLAYER_RELOGON_RESPONSE_MT_VALUE, 0x00, 0x00};
        if((old_session = this->session_.relogon()) != nullptr)
        {
            old_session->send(const_notify_to_older, sizeof(const_notify_to_older) );
            old_session->kick("kicked by later player");
        }
        this->session_.send(const_notify_to_later, sizeof(const_notify_to_later));
    }
    else {
        this->session_.kick("kicked by previously player");
    }
    return true;
}

bool player_message_handler::on_message(const char* bstream, int length)
{
    uint8_t type = static_cast<uint8_t>(*bstream);
    auto handler = s_msg_handler_tab.find(type);
    if(handler != s_msg_handler_tab.end())
    {
        return (this->*(handler->second))(bstream, length);
    }
    else
    {
        LOG_ERROR("unknow message[MT:%d,TSOM:%d] received from %s, now, close it...\n", 
            type,
            length,
            this->session_.get_addr_info());
        return false;
    }
}

size_t  player_message_handler::decode_message_length(const char* bstream, int length)
{
    if(length >= PLAYER_MSG_HDR_SIZE)
    {
        return ntohs(*((uint16_t*)(bstream + 1))) + PLAYER_MSG_HDR_SIZE;
    }
    return 0;
}

void player_message_handler::on_disconnect(void)
{ // Abnormal exit
    if(this->proto_state_ == IN_A_ROOM)
    {
        this->notify_exit_room(egr_abnormal, "Server(Abnormal)");
    }
    
    this->exit_hall();
    this->proto_state_ = DISCONNECTED;
}

player_session& player_message_handler::get_session(void) const
{
    return this->session_;
}

bool player_message_handler::is_history_room(game_room* const room) const
{
    for(size_t i = 0; i < sizeof(this->history_rooms_) / sizeof(game_room*); ++i)
    {
        if(room == this->history_rooms_[i])
        {
            return true;
        }
    }
    return false;
}

void player_message_handler::get_game_result(player_game_result& game_result)
{
    this->calc_game_result(game_result.player_hall_mark_up_exp_, game_result.player_hall_mark_up_gold_);
    this->update_data_based_on_level();

    game_result.player_id_ = this->player_info_.ci_.id_;
    game_result.player_level_ = this->player_info_.si_.level_;
    game_result.player_level_cur_exp_ = this->player_info_.si_.level_cur_exp_;
    game_result.player_levelup_total_exp_ = this->player_info_.si_.levelup_total_exp_;
    game_result.player_round_exp_ = this->round_of_game_exp_;
    game_result.player_round_gold_ = this->round_of_game_gold_;
    game_result.player_total_gold_ = this->player_info_.si_.total_gold_;
    game_result.player_evalue_toplimit_ = this->player_info_.si_.evalue_toplimit_;
}

bool player_message_handler::is_evalue_full(void) const
{
    return this->player_info_.si_.evalue_ >= this->player_info_.si_.evalue_toplimit_;
}

void player_message_handler::try_clear_injured_times(void)
{
    if(this->player_info_.si_.evalue_ >= ar_server_config.gci_.initial_evalue_toplimit_)
    {
        this->injured_times_ = 0;
    }
}


bool player_message_handler::is_heartbeat_timeout(void) const
{
    return ( time(nullptr) - this->heartbeat_timestamp_ ) > ar_server_config.heartbeat_timeout_;
}

bool player_message_handler::enter_hall(uint8_t hall_number_)
{
    this->hall_ = this->hall_mgr_.locate_hall(hall_number_);
    if(this->hall_ != nullptr) {
        this->hall_->count_player(1);
        this->hall_number_ = hall_number_;
        return true;
    }
    return false;
}

void player_message_handler::exit_hall(void)
{
    if(this->hall_ != nullptr)
    {
        this->hall_->count_player(-1);
        this->proto_state_ = CONNECTED;
        this->hall_ = nullptr;
        this->hall_number_ = 0;
    }
}

int player_message_handler::enter_room(uint8_t roomNumber)
{
    if(this->hall_ != nullptr) 
    {
        this->room_ = this->hall_->locate_room(roomNumber);
        if(this->room_ != nullptr) 
        {
            return this->room_->enter(this);
        }
    }
    return -1;
}

uint8_t player_message_handler::quick_enter_room(uint8_t& index_in_room)
{
    uint8_t roomNumber = -1;
    if(this->hall_ != nullptr)
    { // search in this hall firstly
        this->room_ = this->hall_->search_room(roomNumber, *this);		
        if(this->room_ != nullptr) 
        {
            index_in_room = this->room_->enter(this);
            return roomNumber;
        }
    }
    for(int i = 0; i < TOTAL_HALL_NUM; ++i) 
    {
        if(this->hall_number_ != i)
        {
            game_hall* temp = this->hall_mgr_.locate_hall(i);
            this->room_ = temp->search_room(roomNumber, *this);
            if(this->room_ != nullptr)
            {
                this->hall_ = temp;
                this->hall_number_ = i;
                index_in_room = this->room_->enter(this);
                break;
            }
        }
    }
    return roomNumber;
}

void player_message_handler::exit_room(void)
{
    if(this->room_ != nullptr) {
        if(player_state_ready == this->player_info_.ci_.ready_state_ || 
            player_state_playing == this->player_info_.ci_.ready_state_)
        {
            this->room_->change_ready_count(-1);
        }
        
        //LOG_TRACE_ALL("the player: %s exiting.\n", this->session_.get_addr_info());
        this->room_->exit(this, this->player_info_.si_.index_in_room_);
        this->room_ = nullptr;
        this->clear_game_temp_values();
        this->player_info_.si_.index_in_room_ = 0xff;
    }
}

uint32_t player_message_handler::add_evalue(uint32_t post_evalue)
{
    if(this->player_info_.si_.evalue_ < this->player_info_.si_.evalue_toplimit_)
    {
        int d = this->player_info_.si_.evalue_toplimit_ - this->player_info_.si_.evalue_;
        int evalue_plus = post_evalue;
        if(d > evalue_plus)
        {
            d = evalue_plus;
        }
        this->player_info_.si_.evalue_ += d;
        return d;
    }
    return 0;
}

void player_message_handler::notify_exit_room(exit_game_reason reason, const char* log)
{
    (void)log;

    player_exit_room_information info;

    info.player_id_ = this->player_info_.ci_.id_;
    info.reason_ = (uint8_t)reason;

    int real_len = 0;                        
    char net_data[sizeof(info)];             
    info.encode(net_data, real_len);
    this->room_->broadcast_to_all(net_data, real_len);
    game_room* old_room = this->room_;
    this->exit_room();
    this->proto_state_ = IN_A_HALL;

    int room_player_count = old_room->get_current_player_count();
    int room_ready_count = old_room->get_ready_player_count();
    if(room_player_count > 0 && 
        room_ready_count >= room_player_count)
    {
        old_room->start_game();
    }

    PROTO_LOG(info.get_formated_string().c_str(), log, this->session_.get_addr_info() );
}

void player_message_handler::cycling_append_histroy_room(void)
{
    this->history_rooms_[this->history_room_index_] = this->room_;
    ++this->history_room_index_;
    if(this->history_room_index_ >= sizeof(this->history_rooms_) / sizeof(game_room*))
    {
        this->history_room_index_ = 0;
    }
}

// 等差数列通项公式g(n) = g(1) + ( n - 1) * 2
int player_message_handler::calc_gold_award(uint16_t widget_id, const widget_attr& attr)
{
    if(attr.gold_plus_ == 0) // item
    {
        this->seq_got_wi_.widget_id_ = widget_id;
        this->seq_got_wi_.got_times_ = 1;
        return attr.gold_plus_;
    }
    
    if(this->seq_got_wi_.widget_id_ != widget_id)
    {
        this->seq_got_wi_.widget_id_ = widget_id;
        this->seq_got_wi_.got_times_ = 1;
    }
    else {
        ++this->seq_got_wi_.got_times_;
    }
    if(attr.gold_plus_ > 0) {
        uint32_t max_times = (1000 - attr.gold_plus_);
        if(this->seq_got_wi_.got_times_ > max_times)
        {
            this->seq_got_wi_.got_times_ = max_times;
        }
        return (attr.gold_plus_ + this->seq_got_wi_.got_times_ - 1);
    }
    else {
    	  return (attr.gold_plus_ - this->seq_got_wi_.got_times_ + 1);
    }
}

void player_message_handler::calc_game_result(uint32_t& exp_mark_up, int32_t& gold_mark_up)
{
    // calculate mark up
    exp_mark_up = this->round_of_game_exp_ * (this->hall_number_ * ar_server_config.gci_.hall_mark_up_factor_);
    gold_mark_up = this->round_of_game_gold_ * (this->hall_number_ * ar_server_config.gci_.hall_mark_up_factor_);
    this->round_of_game_exp_ += exp_mark_up;
    this->round_of_game_gold_ += gold_mark_up;

    // upgrade level
    int total_exp = eval_exp_by_level(this->player_info_.si_.level_) + this->player_info_.si_.level_cur_exp_ + this->round_of_game_exp_;
    this->player_info_.si_.level_ = (int)eval_level_by_exp(total_exp);
    int new_level_initial_exp = eval_exp_by_level(this->player_info_.si_.level_);
    this->player_info_.si_.level_cur_exp_ = total_exp - new_level_initial_exp;
    this->player_info_.si_.levelup_total_exp_ = eval_exp_by_level(this->player_info_.si_.level_ + 1) - new_level_initial_exp;
    
    // update player info
    this->dodge_ = this->player_info_.si_.level_ * 5;
    this->strength_ = this->player_info_.si_.level_;

    // update highest exp record
    if(this->player_info_.si_.history_highest_round_exp_ < this->round_of_game_exp_)
    {
        this->player_info_.si_.history_highest_round_exp_ = this->round_of_game_exp_;
    }

	if(this->player_info_.si_.history_highest_round_gold_ < this->round_of_game_gold_)
	{
		this->player_info_.si_.history_highest_round_gold_ = this->round_of_game_gold_;
	}
}

void player_message_handler::update_data_based_on_level(void)
{
    this->player_info_.si_.evalue_toplimit_ = get_general_arithmetic_progression(ar_server_config.gci_.initial_evalue_toplimit_,ar_server_config.gci_.evalue_diff_,this->player_info_.si_.level_);
    this->player_info_.si_.gold_capacity_ = get_general_arithmetic_progression(ar_server_config.gci_.initial_gold_capacity_, ar_server_config.gci_.gold_capacity_diff_,this->player_info_.si_.level_);
    this->player_info_.si_.levelup_total_exp_ = eval_exp_by_level(this->player_info_.si_.level_ + 1) - eval_exp_by_level(this->player_info_.si_.level_);
}

void player_message_handler::clear_game_temp_values(void)
{
    this->round_of_game_exp_ = 0;
    this->round_of_game_gold_ = 0;
    this->player_info_.si_.evalue_ = 0;
    this->player_info_.ci_.ready_state_ = player_state_not_ready;
    this->player_effect_mask_ = EMASK_NULL;
    this->seq_got_wi_.got_times_ = 0;
    this->seq_got_wi_.widget_id_ = 0xff;
    this->injured_times_ = 0;
}

void player_message_handler::post_donate_info(void)
{
    if(this->room_ != nullptr) {
        item_donate_for_player idfp;
        idfp.donate_item_count_ = this->get_donate_item_count();

        int info_size = sizeof(donate_item_info) * idfp.donate_item_count_;
        idfp.donate_item_infos_ = (decltype(idfp.donate_item_infos_))mpool_alloc(info_size);

        int donate_index = 0;
        // vip donate
        for(uint8_t i = 0 ; i < this->player_info_.si_.service_count_ && i < sizeof(this->player_info_.si_.service_infos_) / sizeof(this->player_info_.si_.service_infos_[0]); ++i)
        {
            int vip_donate_count = this->count_of_donate_item(this->player_info_.si_.service_infos_[i].item_pay_type_);

            for(int j = 0; j < vip_donate_count; ++j)
            {
                idfp.donate_item_infos_[donate_index].item_widget_id_ = this->player_info_.si_.service_infos_[i].item_id_;
                idfp.donate_item_infos_[donate_index].item_serial_number = this->room_->donate_vip_item_serial_num_++;
                this->room_->add_widget(idfp.donate_item_infos_[donate_index].item_widget_id_, 
                    this->player_info_.ci_.x_, 
                    idfp.donate_item_infos_[donate_index].item_serial_number, 
                    this->player_info_.ci_.id_, true);
                ++donate_index;
            }
        }

        // normal donate
        const item_cost_info* ici = ar_game_widget_table.get_item_cost_info_list();
        for(int i = 0; i < 9; ++i)
        {
            for(int j = 0; j < ar_server_config.sci_.common_donate_info_.donate_count_; ++j)
            {
                idfp.donate_item_infos_[donate_index].item_widget_id_ = ici[i].item_id_;
                idfp.donate_item_infos_[donate_index].item_serial_number = this->room_->donate_item_serial_num_++;
                this->room_->add_widget(idfp.donate_item_infos_[donate_index].item_widget_id_, 
                    this->player_info_.ci_.x_, 
                    idfp.donate_item_infos_[donate_index].item_serial_number, 
                    this->player_info_.ci_.id_);
                ++donate_index;
            }
        }

        //assert(donate_index == idfp.donate_item_count_);

        int len = 0;
        char* net_data = (char*)mpool_alloc(sizeof(idfp) + info_size);
        idfp.encode(net_data, len);
        this->session_.send(net_data, len);

        PROTO_LOG(idfp.get_formated_string().c_str(), "Server", this->session_.get_addr_info());

        mpool_free(net_data);
        mpool_free(idfp.donate_item_infos_);
    }
}

int player_message_handler::get_donate_item_count(void)
{
    this->read_service_info(this->player_info_.ci_.id_);
    int donate_item_count_ = 9 * ar_server_config.sci_.common_donate_info_.donate_count_;
    for(uint8_t i = 0 ; i < this->player_info_.si_.service_count_ && i < sizeof(this->player_info_.si_.service_infos_) / sizeof(this->player_info_.si_.service_infos_[0]); ++i)
    {
        donate_item_count_ += this->count_of_donate_item(this->player_info_.si_.service_infos_[i].item_pay_type_);
    }
    return donate_item_count_;
}

int  player_message_handler::count_of_donate_item(uint32_t serviceId)
{
    switch(serviceId)
    {
    case WEEK:
        return ar_server_config.sci_.vip_donate_info_.week_donate_count_;
        break;
    case MONTH:
        return ar_server_config.sci_.vip_donate_info_.month_donate_count_;
        break;
    case YEAR:
        return ar_server_config.sci_.vip_donate_info_.year_donate_count_;
        break;
    default:;
    }
    return 0;
}

void player_message_handler::read_level_info(uint32_t player_id)
{
#define USR_EXP_SQL_PREFIX "select level,total_gold,total_exp,history_max_exp,history_max_gold,rate from user_exp where uid = "
    // read level info
    char sqlstr[128] = USR_EXP_SQL_PREFIX;
    sprintf(sqlstr + sizeof(USR_EXP_SQL_PREFIX) - 1, "%d", player_id);
    mysql3c::result_set result_set;
    if(ar_db.exec_query(sqlstr, &result_set) && result_set.is_valid())
    {
        char** record = result_set.fetch_row();
        if(record != nullptr) {
            this->player_info_.si_.level_ = atoi(record[0]);
            this->player_info_.si_.total_gold_ = atoi(record[1]);
            this->player_info_.si_.level_cur_exp_ = atoi(record[2]) - eval_exp_by_level(this->player_info_.si_.level_);
            this->player_info_.si_.history_highest_round_exp_ = atoi(record[3]);
			this->player_info_.si_.history_highest_round_gold_ = atoi(record[4]);
            this->player_info_.si_.levelup_total_exp_ = eval_exp_by_level(this->player_info_.si_.level_ + 1) - eval_exp_by_level(this->player_info_.si_.level_);
            this->anti_addiction_factor_ = atof(record[5]);
        }
    }
}

void  player_message_handler::read_service_info(uint32_t player_id)
{
#define USR_PROPS_SQL_PREFIX "select pid,buy_type,validtime from user_props where uid = "
    this->player_info_.si_.service_count_ = 0;
    memset(this->player_info_.si_.service_infos_, 0x00, sizeof(this->player_info_.si_.service_infos_));

    // read service info
    char sqlstr[128] = USR_PROPS_SQL_PREFIX;
    sprintf(sqlstr + sizeof(USR_PROPS_SQL_PREFIX) - 1, "%d", player_id);
    mysql3c::result_set result_set;
    if(ar_db.exec_query(sqlstr, &result_set) && result_set.is_valid())
    {
        size_t index = 0;
        char** record = nullptr;
        while((record = result_set.fetch_row()) != nullptr && index < sizeof(this->player_info_.si_.service_infos_) / sizeof(this->player_info_.si_.service_infos_[0]))
        {
            if(atoi(record[2]) > time(nullptr))
            {
                this->player_info_.si_.service_infos_[index].item_id_ = atoi(record[0]);
                this->player_info_.si_.service_infos_[index].item_pay_type_ = atoi(record[1]);
                ++index;
            }
        }
        this->player_info_.si_.service_count_ = index;
    }
}

uint8_t player_message_handler::calc_dodge_item_attack_code(const widget_attr& wa)
{
    int rand_value = next_rand_int();
    int probability_factor = rand_value % 1000;
    /*printf("probability_factor: %d， dodge: %d, effect mask: %d\n", probability_factor, this->dodge_, this->player_effect_mask_);*/
    if((probability_factor <= this->dodge_))
    {
        return dodge_code_by_probability;
    }
    if((this->player_effect_mask_ & wa.effect_mask_ ||
        this->player_effect_mask_ & EMASK_INVINCIBLE ||
        (this->player_effect_mask_ >> 5) & wa.effect_mask_ ))
    {
        return dodge_code_by_equip;
    }
    //this->player_effect_mask_ |= EMASK_INVINCIBLE;
    ++this->injured_times_;
    return dodge_code_null;
}

