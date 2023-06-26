/******************************************************************
 * Notes:
 * GitHub hosts Linux and Windows runners on Standard_DS2_v2 virtual machines in Microsoft Azure with the GitHub Actions runner application installed.
 * And due to security policy Azure blocks ICMP by default. Hence, you cannot get ICMP answer in workflow.
 *   refer to: https://github.com/orgs/community/discussions/26184
 */
#include <stdint.h>
#include <thread>
#include "yasio/yasio.hpp"

using namespace yasio;

#define ICMPTEST_PIN_HOST "www.ip138.com"
#define ICMPTEST_PIN "yasio 4.0.x ping."
#define ICMPTEST_PIN_LEN (sizeof(ICMPTEST_PIN) - 1)
#define ICMPTEST_MAX_LEN 64 // max ip packet len

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
  sequence_number_mismatch,
};
const char* strerror(int ec)
{
  switch (ec)
  {
    case ETIMEDOUT:
      return "request timed out.";
    case checksum_fail:
      return "icmp: check sum fail.";
    case type_mismatch:
      return "icmp: type mismatch.";
    case identifier_mismatch:
      return "icmp: identifier mismatch.";
    case sequence_number_mismatch:
      return "icmp: sequence number mismatch.";
    default:
      return yasio::io_service::strerror(ec);
  }
}
} // namespace icmp
} // namespace yasio

static int icmp_ping(yasio::io_watcher& watcher, const ip::endpoint& endpoint, int socktype, const std::chrono::microseconds& wtimeout,
                     yasio::byte_buffer& body, ip::endpoint& peer, icmp_hdr_st& reply_hdr, uint8_t& ttl, int& ec)
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

  icmp_hdr_st req_hdr = {0};

  req_hdr.type  = icmp_echo;
  req_hdr.seqno = ++s_seqno;
  req_hdr.code  = 0;

#if !defined(__linux__)
  req_hdr.id = get_identifier();
  icmp_checksum(req_hdr, body.begin(), body.end());
#else
  /** Linux:
   * SOCK_DGRAM
   * This allows you to only send ICMP echo requests,
   * The kernel will handle it specially (match request/responses, fill in the checksum and identifier).
   */
  if (socktype == SOCK_RAW)
  {
    req_hdr.id = get_identifier();
    icmp_checksum(req_hdr, body.begin(), body.end());
  }
#endif

  yasio::obstream obs;
  obs.write(req_hdr.type);
  obs.write(req_hdr.code);
  obs.write(req_hdr.sum);
  obs.write(req_hdr.id);
  obs.write(req_hdr.seqno);
  obs.write_bytes(body.data(), static_cast<int>(body.size()));

  auto icmp_request       = std::move(obs.buffer());
  const size_t ip_pkt_len = sizeof(ip_hdr_st) + icmp_request.size();

  int n = s.sendto(icmp_request.data(), static_cast<int>(icmp_request.size()), endpoint);
  if (n < 0 && !xxsocket::not_send_error(ec = xxsocket::get_last_errno()))
    return -1;

  s.set_nonblocking(true);

  watcher.mod_event(s.native_handle(), socket_event::read, 0);
  int ret = watcher.poll_io(wtimeout.count());
  watcher.mod_event(s.native_handle(), 0, socket_event::read);
  if (ret > 0 && watcher.is_ready(s.native_handle(), socket_event::read))
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

#if !defined(__linux__)
    if (reply_hdr.id != req_hdr.id)
    {
      ec = yasio::icmp::errc::identifier_mismatch;
      return -1; // id not equals
    }
#else
    // SOCK_DGRAM on Linux: kernel handle to fill identifier, so don't check
    // if socktype == SOCK_DGRAM
    if (socktype == SOCK_RAW && reply_hdr.id != req_hdr.id)
    {
      ec = yasio::icmp::errc::identifier_mismatch;
      return -1; // id not equals
    }
#endif
    if (reply_hdr.seqno != req_hdr.seqno)
    {
      ec = yasio::icmp::errc::sequence_number_mismatch;
      return -1;
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

  printf("sizeof(intptr_t)=%u, sizeof(wchar_t)=%u, sizeof(long)=%u\n", static_cast<int>(sizeof(intptr_t)), static_cast<int>(sizeof(wchar_t)),
         static_cast<int>(sizeof(long)));

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

  const size_t ip_icmp_hdr_len = sizeof(ip_hdr_st) + sizeof(icmp_hdr_st);

  // some host router require ip packet of icmp must align with 32 bytes, otherwise will be dropped by router
  const size_t ip_pkt_len = std::min<size_t>(YASIO_SZ_ALIGN(static_cast<int>(ip_icmp_hdr_len + ICMPTEST_PIN_LEN), 32), ICMPTEST_MAX_LEN);

  const size_t icmp_data_len = ip_pkt_len - ip_icmp_hdr_len;
  yasio::byte_buffer icmp_body(icmp_data_len, 0, std::true_type{});

  memcpy(icmp_body.data(), ICMPTEST_PIN, std::min<int>(icmp_data_len,ICMPTEST_PIN_LEN));

  const std::string remote_ip = endpoints[0].ip();
  fprintf(stdout, "Ping %s [%s] with %d bytes of data(%s):\n", host, remote_ip.c_str(), static_cast<int>(ip_pkt_len),
          socktype == SOCK_RAW ? "SOCK_RAW" : "SOCK_DGRAM");

  icmp_hdr_st reply_hdr;

  yasio::io_watcher watcher;
  for (int i = 0; i < max_times; ++i)
  {
    ip::endpoint peer;
    uint8_t ttl = 0;
    int error   = 0;

    auto start_ms = yasio::highp_clock();
    int n         = icmp_ping(watcher, endpoints[0], socktype, std::chrono::seconds(3), icmp_body, peer, reply_hdr, ttl, error);
    if (n > 0)
      fprintf(stdout, "Reply from %s: bytes=%d icmp_seq=%u ttl=%u id=%u time=%.1lfms\n", peer.ip().c_str(), n, static_cast<unsigned int>(reply_hdr.seqno),
              static_cast<unsigned int>(ttl), static_cast<unsigned int>(reply_hdr.id), (yasio::highp_clock() - start_ms) / 1000.0);
    else
      fprintf(stderr, "Ping %s [%s] fail, ec=%d, detail: %s\n", host, remote_ip.c_str(), error, yasio::icmp::strerror(error));
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
