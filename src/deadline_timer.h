//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
// version: 3.0
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2018 halx99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _XXSOCKET_DEADLINE_TIMER_H_
#define _XXSOCKET_DEADLINE_TIMER_H_
#include <chrono>
#include <functional>

namespace purelib
{
namespace inet {
class async_socket_io;

typedef std::chrono::high_resolution_clock highp_clock_t;
typedef long long highp_time_t;

class deadline_timer {
public:
    ~deadline_timer();
    deadline_timer(async_socket_io& service) : repeated_(false), service_(service)
    {
    }

    void expires_from_now(const std::chrono::microseconds& duration, bool repeated = false)
    {
        this->duration_ = duration;
        this->repeated_ = repeated;
        expire_time_ = highp_clock_t::now() + this->duration_;
    }

    void expires_from_now()
    {
        expire_time_ = highp_clock_t::now() + this->duration_;
    }

    // Wait timer timeout or cancelled.
    void async_wait(const std::function<void(bool cancelled)>& callback);

    // Cancel the timer
    void cancel();

    // Check if timer is expired?
    bool expired() const
    {
        return wait_duration().count() <= 0;
    }

    // Let timer expire immidlately
    void expire()
    {
        expire_time_ = highp_clock_t::now() - duration_;
    }

    // Gets wait duration of timer.
    std::chrono::microseconds wait_duration() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(expire_time_ - highp_clock_t::now());
    }

    bool repeated_;
    async_socket_io& service_;
    std::chrono::microseconds duration_;
    std::chrono::time_point<highp_clock_t> expire_time_;
    std::function<void(bool cancelled)> callback_;
};
}
}

#endif
