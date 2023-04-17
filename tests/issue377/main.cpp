#include "yasio/yasio.hpp"
#include <iostream>

using namespace yasio;
using namespace yasio::inet;

int main()
{
  
  yasio::io_service server = {};
  deadline_timer timer     = {server};
  server.set_option(YOPT_S_NO_DISPATCH, 1);
  server.set_option(YOPT_C_REMOTE_HOST, 0, "127.0.0.1");
  server.set_option(YOPT_C_REMOTE_PORT, 0, 7899);
  server.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 1);
  server.open(0, YCK_TCP_SERVER);
  server.start([&](event_ptr&& ev) {
    switch (ev->kind())
    {
      case YEK_CONNECT_RESPONSE:
        std::cout << "connected.And the connection is disconnected after 3 seconds." << std::endl;
        timer.expires_from_now(std::chrono::seconds(3));
        timer.async_wait_once([](io_service& server) { server.close(0); });
        break;
      default:
        break;
    }
  });

  yasio::io_service client = {};
  client.set_option(YOPT_S_NO_DISPATCH, 1);
  client.set_option(YOPT_C_REMOTE_HOST, 0, "127.0.0.1");
  client.set_option(YOPT_C_REMOTE_PORT, 0, 7899);
  client.open(0, YCK_TCP_CLIENT);
  client.start([](event_ptr&& ev) {
    switch (ev->kind())
    {
      case YEK_CONNECTION_LOST:

        std::cout << "connection lost." << std::endl;
        break;
      default:
        break;
    }
  });

  while (true)
  {
    server.dispatch();
    client.dispatch();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  return 0;
}