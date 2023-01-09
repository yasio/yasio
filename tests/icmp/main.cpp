#include <stdint.h>
#include <thread>
#include "yasio/yasio.hpp"

#include "yasio/detail/ibstream.hpp"
#include "yasio/detail/obstream.hpp"

using namespace yasio;

#define ICMPTEST_PIN_HOST "www.ip138.com"
#define ICMPTEST_PIN "yasio-3.39.x ping."

// ICMP header for both IPv4 and IPv6.
//
// The wire format of an ICMP header is:
//
// 0               8               16                             31
// +---------------+---------------+------------------------------+      ---
// |               |               |                              |       ^
// |     type      |     code      |          checksum            |       |
// |               |               |                              |       |
// +---------------+---------------+------------------------------+    8 bytes
// |                               |                              |       |
// |          identifier           |       sequence number        |       |
// |                               |                              |       v
// +-------------------------------+------------------------------+      ---

template <typename Iterator>
static void icmp_checksum(icmp_hdr_st& header, Iterator body_begin, Iterator body_end)
{
  unsigned int sum = (header.type << 8) + header.code + header.id + header.seqno;

  Iterator body_iter = body_begin;
  while (body_iter != body_end)
  {
    sum += (static_cast<unsigned char>(*body_iter++) << 8);
    if (body_iter != body_end)
      sum += static_cast<unsigned char>(*body_iter++);
  }

  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);

  header.sum = ~sum;
}

