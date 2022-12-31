#include "yasio/yasio.hpp"
#include <iostream>

#if defined(__linux__)
#  include <sys/resource.h>
#endif

using namespace yasio;
using namespace yasio::inet;

int main()
{
#if defined(__linux__)
  static const int required_fd_limit = 4096;
  struct rlimit rlim {};
  int ret = getrlimit(RLIMIT_NOFILE, &rlim);
  if (ret == 0)
  {
    printf("===> The file descriptor limits: rlim_cur=%d, rlim_max=%d\n", (int)rlim.rlim_cur, (int)rlim.rlim_max);

    if (rlim.rlim_cur < required_fd_limit)
    {
      rlim.rlim_cur = required_fd_limit;
      ret           = setrlimit(RLIMIT_NOFILE, &rlim);
      if (ret == 0)
      {
        printf("===> Enlarge file descriptor limit current to to %d succeed\n\n", (int)rlim.rlim_cur);
      }
    }
  }
#endif

  static const int reserve_socks_count = 1024;
  // reserve 1024 socket.fds for test io service when fd greater than 1024
  std::vector<xxsocket> socks;
  socks.resize(reserve_socks_count);
  for (auto& sock : socks)
    sock.open();

  deadline_timer timer      = {};
  yasio::io_service service = {};
  service.set_option(YOPT_S_NO_NEW_THREAD, 1);
  service.set_option(YOPT_S_DEFERRED_EVENT, 0);
  timer.expires_from_now(std::chrono::seconds(3));
  timer.async_wait_once(service, [](io_service& svc) { svc.stop(); });
  printf("%s", "===>The service will stop after 3 seconds ...\n\n");
  service.start([](event_ptr&& ev) {});
  printf("%s", "\n===>The service stopped.\n");

  // cleanup reserved socket.fds
  for (auto& sock : socks)
    sock.close();
  socks.clear();
  return 0;
}
