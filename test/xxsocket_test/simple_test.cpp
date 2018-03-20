#include "async_tcp_client.h"

using namespace purelib::inet;

bool decode_pdu_length(const char* data, size_t datalen, int& len)
{
    return true;
}

int main(int, char**)
{
    purelib::inet::channel_endpoint endpoints[] = {
        { "172.31.238.193", "172.31.238.193", 8888 },
        { "www.baidu.com", "www.baidu.com", 443 },
        //{ "www.tencent.com", "www.x-studio365.com", 443 },
    };
    tcpcli->start_service(endpoints, _ARRAYSIZE(endpoints));

    tcpcli->set_callbacks(decode_pdu_length,
        [](size_t, bool, int) {},
        [](size_t, int error, const char* errormsg) {printf("The connection is lost %d, %s", error, errormsg); },
        [](std::vector<char>&&) {},
        [](const vdcallback_t& callback) {callback(); });

    deadline_timer t(*tcpcli);
    t.expires_from_now(std::chrono::microseconds(std::chrono::seconds(20)), true);

    t.async_wait([](bool) {
        printf("start async connect request!\n");
    });

    Sleep(1000);
    tcpcli->async_connect();

    Sleep(5000);
    /*
    printf("start async connect request!\n");
    tcpcli->async_connect();*/
    //tcpcli->async_connect(1);
    //tcpcli->async_connect(2);
    // tcpcli->async_connect();
    Sleep(500000);

    return 0;
}
