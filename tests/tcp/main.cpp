#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "yasio/yasio.hpp"

using namespace yasio;

#define HTTP_TEST_HOST "tool.chinaz.com"


void yasioTest()
{
  yasio::inet::io_hostent endpoints[] = {{HTTP_TEST_HOST, 80}};

  using namespace cxx17;
  bool yes = cxx20::starts_with("hello world", "hello");
  yes      = cxx20::starts_with("hello world", (int)'h');
  yes      = cxx20::starts_with("hello world", std::string{"hello"});
  yes      = cxx20::starts_with(std::string{"hello world"}, "hello");
  yes      = cxx20::starts_with(std::string{"hello world"}, std::string{"hello"});
  yes      = cxx20::starts_with(std::string{"hello world"}, (int)'h');
#if YASIO__HAS_CXX14
  yes      = cxx20::starts_with("hello world"_sv, (int)'h');
  yes      = cxx20::starts_with("hello world"_sv, "hello");
  yes      = cxx20::starts_with("hello world", "hello"_sv);
  yes      = cxx20::starts_with(std::string{"hello world"}, "hello"_sv);
  yes      = cxx20::starts_with("hello world"_sv, std::string{"hello"});
#endif

  yasio::obstream obs;
  auto where = obs.push<int>();
  obs.write(3.141592654);
  obs.write(1.17723f);
  obs.write_ix(20201125);
  obs.write_ix(-9223372036854775807);
#if defined(YASIO_ENABLE_HALF_FLOAT)
  obs.write(static_cast<fp16_t>(3.85f));
#endif
  obs.write_varint(23123, 3);
  obs.pop<int>(where);

  yasio::ibstream_view ibs(&obs);
  auto r0 = ibs.read<int>();
  auto r1 = ibs.read<double>();
  auto f1 = ibs.read<float>();
  auto v5 = ibs.read_ix<int32_t>();
  auto v6 = ibs.read_ix<int64_t>();
#if defined(YASIO_ENABLE_HALF_FLOAT)
  auto v7 = static_cast<float>(ibs.read<fp16_t>());
#endif
  auto v8 = ibs.read_varint(3); // uint24
#if defined(YASIO_ENABLE_HALF_FLOAT)
  std::cout << r0 << ", " << r1 << ", " << f1 << ", " << v5 << ", " << v6 << ", " << v7 << ", " << v8 << "\n";
#else
  std::cout << r0 << ", " << r1 << ", " << f1 << ", " << v5 << ", " << v6 << ", " << v8 << "\n";
#endif

  yasio::sbyte_buffer vecbuf;
  std::string strbuf;
  std::array<char, 16> arrbuf;
  char raw_arrbuf[16];
  yasio::obstream_span<yasio::sbyte_buffer>{vecbuf}.write_bytes("hello world!");
  yasio::obstream_span<std::string>{strbuf}.write_bytes("hello world!");
  yasio::obstream_span<yasio::fixed_buffer_span>{arrbuf}.write_bytes("hello world!");
  yasio::obstream_span<yasio::fixed_buffer_span>{raw_arrbuf}.write_bytes("hello world!");


  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));

  resolv_fn_t resolv = [&](std::vector<ip::endpoint>& endpoints, const char* hostname, unsigned short port) {
    return service.resolve(endpoints, hostname, port);
  };
  service.set_option(YOPT_S_RESOLV_FN, &resolv);

  std::vector<transport_handle_t> transports;

  deadline_timer udpconn_delay(service);
  deadline_timer udp_heartbeat(service);
  int total_bytes_transferred = 0;

  int max_request_count = 1;
  service.start([&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        total_bytes_transferred += static_cast<int>(packet.size());
        // fwrite(packet.data(), packet.size(), 1, stdout);
        // fflush(stdout);
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto transport = event->transport();
          if (event->cindex() == 0)
          {
            obstream obs;
            obs.write_bytes("GET / HTTP/1.1\r\n");

            obs.write_bytes("Host: " HTTP_TEST_HOST "\r\n");

            // Chrome/51.0.2704.106
            obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                            "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/108.0.5359.125 Safari/537.36\r\n");
            obs.write_bytes("Accept: */*;q=0.8\r\n");
            obs.write_bytes("Connection: Close\r\n\r\n");

            service.write(transport, std::move(obs.buffer()));

            // service.close(transport, YWF_WRITE);
          }

          transports.push_back(transport);
        }
        break;
      case YEK_CONNECTION_LOST: {
        auto channel = service.channel_at(event->cindex());
        printf("\n\nThe connection is lost, %d bytes transferred, internal stats: %lld\n", total_bytes_transferred, channel->bytes_transferred());

        total_bytes_transferred = 0;
        if (--max_request_count > 0)
        {
          udpconn_delay.expires_from_now(std::chrono::seconds(1));
          udpconn_delay.async_wait_once([](io_service& service) { service.open(0); });
        }
        else
          service.stop();
        break;
      }
    }
  });

  /*
  ** If after 5 seconds no data interaction at application layer,
  ** send a heartbeat per 10 seconds when no response, try 2 times
  ** if no response, then he connection will shutdown by driver.
  ** At windows will close with error: 10054
  */
  service.set_option(YOPT_S_TCP_KEEPALIVE, 5, 10, 2);
  service.set_option(YOPT_S_NO_DISPATCH, 1);

  std::this_thread::sleep_for(std::chrono::seconds(1));
  service.open(0, YCK_TCP_CLIENT); // open http client
  time_t duration = 0;
  while (service.is_running())
  {
    service.dispatch();
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
