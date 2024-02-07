#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "yasio/yasio.hpp"

using namespace yasio;

static highp_time_t s_last_send_time[3]   = {0};
static const highp_time_t s_send_interval = 2000; // (ms)

void run_echo_client(const char* ip, int port, const char* protocol)
{
  yasio::inet::io_hostent endpoints[] = {
      {ip, static_cast<u_short>(port)}, // tcp client
  };

  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));

  std::vector<transport_handle_t> transports;

  deadline_timer send_timer(service);
  int total_bytes_transferred = 0;

  int max_request_count = 10;

  service.start([&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto index = event->cindex();
        auto diff  = highp_clock() - s_last_send_time[index];

        auto packet = std::move(event->packet());
        total_bytes_transferred += static_cast<int>(packet.size());
        printf("[latency: %lf(ms), bytes=%zu]", diff / 1000.0, packet.size());
        fwrite(packet.data(), packet.size(), 1, stdout);
        printf("\n");
        fflush(stdout);
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          auto index     = event->cindex();
          if (index == 0)
          {
            send_timer.expires_from_now(std::chrono::milliseconds(s_send_interval));
            send_timer.async_wait([transport, index, protocol](io_service& service) -> bool {
              obstream obs;
              obs.write_byte('[');
              obs.write_bytes(protocol, static_cast<int>(strlen(protocol)));
              obs.write_bytes("] Hello, ");
              auto n = 4096 - static_cast<int>(obs.length());
              obs.fill_bytes(n, 'a');
              auto bytes_transferred = service.write(transport, std::move(obs.buffer()));
              printf("sent %d bytes ...\n", bytes_transferred);
              s_last_send_time[index] = highp_clock();
              return false;
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

  std::this_thread::sleep_for(std::chrono::seconds(1));

  printf("[%s] connecting %s:%u ...\n", protocol, ip, port);
  if (cxx20::ic::iequals(protocol, "udp"))
  {
    service.open(0, YCK_UDP_CLIENT);
  }
  else if (cxx20::ic::iequals(protocol, "kcp"))
  {
    service.open(0, YCK_KCP_CLIENT);
  }
  else
  { 
    /*
     ** If after 5 seconds no data interaction at application layer,
     ** send a heartbeat per 10 seconds when no response, try 2 times
     ** if no response, then he connection will shutdown by driver.
     ** At windows will close with error: 10054
     */
    service.set_option(YOPT_S_TCP_KEEPALIVE, 5, 10, 2);
    service.open(0, YCK_TCP_CLIENT);
  }

  time_t duration = 0;
  while (service.is_running())
  {
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

int main(int argc, char** argv)
{
  if (argc > 3)
    run_echo_client(argv[1], atoi(argv[2]), argv[3]);

  return 0;
}
