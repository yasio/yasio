#include "async_socket_io.h"
#pragma comment(lib, R"(../../src/c-ares/vc/cares/lib-debug/libcaresd.lib)")

using namespace purelib::inet;

bool decode_pdu_length(const char* /*data*/, size_t datalen, int& len)
{ // UDP ONLY
    if (datalen > 0)
    {
        len = datalen;
    }
    return true;
}

template<size_t _Size>
void write_string(std::vector<char>& packet, const char(&message)[_Size])
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
      //  "www.tencent.com", 443,
        // {"192.168.0.102", 9596},
    { "192.168.0.102", 51219 }, // p2p acceptor
    { "0.0.0.0", 0 }, // p2p connector
                      //{ "172.31.238.193", "172.31.238.193", 8888 },
                      /*{ "www.baidu.com", 443 },*/
                      // { "www.baidu.com", "www.baidu.com", 443 },
                      //{ "www.tencent.com", "www.x-studio365.com", 443 },
    };
    myasio->start_service(endpoints, _ARRAYSIZE(endpoints));

    deadline_timer t0(*myasio);

    myasio->set_callbacks(decode_pdu_length,
        [&](size_t index, bool succeed, int) {
        if (succeed) {
            printf("[index: %u] connect succeed.\n", index);
        }
        else {
            printf("[index: %u] connect failed!\n", index);
        }
    },
        [](size_t, int error, const char* errormsg) {printf("The connection is lost %d, %s", error, errormsg); },
        [&](std::vector<char>&& d) {

        d.push_back('\0');
        printf("received message:%s\n", d.data());
    },
        [](const vdcallback_t& callback) {callback(); });


    Sleep(1000);
    printf("connecting server...\n");
    myasio->open(0, CHANNEL_UDP_SERVER);

    Sleep(3000);
    std::vector<char> packet;
    write_string(packet, "hello udp server!");
    // myasio->async_send(std::move(packet));

    while (true)
    {
        Sleep(50);
        myasio->dispatch_received_pdu();
    }

    Sleep(5000000);

    return 0;
}
