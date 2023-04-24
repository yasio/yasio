#include "yasio/yasio.hpp"

using namespace yasio;

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
          printf("A client is income, status=%d, %lld, combine 2 packet and send to client!\n", event->status(), event->timestamp());

          // send 2 packet
          obstream obs1;
          auto where = obs1.push<uint32_t>();
          for (auto i = 0; i < 119 - 4; ++i)
            obs1.write_byte(i + 1);
          obs1.pop<uint32_t>(where, static_cast<uint32_t>(obs1.length()));

          obstream obs2;
          where = obs2.push<uint32_t>();
          for (auto i = 0; i < 104 - 4; ++i)
            obs2.write_byte(i + 1);
          obs2.pop<uint32_t>(where, static_cast<uint32_t>(obs2.length()));

          // merge 2 packets
          obs1.write_bytes(obs2.data(), static_cast<int>(obs2.length()));
          service.write(event->transport(), std::move(obs1.buffer()));
        }

        break;
      }
    }
  });
  service.set_option(YOPT_S_CONNECT_TIMEOUT, 5);
  service.set_option(YOPT_C_UNPACK_PARAMS, 1, 65535, 0, 4, 0);
  service.set_option(YOPT_C_LFBFD_IBTS, 1, 4);  // Sets initial bytes to strip
  service.open(0, YCK_TCP_SERVER);              // open server

  delay_timer.expires_from_now(std::chrono::seconds(1));
  delay_timer.async_wait_once([](io_service& service) { service.open(1, YCK_TCP_CLIENT); });

  getchar();
  return 0;
}
