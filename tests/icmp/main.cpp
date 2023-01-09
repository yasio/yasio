#include <stdint.h>
#include <thread>
#include "yasio/yasio.hpp"

#include "yasio/detail/ibstream.hpp"
#include "yasio/detail/obstream.hpp"

using namespace yasio;

#define ICMPTEST_PIN_HOST "www.ip138.com"
#define ICMPTEST_PIN "\"Hello!\" from yasio-3.39.x ping."

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
      return xxsocket::strerror(ec);
  }
}
} // namespace icmp
} // namespace yasio

static int icmp_ping(const ip::endpoint& endpoint, const std::chrono::microseconds& wtimeout, uint8_t& ttl, ip::endpoint& peer,
                     int& ec)
{
  enum
  {
    icmp_echo       = 8,
    icmp_echo_reply = 0,
    icmp_min_len    = 14,
  };

  xxsocket s;
#if defined(_WIN32)
  int socktype = SOCK_RAW;
#else
  int socktype = SOCK_DGRAM;
#endif
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

    if (n > sizeof(ip_hdr_st) + sizeof(icmp_hdr_st))
    {
      yasio::ibstream_view ibs(buf, sizeof(ip_hdr_st));
      ibs.advance(offsetof(ip_hdr_st, TTL));
      ttl                  = ibs.read_byte();
      const char* icmp_raw = (buf + sizeof(ip_hdr_st));
      u_short sum          = ip_chksum((uint8_t*)icmp_raw, n - sizeof(ip_hdr_st));

      if (sum != 0)
        return yasio::icmp::checksum_fail; // checksum failed

      icmp_hdr_st hdr;
      ibs.reset(icmp_raw, sizeof(icmp_hdr_st));
      hdr.type  = ibs.read<uint8_t>();
      hdr.code  = ibs.read<uint8_t>();
      hdr.sum   = ibs.read<uint16_t>();
      hdr.id    = ibs.read<uint16_t>();
      hdr.seqno = ibs.read<int16_t>();
      if (hdr.type != icmp_echo_reply)
      {
        yasio::icmp::errc::type_mismatch;
        return -1; // not echo reply
      }
      if (hdr.id != get_identifier())
      {
        ec = yasio::icmp::errc::identifier_mismatch;
        return -1; // id not equals
      }
      return n;
    }
    ec = yasio::errc::invalid_packet;
    return -1; // ip packet incorrect
  }

  ec = ec == 0 ? ETIMEDOUT : xxsocket::get_last_errno();
  return -1; // timeout
}

int main(int, char**)
{
  std::vector<ip::endpoint> endpoints;
  xxsocket::resolve(endpoints, ICMPTEST_PIN_HOST, 0);
  if (endpoints.empty())
  {
    fprintf(stderr, "Ping request could not find host %s. Please check the name and try again.\n", ICMPTEST_PIN_HOST);
    return -1;
  }

  for (int i = 0; i < 4; ++i)
  {
    ip::endpoint peer;
    uint8_t ttl = 0;
    int error   = 0;

    auto start_ms = yasio::clock();
    int n         = icmp_ping(endpoints[0], std::chrono::seconds(3), ttl, peer, error);
    if (n > 0)
      printf("Reply from %s: bytes=%d time=%dms TTL=%u\n", peer.ip().c_str(), n, static_cast<int>(yasio::clock() - start_ms), ttl);
    else
      printf("Ping %s fail, ec=%d, detail: ", ICMPTEST_PIN_HOST, error, yasio::icmp::strerror(error));
  }
  return 0;
}
