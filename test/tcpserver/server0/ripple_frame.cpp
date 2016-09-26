#ifdef _WIN32
#include <simple++/enable_visual_styles.h>
#include "ripple_frame.h"

using namespace exx::gui;

#define _USING_YACHT

ripple_frame::ripple_frame(HBITMAP bmp_handle, int freq, basic_window* parent) : bmp_handle(bmp_handle), epp_window()
{
    this->isLbtnDown = false;
    if(parent != nullptr)
    {
        this->append_style(WS_CHILD);
        SetParent(this->get_handle(), parent->get_handle());
        this->update();
        this->show();
    }

    this->_Init_image(freq);

	/*this->register_event(WM_LBUTTONDOWN, [](simplepp::gui::event_sender& obj, const simplepp::gui::event_arg& arg)->void{
		    dynamic_cast<ripple_frame&>(obj).lbtnDownPoint.x = arg.larg.ms_coord.x;
            dynamic_cast<ripple_frame&>(obj).lbtnDownPoint.y = arg.larg.ms_coord.y;
	});*/

    this->register_event(WM_LBUTTONDOWN, [this](simplepp::gui::event_sender& obj, const simplepp::gui::event_arg& arg)->void {
		/*RECT rect;
		::GetWindowRect(obj.get_handle(), &rect);
		if(arg.warg.value == MK_LBUTTON)
		{
			dynamic_cast<ripple_frame&>(obj).set_position(rect.left + (arg.larg.ms_coord.x - dynamic_cast<ripple_frame&>(obj).lbtnDownPoint.x),
				rect.top + (arg.larg.ms_coord.y - dynamic_cast<ripple_frame&>(obj).lbtnDownPoint.y));
		}*/

		//if(this->isLbtnDown)
		//{
		//    this->set_position(rect.left + (arg.larg.ms_coord.x - this->lbtnDownPoint.x),
		//        rect.top + (arg.larg.ms_coord.y - this->lbtnDownPoint.y));
		//}
        ::ThrowStone(&this->ripp_obj, 
            arg.larg.ms_coord.x + 5, 
            arg.larg.ms_coord.y + 10,
		    5,
		    5,
		    50);
	});

	this->register_event(WM_PAINT, [](simplepp::gui::event_sender& obj, const simplepp::gui::event_arg& arg)->void{
		::UpdateRippleFrame(&dynamic_cast<ripple_frame&>(obj).ripp_obj, dynamic_cast<ripple_frame&>(obj).get_handle(), TRUE);
	});  
}

void ripple_frame::_Init_image(int freq)
{   
    ::InitRippleObject(&this->ripp_obj, 
        this->get_handle(), 
        this->bmp_handle, 
        freq);
    
    this->set_bounds(0, 0, this->ripp_obj.iBmpWidth, this->ripp_obj.iBmpHeight);

#if defined(_USING_NONE)
    // No Special Effect
    ::EnableRippleEffect(&this->ripp_obj,
        T_EFF_NONE,
        0,
        0,
        0,
        0);
#elif defined(_USING_RAIN)
    // Rain
    ::EnableRippleEffect(&this->ripp_obj,
        T_EFF_RAIN,
        0,
        0,
        1,
        250);
#elif defined(_USING_WIND)
    // Wind
    ::EnableRippleEffect(&this->ripp_obj,
        T_EFF_WIND,
        80,
        1,
        1,
        5);
#elif defined(_USING_YACHT)
    // Yacht
    ::EnableRippleEffect(&this->ripp_obj,
        T_EFF_YACHT,
        7,
        2,
        3,
        50);
#else
#error error, the macro must be _USING_NONE, _USING_RAIN, _USING_WIND and _USING_YACHT
#endif
}


#endif


