#include "async_tcp_client.h"
#pragma comment(lib, R"(../../src/c-ares/vc/cares/lib-debug/libcaresd.lib)")

using namespace purelib::inet;

bool decode_pdu_length(const char* data, size_t datalen, int& len)
{
    return true;
}

int main(int, char**)
{
    // resolve domain:www.kernel.org succeed, ip:2604:1380:40a0:500::1:443
    // AI_V4MAPPED format is ---> ::ffff:255.255.255.255
    std::vector<ip::endpoint> ipList;
    bool succeed = xxsocket::resolve_v6(ipList, "www.baidu.com", 80);
    if (!succeed) {
        succeed = xxsocket::resolve_v4to6(ipList, "www.baidu.com", 80);
    }

    ipList.clear();
    xxsocket::resolve(ipList, "www.kernel.org");
    ipList.clear();
    xxsocket::resolve_v4(ipList, "www.kernel.org");
    ipList.clear();
    xxsocket::resolve_v6(ipList, "www.kernel.org");
    ipList.clear();
    xxsocket::resolve_v4to6(ipList, "www.kernel.org");

    ipList.clear();
    xxsocket::force_resolve_v6(ipList, "www.kernel.org");
    /*ipList;
    xxsocket::resolve(ipList, "www.kernel.org");*/

   /* auto test = ipList[0].to_string();
    auto test2 = ipList[1].to_string();*/
    purelib::inet::channel_endpoint endpoints[] = {
        //{ "172.31.238.193", "172.31.238.193", 8888 },
        { "www.baidu.com", 443 },
        // { "www.baidu.com", "www.baidu.com", 443 },
        //{ "www.tencent.com", "www.x-studio365.com", 443 },
    };
    tcpcli->start_service(endpoints, _ARRAYSIZE(endpoints));

    tcpcli->set_callbacks(decode_pdu_length,
        [](size_t, bool, int) {},
        [](size_t, int error, const char* errormsg) {printf("The connection is lost %d, %s", error, errormsg); },
        [](std::vector<char>&&) {},
        [](const vdcallback_t& callback) {callback(); });

    deadline_timer t(*tcpcli);
    t.expires_from_now(std::chrono::microseconds(std::chrono::seconds(10)), true);

    t.async_wait([](bool) {
        printf("timer is timeout 20 seconds!\n");
    });

    Sleep(1000);
    tcpcli->async_connect(0);
    // tcpcli->async_connect(1);

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
