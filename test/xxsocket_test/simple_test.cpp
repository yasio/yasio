
#include "async_socket_io.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#define strcasestr StrStrIA
#endif

using namespace purelib::inet;

template <size_t _Size>
void append_string(std::vector<char> &packet, const char(&message)[_Size]) {
    packet.insert(packet.end(), message, message + _Size - 1);
}

int main(int, char **) {

    purelib::inet::channel_endpoint endpoints[] = {
        { "www.ip138.com", 80 },  // http client
        {"0.0.0.0", 56981}, // tcp server
    };
    myasio->start_service(endpoints, _ARRAYSIZE(endpoints));

    deadline_timer t0(*myasio);

    t0.expires_from_now(std::chrono::seconds(3));
    t0.async_wait([](bool) { // called at network thread
        printf("the timer is expired\n");
    });

    std::vector<std::shared_ptr<channel_transport>> transports;

    myasio->set_callbacks(
        [](char *data, size_t datalen, int &len) { // decode pdu length func
        if (datalen >= 4 && data[datalen - 1] == '\n' &&
            data[datalen - 2] == '\r' && data[datalen - 3] == '\n' &&
            data[datalen - 4] == '\r') {
            len = datalen;
        }
        else {
            data[datalen] = '\0';
            auto ptr = strcasestr(data, "Content-Length:");

            if (ptr != nullptr) {
                ptr += (sizeof("Content-Length:") - 1);
                if (static_cast<int>(ptr - data) < static_cast<int>(datalen)) {
                    while (static_cast<int>(ptr - data) < static_cast<int>(datalen) &&
                        !isdigit(*ptr))
                        ++ptr;
                    if (isdigit(*ptr)) {
                        int bodylen = static_cast<int>(strtol(ptr, nullptr, 10));
                        if (bodylen > 0) {
                            ptr = strstr(ptr, "\r\n\r\n");
                            if (ptr != nullptr) {
                                ptr += (sizeof("\r\n\r\n") - 1);
                                len = bodylen + (ptr - data);
                            }
                        }
                    }
                }
            }
        }
        return true;
    },
        [&](size_t, std::shared_ptr<channel_transport> transport,
            int ec) { // connect response callback
        if (ec == 0) {
            // printf("[index: %zu] connect succeed.\n", index);
            std::vector<char> packet;
            append_string(packet, "GET /index.htm HTTP/1.1\r\n");
            append_string(packet, "Host: www.ip138.com\r\n");
            append_string(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                "Chrome/51.0.2704.106 Safari/537.36\r\n");
            append_string(packet, "Accept: */*;q=0.8\r\n");
            append_string(packet, "Connection: Close\r\n\r\n");

            transports.push_back(transport);

            myasio->write(transport, std::move(packet));
        }
        else {
            // printf("[index: %zu] connect failed!\n");
        }
    },
        [&](std::shared_ptr<channel_transport> transport) { // on connection lost
    },
        [&](std::vector<char> &&packet) { // on receive packet
        packet.push_back('\0');
        printf("receive data:%s", packet.data());
    },
        [](const vdcallback_t &callback) { // thread safe call
        callback();
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    myasio->open(0, CHANNEL_TCP_CLIENT);
    myasio->open(1, CHANNEL_TCP_SERVER);

    time_t duration = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        myasio->dispatch_packets();
        duration += 50;
        if (duration >= 10000) {
            for (auto transport : transports)
                myasio->close(transport);
            myasio->close(1);
            break;
        }
    }

    std::this_thread::sleep_for(std::chrono::seconds(60));

    return 0;
}
