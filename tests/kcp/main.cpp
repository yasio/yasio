#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define YASIO_HAVE_KCP 1
#define YASIO_HEADER_ONLY 1

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

void run_test()
{
  yasio::inet::io_hostent endpoints[] = {
      {"127.0.0.1", 30001}, // udp client1
      {"127.0.0.1", 30002}, // udp client2
  };

  io_service service;

  service.set_option(YOPT_TCP_KEEPALIVE, 60, 30, 3);

  resolv_fn_t resolv = [&](std::vector<ip::endpoint>& endpoints, const char* hostname,
                           unsigned short port) {
    return service.__builtin_resolv(endpoints, hostname, port);
  };
  service.set_option(YOPT_RESOLV_FN, &resolv);

  std::vector<transport_handle_t> transports;
  deadline_timer udp_msg_delay(service);
  service.start_service(endpoints, YASIO_ARRAYSIZE(endpoints), [&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        packet.push_back('\0');
        printf("index:%d, receive data:%s\n", event->transport()->cindex(), packet.data());

        if (event->cindex() == 1)
        { // response udp client
          obstream obs;
          obs.write_bytes("hello udp client 0\n");
          printf("---- response a packet to udp client 0\n");
          service.write(event->transport(), std::move(obs.buffer()));
        }
        else
        {
          auto transport = event->transport();
          service.schedule(std::chrono::seconds(3), [&service, transport](bool) {
            obstream obs;
            obs.write_bytes("hello udp client 1\n");
            printf("---- send a packet to udp client 1\n");
            service.write(transport, std::move(obs.buffer()));
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
            obstream obs;
            obs.write_bytes("GET /index.htm HTTP/1.1\r\n");

            obs.write_bytes("Host: www.ip138.com\r\n");

            obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                                  "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                                  "Chrome/51.0.2704.106 Safari/537.36\r\n");
            obs.write_bytes("Accept: */*;q=0.8\r\n");
            obs.write_bytes("Connection: Close\r\n\r\n");

            service.write(transport, std::move(obs.buffer()));
          }
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));

  service.set_option(YOPT_CHANNEL_LOCAL_PORT, 0, 30002);
  service.set_option(YOPT_CHANNEL_LOCAL_PORT, 1, 30001);

  service.open(0, YCM_KCP_CLIENT);
  service.open(1, YCM_KCP_CLIENT);

  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch();
    if (duration >= 6000000)
    {
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int, char**)
{
  run_test();

  return 0;
}
