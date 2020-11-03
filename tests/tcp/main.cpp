#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "yasio/yasio.hpp"

#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

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

  cxx17::string_view userdata = "yasio-3.33.1";

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

  memcpy(&icmp_request.front() + sizeof(icmp_header), (const char*)userdata.data(),
         (std::min)(userdata.size(), icmp_request.size() - sizeof(icmp_header)));
  hdr->checksum = ip_chksum((uint16_t*)icmp_request.data(), icmp_request.size());

  s.sendto(icmp_request.data(), icmp_request.size(), endpoints[0]);

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
  yasio::inet::io_hostent endpoints[] = {{"www.ip138.com", 80}};

  for (int i = 0; i < 4; ++i)
  {
    if (icmp_ping("www.ip138.com", std::chrono::seconds(3)))
      printf("ping www.ip138.com succeed, times=%d\n", i + 1);
    else
      printf("ping www.ip138.com failed, times=%d\n", i + 1);
  }

  yasio::obstream obstest;
  obstest.push24();
  obstest.write_i(3.141592654);
  obstest.write_i(1.17723f);
  obstest.write_u24(0x112233);
  obstest.write_u24(16777217); // uint24 value overflow test
  obstest.write_i24(259);
  obstest.write_i24(-16);
  obstest.pop24();

  yasio::ibstream_view ibs(obstest.data(), static_cast<int>(obstest.length()));
  ibs.seek(3, SEEK_CUR);
  auto r1 = ibs.read_ix<double>();
  auto f1 = ibs.read_ix<float>();
  auto v1 = ibs.read_u24(); // should be 0x112233(1122867)
  auto v2 = ibs.read_u24(); // should be 1
  auto v3 = ibs.read_i24(); // should be 259
  auto v4 = ibs.read_i24(); // should be -16

  std::cout << r1 << ", " << f1 << ", " << v1 << ", " << v2 << ", " << v3 << ", " << v4 << "\n";

  io_service service(endpoints, YASIO_ARRAYSIZE(endpoints));

  resolv_fn_t resolv = [&](std::vector<ip::endpoint>& endpoints, const char* hostname,
                           unsigned short port) {
    return service.resolve(endpoints, hostname, port);
  };
  service.set_option(YOPT_S_RESOLV_FN, &resolv);

  std::vector<transport_handle_t> transports;

  deadline_timer udpconn_delay(service);
  deadline_timer udp_heartbeat(service);
  int total_bytes_transferred = 0;

  int max_request_count = 5;
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
            obs.write_bytes("GET /index.htm HTTP/1.1\r\n");

            obs.write_bytes("Host: www.ip138.com\r\n");

            obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                            "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/51.0.2704.106 Safari/537.36\r\n");
            obs.write_bytes("Accept: */*;q=0.8\r\n");
            obs.write_bytes("Connection: Close\r\n\r\n");

            service.write(transport, std::move(obs.buffer()));

            // service.close(transport, YWF_WRITE);
          }

          transports.push_back(transport);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost, %d bytes transferred\n", total_bytes_transferred);

        total_bytes_transferred = 0;
        if (--max_request_count > 0)
        {
          udpconn_delay.expires_from_now(std::chrono::seconds(1));
          udpconn_delay.async_wait_once([&]() { service.open(0); });
        }
        else
          service.stop();
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

  std::this_thread::sleep_for(std::chrono::seconds(1));
  service.open(0); // open http client
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
  yasioTest();

  return 0;
}
