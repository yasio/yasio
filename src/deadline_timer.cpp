#include "deadline_timer.h"
#include "async_tcp_client.h"

using namespace purelib::inet;

deadline_timer::~deadline_timer()
{
    tcpcli->cancel_timer(this);
}

void deadline_timer::async_wait(const std::function<void(bool cancelled)>& callback)
{
    this->callback_ = callback;
    tcpcli->schedule_timer(this);
}
