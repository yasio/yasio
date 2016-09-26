/***************************************************************
* xxpiedef.h - Common definations
*                Copyright (c) pholang.com. All rights reserved.
*
* Author: xml3see
*
* Purpose: This file contains Some common definations, and includes files.
*
* Code Compatibility: Support windows or linux, can be compiled as
*                     32 bit or 64 bit  APP.
*
* 3rd libraries: boost_1_52_0, mysql-connector-c-6.0.2, libxml2-2.9.0
*
****************************************************************/
#ifndef _XXPIEDEF_H_
#define _XXPIEDEF_H_
#include <time.h>
#include <cstring>
#include <new>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <iomanip>
#define _ENABLE_MULTITHREAD
#include <simple++/xxsocket.h>
#include <simple++/thread_basic.h>
#include <simple++/thread_synch.h>
#include <simple++/nsconv.h>
//#include <simple++/threadpool.h>
#include <simple++/container_helper.h>
#include <simple++/unreal_string.h>
#include <simple++/object_pool.h>
#include <simple++/singleton.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ngx_palloc.h"

static const int MAX_IOBUF_SIZE  = 1024;  
static const int PLAYER_MSG_TYPE_SIZE = 1;
static const int PLAYER_MSG_LEN_SIZE = 2;
static const int PLAYER_MSG_HDR_SIZE = PLAYER_MSG_TYPE_SIZE + PLAYER_MSG_LEN_SIZE; // 1 bytes size and 2 bytes length
static const int MIN_RECV_DATA_INTERVAL = 30;

// max player number every room
static const int MAX_PLAYER_NUM_A_ROOM = 5;

// total hall number
static const int TOTAL_HALL_NUM = 5;

// ui room number every page
static const int UI_ROOM_NUM_A_PAGE = 18;

// ui page number every hall
static const int UI_PAGE_NUM_A_HALL = 5;

// total room number every hall
static const int TOTAL_ROOM_NUM_A_HALL = UI_ROOM_NUM_A_PAGE * UI_PAGE_NUM_A_HALL;

// total room number
static const int TOTAL_ROOM_NUM = TOTAL_ROOM_NUM_A_HALL * TOTAL_HALL_NUM;

// max player number in a hall
static const int MAX_PLAYER_NUM_A_HALL = TOTAL_ROOM_NUM_A_HALL * MAX_PLAYER_NUM_A_ROOM;

// max player number a server
static const int MAX_TOTAL_PALYER_NUM = MAX_PLAYER_NUM_A_ROOM * TOTAL_ROOM_NUM;

static const int MAX_CONN_COUNT = 10000;

static const int IPV4_MAX_LEN = sizeof("255.255.255.255:65535");

static const int MAX_PLAYER_LEVEL = 100;

#define ROLE_HEIGHT  102
#define ROLE_WIDTH   54

using boost::asio::ip::tcp;
using namespace exx::asy;
using namespace exx::net;

#include "xxpie_logger.h"

/// classes fwd
// network
class player_server;
class player_session;
class player_session_manager;

// game
class game_room;
class game_hall;
class game_hall_manager;

class player_message_handler;

#define IS_VALID_HALL_NUM(hall_num) ( (hall_num) > 0 && (hall_num) <= TOTAL_HALL_NUM )
#define IS_VALID_PAGE_NUM(page_num) ( (page_num) > 0 && (page_num) <= UI_PAGE_NUM_A_HALL)
#define IS_VALID_ROOM_NUM(room_num) ( (room_num) > 0 && (room_num) <= TOTAL_ROOM_NUM_A_HALL)
#define IS_VALID_ROOM_NUM2(room_num) ( (room_num) >= 0 && (room_num) <= TOTAL_ROOM_NUM_A_HALL)
#define IS_VALID_INDEX_IN_ROOM(index_in_room) ( (index_in_room) >= 0 && (index_in_room) < MAX_PLAYER_NUM_A_ROOM)
#define IS_VALID_PLAYER_NR(player_num) IS_VALID_INDEX_IN_ROOM(player_num)
#define TO_SVR_RGN_VAL(value) ( (value) - 1 )
#define TO_NET_RGN_VAL(value) ( (value) + 1 )
#define INVALID_PLAYER_ID_VALUE  0xffffffff
#define IS_VIP_ITEM(serial_number) ( (uint32_t)(serial_number) >= 0x90000000 )

#define EMASK_NULL                0x00
#define EMASK_CHESTSNUT_SPEED_UP  0x01
#define EMASK_INVINCIBLE          0x02
#define EMASK_CHESTSNUT_SLOW_DOWN 0x04
#define EMASK_FELL_DIZZY          0x08
#define EMASK_EQUIP_BONES         0x10
#define EMASK_EQUIP_SHOES         0x20
#define EMASK_EQUIP_BODY_ARMOR    0x40
#define EMASK_EQUIP_SACHETS       0x80


typedef enum  {
    damage_type_null, // 非伤害物品
    damage_type_single, // single damage(单体伤害)
    damage_type_group // group damage(群体伤害)
} damage_type;

typedef enum {
    item_use_rc_succeed,
    item_use_rc_gold_not_enough,
    item_use_rc_not_exist,
    item_use_rc_illegal_widget // item not belong issuer
} item_use_retcode;

typedef enum {
    room_state_idle,
    room_state_playing
} room_state;

typedef enum {
    effect_status_added,
    effect_status_disappearing,
    effect_status_dispappered
} player_effect_status;

typedef enum {
    dodge_code_null,
    dodge_code_by_equip,
    dodge_code_by_probability
} player_dodge_code;

struct widget_attr {
    std::string             name_;
    int32_t                 gold_plus_;
    int32_t                 exp_plus_;
    int32_t                 evalue_plus_;
    int32_t                 hit_gold_plus_;
    uint8_t                 damage_type_;
    uint8_t                 affect_effect_;
    uint16_t                affect_range_;
    uint16_t                duration_;
    int32_t                 evalue_cost_;
    uint32_t                effect_mask_;
    int                     occurs_;
};

/// follow structures or codes are defined by manually
/** Save player got widget sequently times
*/
struct seq_got_widget_info
{
    uint16_t widget_id_;
    uint32_t got_times_;
};

struct level_exp_pr
{
    int level_;
    int level_initial_exp_;
    int levelup_total_need_exp_;
};

#define get_general_arithmetic_progression(a1,d,n) ( (a1) + ((n) - 1) * (d) )

#endif

