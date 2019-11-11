#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

void yasioMulticastTest()
{
  io_service service;

  io_hostent hosts[] = {{"0.0.0.0", 20524}};

  service.start_service(hosts, YASIO_ARRAYSIZE(hosts), [&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fprintf(stdout, "%s", "\n--------------------\n");
        fflush(stdout);

        obstream obs;
        obs.write_bytes("hello client, my ip is:");
        obs.write_bytes(event->transport()->local_endpoint().to_string());
        obs.write_bytes("\n");
        service.write(event->transport(), std::move(obs.buffer()));
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          /*struct ip_mreq mreq;
          mreq.imr_interface.s_addr = inet_addr("0.0.0.0");
          mreq.imr_multiaddr.s_addr = inet_addr("224.0.0.19");
          service.set_option(YOPT_TRANSPORT_SOCKOPT, event->transport(), IPPROTO_IP,
                             IP_ADD_MEMBERSHIP, &mreq, (int)sizeof(mreq));*/

          obstream obs;
          obs.write_bytes("hello client, my ip is:");
          obs.write_bytes(event->transport()->local_endpoint().to_string());
          obs.write_bytes("\n");
          service.write(event->transport(), std::move(obs.buffer()));
          // service.open(0, YCM_UDP_SERVER);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost, bytes transferred\n");
        break;
    }
  });

  service.set_option(YOPT_CHANNEL_LFBFD_PARAMS, 0, 16384, -1, 0, 0);

  /// channel 0: enable  multicast server
  service.set_option(YOPT_CHANNEL_REMOTE_HOST, 0, "224.0.0.19");
  service.open(0, YCM_MCAST_SERVER | YCM_MCAST_LOOPBACK);

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
  // yasioTest();
  // reference:
  // https://www.winsocketdotnetworkprogramming.com/winsock2programming/winsock2advancedmulticast9a.html
  yasioMulticastTest();

  return 0;
}
