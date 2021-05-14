#include "yasio/yasio.hpp"
#include <signal.h>

using namespace yasio::inet;

io_service* gservice = nullptr;
void handle_signal(int sig)
{
  if (gservice && sig == 2)
  {
    gservice->close(0);
  }
}

// 6: TCP server, 10: UDP server, 26: KCP server
void run_echo_server(const char* ip, u_short port, int channel_kind)
{
  io_hostent endpoints[] = {{ip, port}};
  io_service server(endpoints, 1);
  gservice = &server;
  server.set_option(YOPT_S_NO_NEW_THREAD, 1);
  server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);

  signal(SIGINT, handle_signal);

  // !important, because we set YOPT_S_NO_NEW_THREAD, so need a timer to start server
  deadline_timer timer;
  timer.expires_from_now(std::chrono::seconds(1));
  timer.async_wait_once(server, [channel_kind](io_service& server) {
    server.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, -1, 0, 0);
    server.open(0, channel_kind);
  });
  server.start([&, channel_kind](event_ptr ev) {
    switch (ev->kind())
    {
      case YEK_ON_OPEN:
        if (ev->passive())
        {
          if (ev->status() == 0)
          {
            printf("[%lld] start server ok, status=%d\n", ev->timestamp(), ev->status());
          }
          else
          {
            printf("[%lld] start server failed, status=%d\n", ev->timestamp(), ev->status());
          }
          return;
        }
        printf("[%lld] A client is income, status=%d\n", ev->timestamp(), ev->status());
      case YEK_ON_PACKET:
        if (!ev->packet().empty())
          server.write(ev->transport(), std::move(ev->packet()));

        // kick out after 3(s) if no new packet income
        {
          timer.expires_from_now(std::chrono::seconds(3));
          auto transport = ev->transport();
          timer.async_wait_once(server, [transport](io_service& server) { server.close(transport); });
        }
        break;
      case YEK_ON_CLOSE:
        if (ev->passive())
        {
          printf("[%lld] stop server done, status=%d\n", ev->timestamp(), ev->status());
          server.stop(); // stop could call self thread
        }
        break;
    }
  });
}

int main(int argc, char** argv)
{
  if (argc > 3)
    run_echo_server(argv[1], atoi(argv[2]), atoi(argv[3]));
  return 0;
}
