#include "yasio/yasio.hpp"

using namespace yasio::inet;

void run_udp_echo_server(const char *ip, u_short port)
{
  io_hostent endpoints[] = {{ip, port}};
  io_service udp_server(endpoints, 1);
  udp_server.set_option(YOPT_S_NO_NEW_THREAD, 1);

  // !important, because we set YOPT_S_NO_NEW_THREAD, so need a timer to start server
  deadline_timer starter(udp_server);
  starter.expires_from_now(std::chrono::seconds(1));
  starter.async_wait([&]() {
    udp_server.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, -1, 0, 0);
    udp_server.open(0, YCM_UDP_SERVER);
  });

  udp_server.start_service([&](event_ptr ev) {
    switch (ev->kind())
    {
      case YEK_PACKET:
        udp_server.write(ev->transport(), std::move(ev->packet()));
        break;
    }
  });
}

int main(int argc, char **argv)
{
  if (argc > 2)
    run_udp_echo_server(argv[1], atoi(argv[2]));
  return 0;
}