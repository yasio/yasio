#ifndef _RIPP_FRAME_H_
#define _RIPP_FRAME_H_

#ifdef _WIN32
#include <simple++/basic_window.h>
#include <simple++/epp_window.h>
#include <simple++/controls.h>
#include <simple++/platform.h>
#include <simple++/thread_basic.h>
#include <simple++/nsconv.h>
#include <simple++/ripple_api.h>

class ripple_frame : public exx::gui::epp_window
{
public:
    ripple_frame(HBITMAP hBitmap, int freq = 25, basic_window* parent = nullptr);

protected:
    void _Init_image(int freq);

   // void _OnLButtonDown(const pretty::gui::event_arg&); 

   // void _OnLButtonUp(const pretty::gui::event_arg&); 

    //void _OnMouseMove(const pretty::gui::event_arg&);

    //void _OnPaint(const pretty::gui::event_arg&);

   // void _OnQuit(const pretty::gui::event_arg&);

private:
    bool isLbtnDown;
    POINT lbtnDownPoint;
    HBITMAP bmp_handle;
    RIPPLE_OBJECT ripp_obj;
};

#endif

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

