#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

enum
{
  MCAST_SERVER_INDEX = 0,
  MCAST_CLIENT_INDEX = 1
};

void yasioMulticastTest()
{

  io_hostent hosts[] = {
      {"224.0.0.19", 22016}, // multicast server
      {"224.0.0.19", 22016}  // multicast client
  };

  io_service service(hosts, YASIO_ARRAYSIZE(hosts));
  service.start_service([&](event_ptr&& event) {
    auto thandle = event->transport();

    switch (event->kind())
    {
      case YEK_PACKET: {
        // print packet msg
        auto packet = std::move(event->packet());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fprintf(stdout, "%s", "\n--------------------\n");
        fflush(stdout);

        if (event->cindex() == MCAST_SERVER_INDEX)
        {
          // multicast udp server channel
          // delay reply msg
          obstream obs;
          obs.write_bytes("hello client, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n\n");
          service.write(thandle, std::move(obs.buffer()));
        }
        else if (event->cindex() == MCAST_CLIENT_INDEX)
        {
          obstream obs;
          obs.write_bytes("hello server, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n");

          service.schedule(std::chrono::milliseconds(1000), [&, thandle, obs]() {
            service.write(thandle, std::move(obs.buffer()));
          });
        }
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->cindex() == MCAST_CLIENT_INDEX)
        {
          obstream obs;
          obs.write_bytes("hello server, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n");
          service.write(thandle, std::move(obs.buffer()));
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost, bytes transferred\n");
        break;
    }
  });

  /// channel 0: enable  multicast
  service.set_option(YOPT_C_MOD_FLAGS, MCAST_SERVER_INDEX, YCF_REUSEPORT | YCF_MCAST_LOOPBACK, 0);
  service.open(MCAST_SERVER_INDEX, YCM_MCAST_SERVER);

  service.set_option(YOPT_C_MOD_FLAGS, MCAST_CLIENT_INDEX, YCF_MCAST_LOOPBACK, 0);
  service.open(MCAST_CLIENT_INDEX, YCM_MCAST_CLIENT);

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
