#include "game_room.h"
#include "game_widget_table.h"
#include "xml3c_api.h"

using namespace exx;

/*
* widget_attr construction function
*/
static widget_attr make_widget_attr(
    const std::string& name, 
    int32_t bounty_plus, 
    int32_t exp_plus,
    int occurs, 
    int32_t evalue_plus = 0,
    int32_t evalue_cost = 0,
    int32_t hit_gold_plus = 0,
    uint8_t damage_type = 0,
    uint16_t affect_range = 0,
    uint32_t effect_mask = 0,
    uint8_t affect_effect = 0,
    uint16_t duration = 0
    )
{
    widget_attr tmp;
    tmp.name_ = name;
    tmp.gold_plus_ = bounty_plus;
    tmp.exp_plus_ = exp_plus;
    tmp.hit_gold_plus_ = hit_gold_plus;
    tmp.damage_type_ = damage_type;
    tmp.affect_effect_ = affect_effect;
    tmp.affect_range_ = affect_range;
    tmp.duration_ = duration;
    tmp.evalue_plus_ = evalue_plus;
    tmp.evalue_cost_ = evalue_cost;
    tmp.occurs_ = occurs;
    tmp.effect_mask_ = effect_mask;
    return tmp;
}

void game_widget_table::init(void)
{
    xml3c::document xmlconf("server_config.xml");
    xmlconf.xforeach("/server_config/widgets/widget", std::bind(&game_widget_table::handle_config_loading,this,xml3c::placeholders::element));
    //xmlconf.xforeach("/server_config/widgets/widget", [this](const xml3c::element& elem) {
    //    this->widget_table_.insert(
    //        std::make_pair( elem.get_property_value<uint32_t>("id"),
    //        make_widget_attr(elem.get_property_value("name"),
    //        elem.get_property_value<int32_t>("gold_plus"),
    //        elem.get_property_value<int32_t>("exp_plus"),
    //        elem.get_property_value<int>("occurs"),
    //        elem.get_property_value<int32_t>("evalue_plus"),
    //        elem.get_property_value<int32_t>("evalue_cost"),
    //        elem.get_property_value<int32_t>("hit_gold_plus"),
    //        elem.get_property_value<uint8_t>("damage_type"),
    //        elem.get_property_value<uint16_t>("affect_range"),
    //        elem.get_property_value<uint32_t>("effect_mask"),
    //        elem.get_property_value<uint8_t>("affect_effect"),
    //        elem.get_property_value<uint16_t>("duration")
    //        ) // make_widget_attr
    //        ) // make_pair
    //        ); // insert
    //});

    xmlconf.close();

    // init item id array
    for(int i = 0; i < 9; ++i)
    {
        this->item_cost_info_list_[i].item_id_ = i + 128;
        this->item_cost_info_list_[i].item_evalue_cost_ = this->widget_table_.find(this->item_cost_info_list_[i].item_id_)->second.evalue_cost_;
    }

    // init Sum Prob Table
    for(std::map<uint32_t, widget_attr>::iterator pr = this->widget_table_.begin(); pr != this->widget_table_.end(); ++pr)
    {   
        for(int i = 0; i < pr->second.occurs_; ++i)
        {   
            this->widget_sum_prob_list_.push_back(pr->first);
        }   
    }  
    //std::for_each(this->widget_table_.cbegin(), this->widget_table_.cend(), [this](const std::pair<uint32_t, widget_attr>& pr) {
    //    for(int i = 0; i < pr.second.occurs_; ++i)
    //    {
    //        this->widget_sum_prob_list_.push_back(pr.first);
    //    }
    //});
}

void game_widget_table::handle_config_loading(const xml3c::element& elem)
{
    this->widget_table_.insert(
        std::make_pair( elem.get_property_value<uint32_t>("id"),
        make_widget_attr(elem.get_property_value("name"),
        elem.get_property_value<int32_t>("gold_plus"),
        elem.get_property_value<int32_t>("exp_plus"),
        elem.get_property_value<int>("occurs"),
        elem.get_property_value<int32_t>("evalue_plus"),
        elem.get_property_value<int32_t>("evalue_cost"),
        elem.get_property_value<int32_t>("hit_gold_plus"),
        elem.get_property_value<uint8_t>("damage_type"),
        elem.get_property_value<uint16_t>("affect_range"),
        elem.get_property_value<uint32_t>("effect_mask"),
        elem.get_property_value<uint8_t>("affect_effect"),
        elem.get_property_value<uint16_t>("duration")
        ) // make_widget_attr
        ) // make_pair
        ); // insert
}

game_widget_table::~game_widget_table(void)
{
    this->widget_table_.clear();
}

const item_cost_info* game_widget_table::get_item_info_list(void) const
{
    return this->item_cost_info_list_;
}

uint16_t game_widget_table::get_random_widget_id(void) const
{
    return this->widget_sum_prob_list_[ next_rand_int() % this->widget_sum_prob_list_.size() ];
}

const item_cost_info* game_widget_table::get_item_cost_info_list(void) const
{
    return &this->item_cost_info_list_[0];
}

bool game_widget_table::is_known_widget(uint16_t widgetId) const
{
    return this->widget_table_.find(widgetId) != this->widget_table_.end();
}

bool game_widget_table::is_item(uint16_t widgetId) const
{
    auto iter = this->widget_table_.find(widgetId);
    if(iter != this->widget_table_.end())
    {
        return iter->second.gold_plus_ == 0;
    }
    return false;
}

bool game_widget_table::is_item(const widget_attr& wa) const
{
    return 0 == wa.gold_plus_;
}

int game_widget_table::get_widget_bounty(uint16_t widgetId) const
{
    auto iter = this->widget_table_.find(widgetId);
    if(iter != this->widget_table_.end())
    {
        return iter->second.gold_plus_;
    }
    return 0;
}

const widget_attr& game_widget_table::get_widget_attr_by_id(uint16_t widget_id) const
{
    return this->widget_table_.find(widget_id)->second;
}

void game_widget_table::spawn_widgets(int widget_count, game_room& room, drop_widget_info* wlst)
{
    for(int i = 0;i < widget_count;i++)
    {
        wlst[i].widget_id_ = this->get_random_widget_id();
        wlst[i].widget_x_ = next_rand_int() % 30 * 23 + 30;
        wlst[i].widget_serial_number_ = i;
        room.add_widget(wlst[i].widget_id_, wlst[i].widget_x_, wlst[i].widget_serial_number_);
    }
}

