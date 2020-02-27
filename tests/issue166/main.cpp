#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

int main()
{
  yasio::inet::io_hostent endpoints[] = {
      {"127.0.0.1", 12345} // tcp client
  };
  io_service service(endpoints, 1);
  int retry_count = 0;
  int total_bytes_transferred = 0;
  service.start_service([&](event_ptr event) {
    switch (event->kind())
    {
        case YEK_PACKET: {
              auto packet = std::move(event->packet());
              total_bytes_transferred += static_cast<int>(packet.size());
              fwrite(packet.data(), packet.size(), 1, stdout);
              fflush(stdout);
              break;
            }
      case YEK_CONNECT_RESPONSE: {
        printf("The connection is YEK_CONNECT_RESPONSE, status=%d, %lld\n", event->status(),
               event->timestamp());
          
          auto transport = event->transport();
          if (event->cindex() == 0)
          {
            obstream obs;
            obs.write_i24(100);
            obs.push32();
            obs.write_bytes("hello world");
            obs.pop32();
              //000xxxxhelloworld
            ip::endpoint ep;
              
            if (ep.assign("127.0.0.1",12345))
            {
                transport->get_context().join_multicast_group(ep, 1);
                service.write_to(transport, std::move(obs.buffer()),ep);
            }
              //transport->leaveMCast();
            
          }

          //transports.push_back(transport);
          
        if (event->status() != 0)
        {
          if (retry_count++ < 2)
          {
            // service.schedule(std::chrono::milliseconds (100), [&](bool
            // cancelled){
            service.set_option(YOPT_C_REMOTE_ENDPOINT, 0, "127.0.0.1", 12345);
            service.open(0, YCM_UDP_CLIENT); // open udp client
            //});
          }
          else
            exit(-1);
        }
        break;
      }
    }
  });
  service.set_option(YOPT_S_CONNECT_TIMEOUT, 5);
  service.set_option(YOPT_C_LFBFD_PARAMS, 0, 65535, 3, 4, 7);
  service.set_option(YOPT_C_LFBFD_IBTS, 0, 7);  // Sets initial bytes to strip
  service.set_option(YOPT_S_DEFERRED_EVENT, 0); // disable event queue
  service.open(0, YCM_UDP_CLIENT);              // open udp client

  getchar();
  return 0;
}
