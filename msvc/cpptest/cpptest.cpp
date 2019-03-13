#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#if defined(YASIO_IOCP)
#include "yasio/experimental/yasio_iocp.h"
#elif defined(YASIO_POLL)
#include "yasio/experimental/yasio_poll.h"
#elif defined(YASIO_EPOLL)
#include "yasio/experimental/yasio_epoll.h"
#else
#include "yasio/yasio.h"
#endif
#include "yasio/ibstream.h"
#include "yasio/obstream.h"

#if defined(_WIN32)
#  include <Shlwapi.h>
#  pragma comment(lib, "shlwapi.lib")
#  define strcasestr StrStrIA
#endif

using namespace yasio::inet;

template <size_t _Size> void append_string(std::vector<char> &packet, const char (&message)[_Size])
{
  packet.insert(packet.end(), message, message + _Size - 1);
}

void yasioTest()
{
  purelib::inet::io_hostent endpoints[] = {{"www.ip138.com", 80}, // http client
                                           {"127.0.0.1", 30001}, // tcp server
                                           {"127.0.0.1", 59281}}; // udp client

  yasio::obstream obs;
  obs.push24();
  obs.write_i24(16777215);
  obs.write_i24(16777213);
  obs.write_i24(259);
  obs.write_i24(16777217); // uint24 value overflow test
  obs.pop24();

  yasio::ibstream_view ibs(obs.data(), obs.length());
  auto n  = ibs.read_i24();
  auto v1 = ibs.read_i24();
  auto v2 = ibs.read_i24();
  auto v3 = ibs.read_i24();
  auto v4 = ibs.read_i24();

  io_service service;

  service.set_option(YOPT_TCP_KEEPALIVE, 60, 30, 3);

  resolv_fn_t resolv = [&](std::vector<ip::endpoint> &endpoints, const char *hostname,
                           unsigned short port) {
    return service.__builtin_resolv(endpoints, hostname, port);
  };
  service.set_option(YOPT_RESOLV_FUNCTION, &resolv);

  std::vector<transport_ptr> transports;
  service.set_option(YOPT_LFBFD_PARAMS, 16384, -1, 0, 0);
  service.set_option(YOPT_LOG_FILE, "yasio.log");

  deadline_timer udpconn_delay(service);
  deadline_timer udp_heartbeat(service);
  service.start_service(endpoints, _ARRAYSIZE(endpoints), [&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET:
      {
        auto packet = event->take_packet();
        packet.push_back('\0');
        printf("index:%d, receive data:%s\n", event->transport()->channel_index(), packet.data());
        if (event->channel_index() == 1)
        { // response udp client
          std::vector<char> packet;
          append_string(packet, "hello udp client!\n");
          service.write(event->transport(), std::move(packet));
        }
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          if (event->channel_index() == 0)
          {
            std::vector<char> packet;
            append_string(packet, "GET /index.htm HTTP/1.1\r\n");

            append_string(packet, "Host: www.ip138.com\r\n");

            append_string(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                                  "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                                  "Chrome/51.0.2704.106 Safari/537.36\r\n");
            append_string(packet, "Accept: */*;q=0.8\r\n");
            append_string(packet, "Connection: Close\r\n\r\n");

            service.write(transport, std::move(packet));
          }
          else if (event->channel_index() == 1)
          { // Sends message to server every per 3 seconds.
#if 0
            udp_heartbeat.expires_from_now(std::chrono::seconds(3));
            udp_heartbeat.async_wait(
                [&service, transport](bool) mutable { // called at network thread
                                                      // std::vector<char> packet;
                  // append_string(packet, "hello udp server!\n");
                  // service.write(transport, std::move(packet));
                  // service.stop_service();
                  service.reopen(transport);
                });
#endif
          }

          transports.push_back(transport);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  service.open(0); // open http client

  // service.set_option(YOPT_RECONNECT_TIMEOUT, 3);
  // service.open(1, YCM_TCP_CLIENT); // open tcp server
  // service.open(2, CHANNEL_UDP_SERVER);
#if 0
  udpconn_delay.expires_from_now(std::chrono::seconds(3));
  udpconn_delay.async_wait([&](bool) { // called at network thread
    printf("Open channel 1.\n");
    //service.set_option(YASIO_OPT_CHANNEL_LOCAL_PORT, 2, 4000);
    service.open(1, YCM_TCP_CLIENT); // open udp client
  });
#endif

  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch_events();
    if (duration >= 6000000)
    {
      for (auto transport : transports)
        service.close(transport);
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
}

int main(int, char **)
{
  yasioTest();

  return 0;
}
