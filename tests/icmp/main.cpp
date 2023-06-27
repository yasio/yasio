/******************************************************************
 * Notes:
 * GitHub hosts Linux and Windows runners on Standard_DS2_v2 virtual machines in Microsoft Azure with the GitHub Actions runner application installed.
 * And due to security policy Azure blocks ICMP by default. Hence, you cannot get ICMP answer in workflow.
 *   refer to: https://github.com/orgs/community/discussions/26184
 */
#include <stdint.h>
#include <thread>
#include "yasio/yasio.hpp"

#define ICMPTEST_PIN_HOST "www.ip138.com"
#define ICMPTEST_PIN "yasio-4.0.x ping."
#define ICMPTEST_PIN_LEN (sizeof(ICMPTEST_PIN) - 1)
#define ICMPTEST_MAX_LEN 64 // max ip packet len
#define ICMPTEST_ENC_TSC

namespace yasio
{
namespace icmp
{
// icmp message type refer to RFC792: https://datatracker.ietf.org/doc/html/rfc792
enum
{
  icmp_echo_reply        = 0,
  icmp_dest_unreachable  = 3,
  icmp_source_quench     = 4,
  icmp_redirect          = 5,
  icmp_echo              = 8,
  icmp_time_exceeded     = 11,
  icmp_parameter_problem = 12,
  icmp_timestamp         = 13,
  icmp_timestamp_reply   = 14,
  icmp_info_request      = 15,
  icmp_info_reply        = 16,
};
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
struct timeval32 {
  int tv_sec;
  int tv_usec;
};
} // namespace icmp
} // namespace yasio

using namespace yasio;

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

/*
 * The default inetutils-ping of system:
 *   Windows: 40(60) bytes: Display icmp data len only, i.e. 32(40 - icmp_hdr(8)) bytes
 *   Linux/macOS: 56(84) bytes: icmp_data(48), ip_hdr(20), icmp_hdr(8) + icmp_timestamp(8)
 */
class icmp_ping_helper {
public:
  icmp_ping_helper()
  {

    xxsocket schk;
    socktype_ = schk.open(AF_INET, SOCK_RAW, IPPROTO_ICMP) ? SOCK_RAW : SOCK_DGRAM;

    ip_total_len_ = static_cast<int>(ip_icmp_hdr_len_ + ICMPTEST_PIN_LEN);

#if defined(ICMPTEST_ENC_TSC)
    if (yasio::time_now() < static_cast<yasio::highp_time_t>((std::numeric_limits<int>::max)() - 86400))
    {
      ip_total_len_ += sizeof(yasio::icmp::timeval32);
      enc_tsc_ = true;
    }
#endif

    ip_total_len_ = YASIO_SZ_ALIGN(ip_total_len_, 8);

    // !!!Notes: some host router require (ip_total_length % 2 == 0), otherwise will be dropped by router
    // 48 bytes
    ip_total_len_ = std::min<size_t>(ip_total_len_, ICMPTEST_MAX_LEN);

    // 20 bytes
    icmp_total_len_ = ip_total_len_ - sizeof(ip_hdr_st);
  }

