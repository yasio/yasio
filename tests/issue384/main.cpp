#include "yasio/yasio.hpp"
#include <iostream>

using namespace yasio;
using namespace yasio::inet;

int main()
{

  // reserve 1024 socket.fds for test io service when fd greater than 1024
  std::vector<xxsocket> socks;
  socks.resize(1024);
  for (int i = 0; i < 1024; ++i)
    socks[i].open();

  deadline_timer timer     = {};
  yasio::io_service service = {};
  service.set_option(YOPT_S_NO_NEW_THREAD, 1);
  service.set_option(YOPT_S_DEFERRED_EVENT, 0);
  timer.expires_from_now(std::chrono::seconds(3));
  timer.async_wait_once(service, [](io_service& svc) { 
	  svc.stop();
  });
  printf("%s", "===>The service will stop after 3 seconds ...\n\n");
  service.start([](event_ptr&& ev) {});
  printf("%s", "\n===>The service stopped.\n");

  // cleanup reserved socket.fds
  for (auto& sock : socks)
    sock.close();
  socks.clear();
  return 0;
}
