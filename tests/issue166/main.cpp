#include "yasio/yasio.hpp"

using namespace yasio::inet;

int main()
{
  yasio::inet::io_hostent endpoints[] = {
      {"127.0.0.1", 8010} // tcp client
  };
  io_service service(endpoints, 1);
  int retry_count = 0;
  service.start_service([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_CONNECT_RESPONSE: {
        printf("The connection is YEK_CONNECT_RESPONSE, status=%d, %lld\n", event->status(),
               event->timestamp());
        if (event->status() != 0)
        {
          if (retry_count++ < 2)
          {
            // service.schedule(std::chrono::milliseconds (100), [&](bool
            // cancelled){
            service.set_option(YOPT_C_REMOTE_ENDPOINT, 0, "127.0.0.1", 9010);
            service.open(0, YCM_TCP_CLIENT); // open tcp client
            //});
          }
          else
            exit(-1);
        }
        break;
      }
    }
  });
  service.set_option(YOPT_S_TIMEOUTS, 5, 5, -1);
  service.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, 0, 4, 0);
  service.set_option(YOPT_S_DEFERRED_EVENT, 0); // disable event queue
  service.open(0, YCM_TCP_CLIENT);              // open tcp client

  getchar();
  return 0;
}