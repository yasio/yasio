#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "masio.h"

#if defined(_WIN32)
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#define strcasestr StrStrIA
#endif

using namespace purelib::inet;

template <size_t _Size>
void append_string(std::vector<char> &packet, const char (&message)[_Size]) {
  packet.insert(packet.end(), message, message + _Size - 1);
}

bool resolv(std::vector<ip::endpoint> &endpoints, const char *hostname,
 unsigned short port) {
  return myasio->resolve(endpoints, hostname, port);
}

int main(int, char **) {
  purelib::inet::io_hostent endpoints[] = {
      {"203.162.71.67", 80},  // http client
      {"www.ip138.com", 80}   //  { "www.ip138.com", 80 },  // http client
  };

  myasio->set_option(MASIO_OPT_TCP_KEEPALIVE, 60, 30, 3);
  myasio->set_option(MASIO_OPT_RESOLV_FUNCTION, resolv);
  myasio->start_service(endpoints, _ARRAYSIZE(endpoints));

  deadline_timer t0(*myasio);

  t0.expires_from_now(std::chrono::seconds(3));
  t0.async_wait([](bool) {  // called at network thread
    printf("the timer is expired\n");
  });

  std::vector<std::shared_ptr<io_transport>> transports;

  myasio->set_callbacks(
      [](void *ud, int datalen) {  // decode pdu length func
        auto data = (char*)ud;
        int len = 0;
        if (datalen >= 4 && data[datalen - 1] == '\n' &&
            data[datalen - 2] == '\r' && data[datalen - 3] == '\n' &&
            data[datalen - 4] == '\r') {
          len = datalen;
        } else {
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
                    len = bodylen + static_cast<int>(ptr - data);
                  }
                }
              }
            }
          }
        }
        return len;
      },
      [&](event_ptr event) {
        switch (event->type()) {
          case MASIO_EVENT_RECV_PACKET: {
            auto packet = event->take_packet();
            packet.push_back('\0');
            printf("receive data:%s", packet.data());
            break;
          }
          case MASIO_EVENT_CONNECT_RESPONSE:
            if (event->error_code() == 0) {
              auto transport = event->transport();
              std::vector<char> packet;
              append_string(packet, "GET /index.htm HTTP/1.1\r\n");

              if (transport->channel_index() == 0)
                append_string(packet, "Host: 203.162.71.67\r\n");
              else
                append_string(packet, "Host: www.ip138.com\r\n");

              append_string(packet,
                            "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                            "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/51.0.2704.106 Safari/537.36\r\n");
              append_string(packet, "Accept: */*;q=0.8\r\n");
              append_string(packet, "Connection: Close\r\n\r\n");

              transports.push_back(transport);

              myasio->write(transport, std::move(packet));
              // myasio->close(transport);
              /*std::shared_ptr<deadline_timer> delayOneFrame(
                  new deadline_timer(*myasio));
              delayOneFrame->expires_from_now(std::chrono::milliseconds(10));
              delayOneFrame->async_wait(
                  [delayOneFrame, transport](bool cancelled) {
                    if (!cancelled) {
                      myasio->close(transport);
                    }
                  });*/
            }
            break;
          case MASIO_EVENT_CONNECTION_LOST:
            printf("The connection is lost(user end)!\n");
            break;
        }
      });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  myasio->open(0);
  myasio->open(1);

  time_t duration = 0;
  while (true) {
    myasio->dispatch_events();
    if (duration >= 60000) {
      for (auto transport : transports) myasio->close(transport);
      // myasio->close(1);
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  std::this_thread::sleep_for(std::chrono::seconds(60));

  return 0;
}
