#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(YASIO_IOCP)
#  include "yasio/experimental/yasio_iocp.h"
#elif defined(YASIO_POLL)
#  include "yasio/experimental/yasio_poll.h"
#elif defined(YASIO_EPOLL)
#  include "yasio/experimental/yasio_epoll.h"
#else
#  include "yasio/experimental/yasio_kcp.h"
#endif
#include "yasio/ibstream.h"
#include "yasio/obstream.h"

#if defined(_WIN32)
#  include <Shlwapi.h>
#  pragma comment(lib, "shlwapi.lib")
#  define strcasestr StrStrIA
#endif

using namespace yasio::inet;

template <size_t _Size> void append_string(std::vector<char> &packet, const char (&message)[_Size])
{
  packet.insert(packet.end(), message, message + _Size - 1);
}

struct smallitem
{
  int a;
  int b;
  int c;
  int d;
};

void yasioTest()
{
  yasio::inet::io_hostent endpoints[] = {
      {"127.0.0.1", 30001}, // udp client1
      {"127.0.0.1", 30002}, // udp client2
      {"www.ip138.com", 80} // http client
  };

  io_service service;

  service.set_option(YOPT_TCP_KEEPALIVE, 60, 30, 3);

  resolv_fn_t resolv = [&](std::vector<ip::endpoint> &endpoints, const char *hostname,
                           unsigned short port) {
    return service.__builtin_resolv(endpoints, hostname, port);
  };
  service.set_option(YOPT_RESOLV_FUNCTION, &resolv);

  std::vector<transport_ptr> transports;
  service.set_option(YOPT_LFBFD_PARAMS, 16384, -1, 0, 0);
  service.set_option(YOPT_LOG_FILE, "yasio.log");

  deadline_timer udp_msg_delay(service);
  service.start_service(endpoints, _ARRAYSIZE(endpoints), [&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET:
      {
        auto packet = std::move(event->packet());
        packet.push_back('\0');
        printf("index:%d, receive data:%s\n", event->transport()->channel_index(), packet.data());

        if (event->cindex() == 1)
        { // response udp client

          std::vector<char> packet_resp;
          append_string(packet_resp, "hello udp client 0\n");
          printf("---- response a packet to udp client 0\n");
          service.write(event->transport(), packet_resp);
        }
        else
        {
          auto transport = event->transport();
          service.schedule(std::chrono::seconds(3), [&service, transport](bool) {
            std::vector<char> packet_resp;
            append_string(packet_resp, "hello udp client 1\n");
            printf("---- send a packet to udp client 1\n");
            service.write(transport, packet_resp);
          });
        }
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          if (event->cindex() == 1)
          {
            std::vector<char> packet;
            append_string(packet, "GET /index.htm HTTP/1.1\r\n");

            append_string(packet, "Host: www.ip138.com\r\n");

            append_string(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                                  "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                                  "Chrome/51.0.2704.106 Safari/537.36\r\n");
            append_string(packet, "Accept: */*;q=0.8\r\n");
            append_string(packet, "Connection: Close\r\n\r\n");

            service.write(transport, std::move(packet));
          }
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));

  service.set_option(YOPT_LFBFD_PARAMS, 65536, -1, 0, 0);
  service.set_option(YOPT_CHANNEL_LOCAL_PORT, 0, 30002);
  service.set_option(YOPT_CHANNEL_LOCAL_PORT, 1, 30001);

  service.open(0, YCM_UDP_CLIENT);
  service.open(1, YCM_UDP_CLIENT);

  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch_events();
    if (duration >= 6000000)
    {
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int, char **)
{
  yasioTest();

  return 0;
}
