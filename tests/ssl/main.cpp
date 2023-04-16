#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"

#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

#include "sslcerts.hpp"

using namespace yasio;

enum
{
  SSLTEST_CHANNEL_HTTP_CLIENT,
  SSLTEST_CHANNEL_CLIENT,
  SSLTEST_CHANNEL_SERVER,
  SSLTEST_MAX_CHANNELS
};

void yasioTest()
{
  yasio::inet::io_hostent endpoints[] = {{"github.com", 443}, {"127.0.0.1", 20231}, {"127.0.0.1", 20231}};

  io_service service(endpoints, SSLTEST_MAX_CHANNELS);

  std::vector<transport_handle_t> transports;

  deadline_timer udpconn_delay;
  deadline_timer udp_heartbeat;
  int http_client_bytes_transferred = 0;

  int done_count = 0;

  service.set_option(YOPT_S_FORWARD_PACKET, 1); // fast forward packet to up layer once got data from OS kernel
  service.set_option(YOPT_S_SSL_CACERT, SSLTEST_CACERT);
  service.set_option(YOPT_S_SSL_CERT, SSLTEST_CERT, SSLTEST_PKEY);
  service.set_option(YOPT_C_MOD_FLAGS, SSLTEST_CHANNEL_SERVER, YCF_REUSEADDR, 0);

  yasio::sbyte_buffer http_resp_data;

  service.start([&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_ON_PACKET: {
        auto packet = event->packet_view();
        switch (event->cindex())
        {
          case SSLTEST_CHANNEL_HTTP_CLIENT: {
            auto packet = event->packet_view();
            http_client_bytes_transferred += static_cast<int>(packet.size());
            http_resp_data.append(packet.data(), packet.data() + packet.size());
          }
          break;
          case SSLTEST_CHANNEL_CLIENT:
            fprintf(stdout, "==> ssl client: recv message '%s' from server\n", std::string{packet.data(), packet.size()}.c_str());
            fflush(stdout);
            service.close(event->transport());
            break;
          case SSLTEST_CHANNEL_SERVER:
            fprintf(stdout, "==> ssl server: recv message '%s' from client\n", std::string{packet.data(), packet.size()}.c_str());
            fflush(stdout);
            break;
        }

        break;
      }
      case YEK_ON_OPEN:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          obstream obs;
          switch (event->cindex())
          {
            case SSLTEST_CHANNEL_HTTP_CLIENT: {
              obs.write_bytes("GET / HTTP/1.1\r\n");

              obs.write_bytes("Host: github.com\r\n");

              obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                              "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                              "Chrome/108.0.5359.125 Safari/537.36\r\n");
              obs.write_bytes("Accept: */*;q=0.8\r\n");
              obs.write_bytes("Connection: Close\r\n\r\n");

              service.write(transport, std::move(obs.buffer()));
            }
            break;
            case SSLTEST_CHANNEL_CLIENT:
              obs.write_bytes("hello server!");
              service.write(transport, std::move(obs.buffer()));
              break;
            case SSLTEST_CHANNEL_SERVER:
              obs.write_bytes("hello client!");
              service.write(transport, std::move(obs.buffer()));
              break;
          }

          transports.push_back(transport);
        }
        break;
      case YEK_ON_CLOSE:
        if (++done_count == SSLTEST_MAX_CHANNELS)
        {
          fprintf(stdout, "===> http client: response %d bytes from www server.\n\n", http_client_bytes_transferred);
          // fwrite(http_resp_data.data(), http_resp_data.size(), 1, stdout);
          fflush(stdout);
          service.stop();
        }
        break;
    }
  });

  /*
  ** If after 5 seconds no data interaction at application layer,
  ** send a heartbeat per 10 seconds when no response, try 2 times
  ** if no response, then he connection will shutdown by driver.
  ** At windows will close with error: 10054
  */
  service.set_option(YOPT_S_TCP_KEEPALIVE, 5, 10, 2);

  std::this_thread::sleep_for(std::chrono::microseconds(200));
  service.open(SSLTEST_CHANNEL_HTTP_CLIENT, YCK_SSL_CLIENT); // open https client
  service.open(SSLTEST_CHANNEL_SERVER, YCK_SSL_SERVER);      // open local ssl server
  std::this_thread::sleep_for(std::chrono::microseconds(200));
  service.open(SSLTEST_CHANNEL_CLIENT, YCK_SSL_CLIENT); // open local ssl client

  time_t duration = 0;
  while (service.is_running())
  {
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

int main(int, char**)
{
#if defined(_WIN32)
  SetConsoleOutputCP(CP_UTF8);
#endif
  yasioTest();

  return 0;
}
