#include "yasio/yasio.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

int main()
{
  yasio::inet::io_hostent endpoints[] = {
      {"0.0.0.0", 8010},  // tcp server
      {"127.0.0.1", 8010} // tcp client
  };
  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));
  deadline_timer delay_timer(service);
  service.start([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET:
        if (event->cindex() == 1)
          printf("Got a packet from server, size=%d\n", (int)event->packet().size());
        break;
      case YEK_CONNECT_RESPONSE: {
        if (event->cindex() == 0)
        {
          printf("A client is income, status=%d, %lld, combine 2 packet and send to client!\n",
                 event->status(), event->timestamp());

          // send 2 packet
          obstream obs1;
          obs1.push32();
          for (auto i = 0; i < 119 - 4; ++i)
            obs1.write_byte(i + 1);
          obs1.pop32(obs1.length());

          obstream obs2;
          obs2.push32();
          for (auto i = 0; i < 104 - 4; ++i)
            obs2.write_byte(i + 1);
          obs2.pop32(obs2.length());

          // merge 2 packets
          obs1.write_bytes(obs2.data(), obs2.length());
          service.write(event->transport(), std::move(obs1.buffer()));
        }

        break;
      }
    }
  });
  service.set_option(YOPT_S_CONNECT_TIMEOUT, 5);
  service.set_option(YOPT_C_LFBFD_PARAMS, 1, 65535, 0, 4, 0);
  service.set_option(YOPT_C_LFBFD_IBTS, 1, 4);  // Sets initial bytes to strip
  service.set_option(YOPT_S_DEFERRED_EVENT, 0); // disable event queue
  service.open(0, YCK_TCP_SERVER);              // open server

  delay_timer.expires_from_now(std::chrono::seconds(1));
  delay_timer.async_wait_once([&]() {
    service.open(1, YCK_TCP_CLIENT);
  });

  getchar();
  return 0;
}