static uint16_t ip_chksum(const uint8_t* addr, int len)
{
  int nleft         = len;
  uint32_t sum      = 0;
  const uint16_t* w = (const uint16_t*)addr;
  uint16_t answer   = 0;

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

static unsigned short get_identifier()
{
#if defined(_WIN32)
  return static_cast<unsigned short>(::GetCurrentProcessId());
#else
  return static_cast<unsigned short>(::getpid());
#endif
}

namespace yasio
{
namespace icmp
{
enum errc
{
  checksum_fail = -201,
  type_mismatch,
  identifier_mismatch,
};
const char* strerror(int ec)
{
  switch (ec)
  {
    case checksum_fail:
      return "icmp: check sum fail!";
    case type_mismatch:
      return "icmp: type mismatch!";
    case identifier_mismatch:
      return "icmp: identifier mismatch!";
    default:
      return yasio::io_service::strerror(ec);
  }
}
} // namespace icmp
} // namespace yasio

static int icmp_ping(const ip::endpoint& endpoint, int socktype, const std::chrono::microseconds& wtimeout, uint8_t& ttl, ip::endpoint& peer, int& ec)
{
  enum
  {
    icmp_echo       = 8,
    icmp_echo_reply = 0,
    icmp_min_len    = 14,
  };

  xxsocket s;

  if (!s.open(endpoint.af(), socktype, IPPROTO_ICMP))
  {
    ec = xxsocket::get_last_errno();
    return -1;
  }

  static uint16_t s_seqno = 0;

  cxx17::string_view body = ICMPTEST_PIN; //"yasio-3.37.6";

  icmp_hdr_st hdr;
  hdr.id    = get_identifier();
  hdr.type  = icmp_echo;
  hdr.seqno = ++s_seqno;
  hdr.code  = 0;
  icmp_checksum(hdr, body.begin(), body.end());

  yasio::obstream obs;
  obs.write(hdr.type);
  obs.write(hdr.code);
  obs.write(hdr.sum);
  obs.write(hdr.id);
  obs.write(hdr.seqno);
  obs.write_bytes(body);

  auto icmp_request = std::move(obs.buffer());
  const size_t ip_pkt_len = sizeof(ip_hdr_st) + icmp_request.size();

  int n = s.sendto(icmp_request.data(), static_cast<int>(icmp_request.size()), endpoint);
  if (n < 0 && !xxsocket::not_send_error(ec = xxsocket::get_last_errno()))
    return -1;

  yasio::fd_set_adapter fds;
  fds.set(s.native_handle(), socket_event::read);
  int ret = fds.poll_io(static_cast<int>(wtimeout.count() / std::milli::den));
  if (ret > 0)
  {
    char buf[128];
    int n = s.recvfrom(buf, sizeof(buf), peer);

    const char* icmp_raw = nullptr;
    yasio::ibstream_view ibs;
    if (n == ip_pkt_len)
    { // icmp via SOCK_RAW
      // parse ttl and check ip checksum
      ibs.reset(buf, sizeof(ip_hdr_st));
      ibs.advance(offsetof(ip_hdr_st, TTL));
      ttl         = ibs.read_byte();
      icmp_raw    = (buf + sizeof(ip_hdr_st));
      u_short sum = ip_chksum((uint8_t*)icmp_raw, n - sizeof(ip_hdr_st));

      if (sum != 0)
        return yasio::icmp::checksum_fail; // checksum failed
    }
    else
    { // icmp via SOCK_DGRAM
      if (n < sizeof(icmp_hdr_st))
      {
        ec = yasio::errc::invalid_packet;
        return -1;
      }
      icmp_raw = buf;
      ttl      = 0;
    }

    icmp_hdr_st reply_hdr;
    ibs.reset(icmp_raw, sizeof(icmp_hdr_st));
    reply_hdr.type  = ibs.read<uint8_t>();
    reply_hdr.code  = ibs.read<uint8_t>();
    uint16_t sum    = ibs.read<uint16_t>();
    reply_hdr.id    = ibs.read<uint16_t>();
    reply_hdr.seqno = ibs.read<int16_t>();
    icmp_checksum(reply_hdr, buf + sizeof(icmp_hdr_st), buf + n);
    if (reply_hdr.type != icmp_echo_reply)
    {
      ec = yasio::icmp::errc::type_mismatch;
      return -1; // not echo reply
    }

    if (socktype == SOCK_RAW && reply_hdr.id != hdr.id)
    {
      ec = yasio::icmp::errc::identifier_mismatch;
      return -1; // id not equals
    }
    return n;
  }

  ec = ec == 0 ? ETIMEDOUT : xxsocket::get_last_errno();
  return -1; // timeout
}

int main(int argc, char** argv)
{
  const char* host    = argc > 1 ? argv[1] : ICMPTEST_PIN_HOST;
  const int max_times = argc > 2 ? atoi(argv[2]) : 4;

  std::vector<ip::endpoint> endpoints;
  xxsocket::resolve(endpoints, host, 0);
  if (endpoints.empty())
  {
    fprintf(stderr, "Ping request could not find host %s. Please check the name and try again.\n", host);
    return -1;
  }

  xxsocket schk;
  const int socktype = schk.open(AF_INET, SOCK_RAW, IPPROTO_ICMP) ? SOCK_RAW : SOCK_DGRAM;
  schk.close();

  const std::string remote_ip = endpoints[0].ip();
  fprintf(stdout, "Ping %s [%s] with %d bytes of data(%s):\n", host, remote_ip.c_str(),
          static_cast<int>(sizeof(ip_hdr_st) + sizeof(icmp_hdr_st) + sizeof(ICMPTEST_PIN) - 1),
          socktype == SOCK_RAW ? "SOCK_RAW" : "SOCK_DGRAM");

  for (int i = 0; i < max_times; ++i)
  {
    ip::endpoint peer;
    uint8_t ttl = 0;
    int error   = 0;

    auto start_ms = yasio::clock();
    int n         = icmp_ping(endpoints[0], socktype, std::chrono::seconds(3), ttl, peer, error);
    if (n > 0)
      fprintf(stdout, "Reply from %s: bytes=%d time=%dms TTL=%u\n", peer.ip().c_str(), n, static_cast<int>(yasio::clock() - start_ms), ttl);
    else
      fprintf(stderr, "Ping %s fail, ec=%d, detail: %s\n", host, error, yasio::icmp::strerror(error));
  }
  return 0;
}
