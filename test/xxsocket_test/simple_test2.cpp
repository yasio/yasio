#include "async_socket_io.h"
#pragma comment(lib, R"(../../src/c-ares/vc/cares/lib-debug/libcaresd.lib)")

using namespace purelib::inet;

bool decode_pdu_length(const char* data, size_t datalen, int& len)
{
    /*if (datalen >= 4 &&
        data[datalen - 1] == '\n' &&
        data[datalen - 2] == '\r' &&
        data[datalen - 3] == '\n' &&
        data[datalen - 4] == '\r')
    {
        len = datalen;
    }*/
    if (datalen > 0) len = static_cast<int>(datalen);
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
        "www.x-studio365.com", 80,
        // {"192.168.0.102", 9596},
    { "0.0.0.0", 56981 }, // p2p acceptor
    { "0.0.0.0", 0 }, // p2p connector
                      //{ "172.31.238.193", "172.31.238.193", 8888 },
                      /*{ "www.baidu.com", 443 },*/
                      // { "www.baidu.com", "www.baidu.com", 443 },
                      //{ "www.tencent.com", "www.x-studio365.com", 443 },
    };
    myasio->start_service(endpoints, _ARRAYSIZE(endpoints));

    deadline_timer t0(*myasio);

    t0.expires_from_now(std::chrono::seconds(3));
    t0.async_wait([](bool) {
        //myasio->open_udp(1);
    });

    myasio->set_callbacks(decode_pdu_length,
        [&](size_t index, bool succeed, int) {
        if (succeed) {
            printf("[index: %u] connect succeed.\n", index);
            //std::vector<char> packet;
            //write_http(packet, "GET /ip.php HTTP/1.1\r\n");
            //write_http(packet, "Host: www.x-studio365.com\r\n");
            //write_http(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n");
            //write_http(packet, "Accept: */*;q=0.8\r\n");
            //write_http(packet, "Connection: Keep-Alive\r\n\r\n");

            //myasio->write(std::move(packet), 1);
        }
        else {
            printf("[index: %u] connect failed!\n", index);
        }
    },
        [](size_t, int error, const char* errormsg) {printf("The connection is lost %d, %s", error, errormsg); },
        [&](std::vector<char>&& d) {


        d.push_back('\0');
        printf("response data:%s\n", d.data());
        /* if (d.size() >= 28) {
        ip::endpoint* peer =(ip::endpoint*) d.data();
        tcpcli->open_p2p(1, 0);
        }*/
        std::vector<char> packet;
        write_http(packet, "GET /ip.php HTTP/1.1\r\n");
        write_http(packet, "Host: www.x-studio365.com\r\n");
        write_http(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.106 Safari/537.36\r\n");
        write_http(packet, "Accept: */*;q=0.8\r\n");
        write_http(packet, "Connection: Keep-Alive\r\n\r\n");

        myasio->write(std::move(packet), 1);
    },
        [](const vdcallback_t& callback) {callback(); });

    Sleep(1000);
    myasio->open(1, CHANNEL_UDP_SERVER);
#if 0

    Sleep(1000);
    printf("connecting server...\n");
    tcpcli->async_connect(0);

    unsigned int port = 0;
    // printf("please input peer port:");

    scanf("%u", &port);
    printf("connecting peer:%s:%u...\n", "218.17.70.162", port);
    tcpcli->set_endpoint(2, "218.17.70.162", port);
    tcpcli->async_connect(2);
    // tcpcli->async_connect(1);

    /*
    printf("start async connect request!\n");
    tcpcli->async_connect();*/
    //tcpcli->async_connect(1);
    //tcpcli->async_connect(2);
    // tcpcli->async_connect();
#endif
    while (true)
    {
        Sleep(50);
        myasio->dispatch_received_pdu();
    }

    Sleep(5000000);

    return 0;
}
