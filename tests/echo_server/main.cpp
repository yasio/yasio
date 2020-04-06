#include "yasio/yasio.hpp"

using namespace yasio::inet;

// 6: TCP server, 10: UDP server
void run_echo_server(const char *ip, u_short port, int channel_kind)
{
  io_hostent endpoints[] = {{ip, port}};
  io_service udp_server(endpoints, 1);
  udp_server.set_option(YOPT_S_NO_NEW_THREAD, 1);
  udp_server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);

  // !important, because we set YOPT_S_NO_NEW_THREAD, so need a timer to start server
  deadline_timer starter(udp_server);
  starter.expires_from_now(std::chrono::seconds(1));
  starter.async_wait_once([&]() {
    udp_server.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, -1, 0, 0);
    udp_server.open(0, channel_kind);
  });

  udp_server.start([&](event_ptr ev) {
    switch (ev->kind())
    {
      case YEK_PACKET:
        udp_server.write(ev->transport(), std::move(ev->packet()));
        break;
    case YEK_CONNECT_RESPONSE:
            printf("A client is income, status=%d, %lld, combine 2 packet and send to client!\n",
                            ev->status(), ev->timestamp());
            break;
    }
  });
}

int main(int argc, char **argv)
{
  if (argc > 3)
    run_echo_server(argv[1], atoi(argv[2]), atoi(argv[3]));
  return 0;
}
