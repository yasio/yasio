#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "yasio/yasio.hpp"

#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

void yasioTest()
{
  yasio::inet::io_hostent endpoints[] = {
      {"test.yasio.org", 50001}, // tcp client
      {"test.yasio.org", 50002},  // udp client
      {"test.yasio.org", 50003},  // kcp client
  };

  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));

  resolv_fn_t resolv = [&](std::vector<ip::endpoint>& endpoints, const char* hostname,
                           unsigned short port) {
    return service.builtin_resolv(endpoints, hostname, port);
  };
  service.set_option(YOPT_S_RESOLV_FN, &resolv);

  std::vector<transport_handle_t> transports;

  deadline_timer tcp_send_timer(service);
  deadline_timer udp_send_timer(service);
  deadline_timer kcp_send_timer(service);
  int total_bytes_transferred = 0;

  int max_request_count = 2;

  service.start([&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        total_bytes_transferred += static_cast<int>(packet.size());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fflush(stdout);
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          if (event->cindex() == 0)
          {
            tcp_send_timer.expires_from_now(std::chrono::seconds(2));
            tcp_send_timer.async_wait([&service, transport]() -> bool {
              obstream obs;
              obs.write_bytes("[TCP] Hello\r\n");
              service.write(transport, std::move(obs.buffer()));
              return false;
            });
          }
          else if (event->cindex() == 1)
          {
            udp_send_timer.expires_from_now(std::chrono::seconds(2));
            udp_send_timer.async_wait([&service, transport]() -> bool {
              obstream obs;
              obs.write_bytes("[UDP] Hello\r\n");
              service.write(transport, std::move(obs.buffer()));
              return false;
            });
          }
          else if (event->cindex() == 2)
          {
            kcp_send_timer.expires_from_now(std::chrono::seconds(2));
            kcp_send_timer.async_wait([&service, transport, &max_request_count]() -> bool {
              obstream obs;
              obs.write_bytes("[KCP] Hello\r\n");
              service.write(transport, std::move(obs.buffer()));
              return true;
            });
          }

          transports.push_back(transport);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost, %d bytes transferred\n", total_bytes_transferred);
        break;
    }
  });

  /*
  ** If after 5 seconds no data interaction at application layer,
  ** send a heartbeat per 10 seconds when no response, try 2 times
  ** if no response, then he connection will shutdown by driver.
  ** At windows will close with error: 10054
  */
  service.set_option(YOPT_S_TCP_KEEPALIVE, 5, 10, 2);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  service.open(0, YCK_TCP_CLIENT); // open channel 0 as TCP client
  service.open(1, YCK_UDP_CLIENT); // open channel 1 as UDP client
  service.open(2, YCK_KCP_CLIENT); // open channel 2 as KCP client

  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch();
    if (duration >= 6000000)
    {
      for (auto transport : transports)
        service.close(transport);
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int, char**)
{
  yasioTest();

  return 0;
}