  int ping(yasio::io_watcher& watcher, const ip::endpoint& endpoint, const std::chrono::microseconds& wtimeout, ip::endpoint& peer, icmp_hdr_st& reply_hdr,
           uint8_t& ttl, int& ec)
  {
    xxsocket s;

    if (!s.open(endpoint.af(), socktype_, IPPROTO_ICMP))
    {
      ec = xxsocket::get_last_errno();
      return -1;
    }
    
    static uint16_t s_seqno = 0;

    icmp_hdr_st req_hdr = {0};

    req_hdr.type  = icmp::icmp_echo;
    req_hdr.seqno = ++s_seqno;
    req_hdr.code  = 0;

    // icmp header
    icmp_pkt_.clear();
    icmp_pkt_.write(req_hdr.type);
    icmp_pkt_.write(req_hdr.code);
    auto sum_off = icmp_pkt_.push<unsigned short>();
    auto id_off  = icmp_pkt_.push<unsigned short>();
    icmp_pkt_.write(req_hdr.seqno);

    // icmp body
    if (enc_tsc_)
    {
      auto tsc = yasio::highp_clock<yasio::system_clock_t>();
      // !!! after 01/19/2038 3:14:07 the 32bit tv_sec will overflow
      icmp::timeval32 tv{static_cast<int>(tsc / std::micro::den), static_cast<int>(tsc % std::micro::den)};
      icmp_pkt_.write_bytes(&tv, static_cast<int>(sizeof(tv)));
    }
    icmp_pkt_.write_bytes(ICMPTEST_PIN, ICMPTEST_PIN_LEN);

    // fill bytes
    if (icmp_pkt_.length() < icmp_total_len_)
      icmp_pkt_.fill_bytes(static_cast<int>(icmp_total_len_ - icmp_pkt_.length()), 'F');

    // id,sum
#if !defined(__linux__)
    req_hdr.id = get_identifier();
    icmp_checksum(req_hdr, icmp_pkt_.data() + sizeof(icmp_hdr_st), icmp_pkt_.buffer().end());
#else
    /** Linux:
     * SOCK_DGRAM
     * This allows you to only send ICMP echo requests,
     * The kernel will handle it specially (match request/responses, fill in the checksum and identifier).
     */
    if (socktype_ == SOCK_RAW)
    {
      req_hdr.id = get_identifier();
      icmp_checksum(req_hdr, icmp_pkt_.data() + sizeof(icmp_hdr_st), icmp_pkt_.buffer().end());
    }
#endif
    icmp_pkt_.pop<unsigned short>(id_off, req_hdr.id);
    icmp_pkt_.pop<unsigned short>(sum_off, req_hdr.sum);

    auto icmp_request       = icmp_pkt_.buffer();
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
      if (reply_hdr.type != icmp::icmp_echo_reply)
      {
        ec = icmp::errc::type_mismatch;
        return -1; // not echo reply
      }

#if !defined(__linux__)
      if (reply_hdr.id != req_hdr.id)
      {
        ec = icmp::errc::identifier_mismatch;
        return -1; // id not equals
      }
#else
      // SOCK_DGRAM on Linux: kernel handle to fill identifier, so don't check
      if (socktype_ == SOCK_RAW && reply_hdr.id != req_hdr.id)
      {
        ec = icmp::errc::identifier_mismatch;
        return -1; // id not equals
      }
#endif
      if (reply_hdr.seqno != req_hdr.seqno)
      {
        ec = icmp::errc::sequence_number_mismatch;
        return -1;
      }
      return n;
    }

    ec = ec == 0 ? ETIMEDOUT : xxsocket::get_last_errno();
    return -1; // timeout
  }

  int socktype_;

  static const size_t ip_icmp_hdr_len_ = sizeof(ip_hdr_st) + sizeof(icmp_hdr_st);
  // !!!Notes: some host router require (ip_total_length % 2 == 0), otherwise will be dropped by router
  size_t ip_total_len_;
  size_t icmp_total_len_;
  yasio::obstream icmp_pkt_;
  bool enc_tsc_ = false;
};

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

  icmp_ping_helper helper;

  const std::string remote_ip = endpoints[0].ip();

  fprintf(stdout, "Ping %s [%s] with %d(%d) bytes of data(%s):\n", host, remote_ip.c_str(), static_cast<int>(helper.icmp_total_len_),
          static_cast<int>(helper.ip_total_len_), helper.socktype_ == SOCK_RAW ? "SOCK_RAW" : "SOCK_DGRAM");

  icmp_hdr_st reply_hdr;

  yasio::io_watcher watcher;
  for (int i = 0; i < max_times; ++i)
  {
    ip::endpoint peer;
    uint8_t ttl = 0;
    int error   = 0;

    auto start_ms = yasio::highp_clock();
    int n         = helper.ping(watcher, endpoints[0], std::chrono::seconds(3), peer, reply_hdr, ttl, error);
    if (n > 0)
      fprintf(stdout, "Reply from %s: bytes=%d icmp_seq=%u ttl=%u id=%u time=%.1lfms\n", peer.ip().c_str(), n, static_cast<unsigned int>(reply_hdr.seqno),
              static_cast<unsigned int>(ttl), static_cast<unsigned int>(reply_hdr.id), (yasio::highp_clock() - start_ms) / 1000.0);
    else
      fprintf(stderr, "Ping %s [%s] fail, ec=%d, detail: %s\n", host, remote_ip.c_str(), error, yasio::icmp::strerror(error));
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return 0;
}
