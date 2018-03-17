#include "async_tcp_client.h"

using namespace purelib::inet;

bool decode_pdu_length(const char* data, size_t datalen, int& len)
{
    return true;
}

int main(int, char**)
{
    tcpcli->start_service();

    tcpcli->set_callbacks(decode_pdu_length,
        [](bool, int) {},
        [](int error, const char* errormsg) {},
        [](std::vector<char>&&) {},
        [](const vdcallback_t& callback) {callback(); });

    tcpcli->set_endpoint("172.31.238.193", "172.31.238.193", 8888);

    deadline_timer t;
    t.expires_from_now(std::chrono::microseconds(2000000));

    auto diff = t.wait_duration();

    t.async_wait([](bool) {
        printf("timer is timeout!\n");
    });

    Sleep(5000);
    printf("start async connect request!\n");
    tcpcli->notify_connect();

    Sleep(1000000);

    return 0;
}
