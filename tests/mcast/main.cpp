#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

#define UDP_TEST_ONLY 0
#define MCAST_ADDR "224.0.0.19"
#define MCAST_PORT (unsigned short)22016

using namespace yasio;

enum
{
  MCAST_SERVER_INDEX = 0,
  MCAST_CLIENT_INDEX = 1,
};

// 1: server, 2: client, 3: server,client
enum
{
  MCAST_ROLE_SERVER = 1,
  MCAST_ROLE_CLIENT = 2,
  MCAST_ROLE_DUAL   = MCAST_ROLE_SERVER | MCAST_ROLE_CLIENT,
};

void yasioMulticastTest()
{
  const int mcast_role = MCAST_ROLE_DUAL;

  io_hostent hosts[] = {
    {"0.0.0.0", MCAST_PORT}, // udp server
#if UDP_TEST_ONLY
    {"127.0.0.1", MCAST_PORT} // multicast client
#else
    {MCAST_ADDR, MCAST_PORT} // multicast client
#endif
  };

  static ip::endpoint server_mcast_ep{MCAST_ADDR, 22016};

  io_service service(hosts, YASIO_ARRAYSIZE(hosts));
  service.start([&](event_ptr&& event) {
    auto thandle = event->transport();
    switch (event->kind())
    {
      case YEK_PACKET: {
        // print packet msg
        auto packet = std::move(event->packet());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fprintf(stdout, "%s", "\n--------------------\n");
        fflush(stdout);
        auto transport = event->transport();
        if (event->cindex() == MCAST_SERVER_INDEX)
        {
          // multicast udp server channel
          // delay reply msg
          obstream obs;
          obs.write_bytes("hello client, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n\n");
#if UDP_TEST_ONLY
          service.write(transport, std::move(obs.buffer()));
#else
          u_short remote_port = event->transport()->remote_endpoint().port();
          const ip::endpoint client_mcast_ep{MCAST_ADDR, remote_port};
          service.write_to(transport, std::move(obs.buffer()), client_mcast_ep);
#endif
        }
        else if (event->cindex() == MCAST_CLIENT_INDEX)
        {
          obstream obs;
          obs.write_bytes("hello server, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n");

          service.schedule(std::chrono::milliseconds(1000), [obs, transport, pk = std::move(packet)](io_service& service) mutable {
#if !UDP_TEST_ONLY
            // parse non multi-cast endpoint
            // win32: not required but also works
            // non-win32: required, otherwise the server 4-tuple doesn't works
            endpoint non_mcast_ep;
            pk.push_back('\0'); // ensure null-terminated character
            cxx17::string_view sv(pk.data(), pk.size() - 1);
            auto colon = sv.find_first_of(':');
            if (colon != cxx17::string_view::npos)
            {
              if (non_mcast_ep.as_is(&sv[colon + 1]))
                service.write_to(transport, std::move(obs.buffer()), non_mcast_ep);
            }
#else
            service.write(transport, std::move(obs.buffer()));
#endif
            return true;
          });
        }
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->passive())
          return;
        if (event->cindex() == MCAST_CLIENT_INDEX)
        {
          auto transport = event->transport();
          obstream obs;
          obs.write_bytes("==> hello server, I'm client\n\n");
          service.write(transport, std::move(obs.buffer()));
        }
        else if (event->cindex() == MCAST_SERVER_INDEX)
        {
          printf("==> A client income\n");
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost!\n");
        break;
    }
  });

  service.set_option(YOPT_C_MOD_FLAGS, MCAST_SERVER_INDEX, YCF_REUSEADDR, 0);

  const int mcast_loopback = mcast_role == MCAST_ROLE_DUAL ? 1 : 0;

  /// channel 0: enable  multicast
#if !UDP_TEST_ONLY
  service.set_option(YOPT_C_ENABLE_MCAST, MCAST_SERVER_INDEX, MCAST_ADDR, mcast_loopback);
#endif
  if (mcast_role & MCAST_ROLE_SERVER)
    service.open(MCAST_SERVER_INDEX, YCK_UDP_SERVER);

#if !UDP_TEST_ONLY
  service.set_option(YOPT_C_ENABLE_MCAST, MCAST_CLIENT_INDEX, MCAST_ADDR, mcast_loopback);
#endif
  if (mcast_role & MCAST_ROLE_CLIENT)
    service.open(MCAST_CLIENT_INDEX, YCK_UDP_CLIENT);

  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch();
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int, char**)
{
  // reference:
  // https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedmulticast9a.html
  yasioMulticastTest();

  return 0;
}
