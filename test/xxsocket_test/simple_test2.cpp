#include "async_tcp_client.h"
#pragma comment(lib, R"(../../src/c-ares/vc/cares/lib-debug/libcaresd.lib)")

using namespace purelib::inet;

bool decode_pdu_length(const char* /*data*/, size_t datalen, int& len)
{
    if (datalen >= 28)
    {
        len = 28;
    }
    return true;
}

template<size_t _Size>
void write_http(std::vector<char>& packet, const char(&message)[_Size])
{
    packet.insert(packet.end(), message, message + _Size - 1);
}

int main(int, char**)
{
    /*ipList;
    xxsocket::resolve(ipList, "www.kernel.org");*/

    /* auto test = ipList[0].to_string();
    auto test2 = ipList[1].to_string();*/
    purelib::inet::channel_endpoint endpoints[] = {
        "www.tencent.com", 443,
        // {"192.168.0.102", 9596},
    { "0.0.0.0", 0 }, // p2p acceptor
    { "0.0.0.0", 0 }, // p2p connector
                      //{ "172.31.238.193", "172.31.238.193", 8888 },
                      /*{ "www.baidu.com", 443 },*/
                      // { "www.baidu.com", "www.baidu.com", 443 },
                      //{ "www.tencent.com", "www.x-studio365.com", 443 },
    };
    tcpcli->start_service(endpoints, _ARRAYSIZE(endpoints));

    deadline_timer t0(*tcpcli);

    tcpcli->set_callbacks(decode_pdu_length,
        [&](size_t index, bool succeed, int) {
        if (succeed) {
            printf("[index: %u] connect succeed.\n", index);
            tcpcli->open_p2p(1, 0);
        }
        else {
            printf("[index: %u] connect failed!\n", index);
        }
    },
        [](size_t, int error, const char* errormsg) {printf("The connection is lost %d, %s", error, errormsg); },
        [&](std::vector<char>&& d) {

        /* if (d.size() >= 28) {
        ip::endpoint* peer =(ip::endpoint*) d.data();
        tcpcli->open_p2p(1, 0);
        }*/

    },
        [](const vdcallback_t& callback) {callback(); });


    Sleep(1000);
    printf("connecting server...\n");
    tcpcli->async_connect(0);

    unsigned int port = 0;
    char buffer[128];
    // printf("please input peer port:");

    scanf("%u", &port);
    printf("connecting peer:%s:%u...\n", "111.221.136.41", port);
    tcpcli->set_endpoint(2, "111.221.136.41", port);
    tcpcli->async_connect(2);
    // tcpcli->async_connect(1);

    /*
    printf("start async connect request!\n");
    tcpcli->async_connect();*/
    //tcpcli->async_connect(1);
    //tcpcli->async_connect(2);
    // tcpcli->async_connect();

    while (true)
    {
        Sleep(50);
        tcpcli->dispatch_received_pdu();
    }

    Sleep(5000000);

    return 0;
}
