#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "yasio/yasio.hpp"

#include "yasio/detail/ibstream.hpp"
#include "yasio/detail/obstream.hpp"

using namespace yasio;

#define HTTP_TEST_HOST "tool.chinaz.com"

static uint16_t ip_chksum(uint16_t* addr, int len)
{
  int nleft       = len;
  uint32_t sum    = 0;
  uint16_t* w     = addr;
  uint16_t answer = 0;

  // Adding 16 bits sequentially in sum
  while (nleft > 1)
  {
    sum += *w;
    nleft -= 2;
    w++;
  }

  // If an odd byte is left
  if (nleft == 1)
  {
    *(unsigned char*)(&answer) = *(unsigned char*)w;
    sum += answer;
  }

  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  answer = ~sum;

  return answer;
}

static bool icmp_ping(const char* host, const std::chrono::microseconds& wtimeout)
{
  enum
  {
    icmp_echo       = 8,
    icmp_echo_reply = 0,
    icmp_min_len    = 14,
  };

  std::vector<ip::endpoint> endpoints;
  xxsocket::resolve(endpoints, host, 0);
  if (endpoints.empty())
  {
    YASIO_LOG("resolv host: %s failed!", host);
    return false;
  }

  xxsocket s;
#if defined(_WIN32)
  int socktype = SOCK_RAW;
#else
  int socktype = SOCK_DGRAM;
#endif
  if (!s.open(AF_INET, socktype, IPPROTO_ICMP))
  {
    int ec = xxsocket::get_last_errno();
    YASIO_LOG("create socket failed, ec=%d, %s failed!", ec, xxsocket::strerror(ec));
    return false;
  }

  s.set_nonblocking(true);

  u_short id                  = std::rand() % (std::numeric_limits<unsigned short>::max)();
  static uint16_t s_seqno     = 0;
  static const int s_icmp_mtu = 1472;
  std::vector<char> icmp_request;

  cxx17::string_view userdata = "yasio-3.37.6";

#if !defined(_WIN32)
  int nud = (std::max)((int)icmp_min_len, (std::min)((int)userdata.size(), s_icmp_mtu));
#else
  int nud      = 32; // win32 must be 32 bytes
#endif
  icmp_request.resize(sizeof(icmp_header) + YASIO_SZ_ALIGN(nud, 2));
  icmp_header* hdr = (icmp_header*)&icmp_request.front();
  hdr->id          = id;
  hdr->type        = icmp_echo;
  hdr->seqno       = s_seqno++;

  memcpy(&icmp_request.front() + sizeof(icmp_header), (const char*)userdata.data(), (std::min)(userdata.size(), icmp_request.size() - sizeof(icmp_header)));
  hdr->checksum = ip_chksum((uint16_t*)icmp_request.data(), static_cast<int>(icmp_request.size()));

  s.sendto(icmp_request.data(), static_cast<int>(icmp_request.size()), endpoints[0]);

  fd_set readfds;
  if (xxsocket::select(s, &readfds, nullptr, nullptr, wtimeout) > 0)
  {
    char buf[128];
    ip::endpoint from;
    int n = s.recvfrom(buf, sizeof(buf), from);

    if (n > sizeof(ip_header) + sizeof(icmp_header))
    {
      const char* icmp_raw = (buf + sizeof(ip_header));
      icmp_header* hdr     = (icmp_header*)icmp_raw;
      u_short sum          = ip_chksum((uint16_t*)icmp_raw, n - sizeof(ip_header));
      if (sum != 0)
        return false; // checksum failed

      if (hdr->type != icmp_echo_reply)
      {
        YASIO_LOG("not echo reply packet!");
        return false; // not echo reply
      }
      if (hdr->id != id)
      {
        YASIO_LOG("id incorrect!");
        return false; // id not equals
      }
      return true;
    }
    YASIO_LOG("invalid packet!");
    return false; // ip packet incorrect
  }

  YASIO_LOG("timeout!");
  return false; // timeout
}


void yasioTest()
{
  yasio::inet::io_hostent endpoints[] = {{HTTP_TEST_HOST, 80}};

  for (int i = 0; i < 4; ++i)
  {
    auto start_ms = yasio::clock();
    if (icmp_ping("www.ip138.com", std::chrono::seconds(3)))
      printf("ping www.ip138.com succeed, time=%d(ms)\n", static_cast<int>(yasio::clock() - start_ms));
    else
      printf("ping www.ip138.com failed!\n");
  }

  using namespace cxx17;
  bool yes = cxx20::starts_with("hello world", "hello");
  yes      = cxx20::starts_with("hello world", (int)'h');
  yes      = cxx20::starts_with("hello world", std::string{"hello"});
  yes      = cxx20::starts_with(std::string{"hello world"}, "hello");
  yes      = cxx20::starts_with(std::string{"hello world"}, std::string{"hello"});
  yes      = cxx20::starts_with(std::string{"hello world"}, (int)'h');
#if YASIO__HAS_FULL_CXX11
  yes      = cxx20::starts_with("hello world"_sv, (int)'h');
  yes      = cxx20::starts_with("hello world"_sv, "hello");
  yes      = cxx20::starts_with("hello world", "hello"_sv);
  yes      = cxx20::starts_with(std::string{"hello world"}, "hello"_sv);
  yes      = cxx20::starts_with("hello world"_sv, std::string{"hello"});
#endif

  yasio::obstream obs;
  obs.push(sizeof(u_short));
  obs.write(3.141592654);
  obs.write(1.17723f);
  obs.write_ix<int32_t>(20201125);
  obs.write_ix<int64_t>(-9223372036854775807);
#if defined(YASIO_HAVE_HALF_FLOAT)
  obs.write(static_cast<fp16_t>(3.85f));
#endif
  obs.write_varint(23123, 3);
  obs.pop(sizeof(u_short));

  yasio::ibstream_view ibs(&obs);
  auto r0 = ibs.read_varint(sizeof(u_short)); // length: uint24
  auto r1 = ibs.read<double>();
  auto f1 = ibs.read<float>();
  auto v5 = ibs.read_ix<int32_t>();
  auto v6 = ibs.read_ix<int64_t>();
#if defined(YASIO_HAVE_HALF_FLOAT)
  auto v7 = static_cast<float>(ibs.read<fp16_t>());
#endif
  auto v8 = ibs.read_varint(3); // uint24
#if defined(YASIO_HAVE_HALF_FLOAT)
  std::cout << r0 << ", " << r1 << ", " << f1 << ", " << v5 << ", " << v6 << ", " << v7 << ", " << v8 << "\n";
#else
  std::cout << r0 << ", " << r1 << ", " << f1 << ", " << v5 << ", " << v6 << ", " << v8 << "\n";
#endif
  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));

  resolv_fn_t resolv = [&](std::vector<ip::endpoint>& endpoints, const char* hostname, unsigned short port) {
    return service.resolve(endpoints, hostname, port);
  };
  service.set_option(YOPT_S_RESOLV_FN, &resolv);

  std::vector<transport_handle_t> transports;

  deadline_timer udpconn_delay;
  deadline_timer udp_heartbeat;
  int total_bytes_transferred = 0;

  int max_request_count = 1;
  service.set_option(YOPT_S_DEFERRED_EVENT, 0);
  service.start([&](event_ptr&& event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        total_bytes_transferred += static_cast<int>(packet.size());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fflush(stdout);
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
                            "Chrome/87.0.4280.66 Safari/537.36\r\n");
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
          udpconn_delay.async_wait_once(service, [](io_service& service) { service.open(0); });
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
