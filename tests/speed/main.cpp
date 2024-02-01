#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"

#if defined(YASIO_ENABLE_KCP)
#  include "kcp/ikcp.h"
#endif

#if YASIO_SSL_BACKEND != 0
#  include "sslcerts.hpp"
#endif

#if defined(_MSC_VER)
#  pragma comment(lib, "Winmm.lib")
#endif

using namespace yasio;

/*
Test detail, please see: https://github.com/yasio/yasio/blob/master/benchmark.md
*/

#define SPEEDTEST_VIA_SSL 0

#if defined(YASIO_ENABLE_UDS) && YASIO__HAS_UDS
#  define SPEEDTEST_VIA_UDS 1 // Now only support TCP/SOCK_STREAM
#else
#  define SPEEDTEST_VIA_UDS 0
#endif

#define SPEEDTEST_PROTO_TCP 1
#define SPEEDTEST_PROTO_UDP 2
#define SPEEDTEST_PROTO_KCP 3

#if SPEEDTEST_VIA_SSL
#  define SPEEDTEST_SSL_MASK YCM_SSL
#else
#  define SPEEDTEST_SSL_MASK 0
#endif

#define SPEEDTEST_TRANSFER_PROTOCOL SPEEDTEST_PROTO_KCP

#if SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_TCP
#  define SPEEDTEST_SERVER_KIND YCK_TCP_SERVER
#  define SPEEDTEST_CLIENT_KIND YCK_TCP_CLIENT | SPEEDTEST_SSL_MASK
#elif SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_UDP
#  define SPEEDTEST_SERVER_KIND YCK_UDP_SERVER
#  define SPEEDTEST_CLIENT_KIND YCK_UDP_CLIENT
#elif SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP
#  define SPEEDTEST_SERVER_KIND YCK_KCP_SERVER
#  define SPEEDTEST_CLIENT_KIND YCK_KCP_CLIENT
#else
#  error "please define SPEEDTEST_TRANSFER_PROTOCOL to one of SPEEDTEST_PROTO_TCP, SPEEDTEST_PROTO_UDP, SPEEDTEST_PROTO_KCP"
#endif

// speedtest kcp mtu to max mss of udp (65535 - 20(ip_hdr) - 8(udp_hdr))
// wsl2 mirrored network only allow udp mss to 1500 - 28 = 1472
#define SPEEDTEST_UDP_MSS 65507
#define SPEEDTEST_KCP_MTU SPEEDTEST_UDP_MSS
#define SPEEDTEST_KCP_MSS (SPEEDTEST_KCP_MTU - 24)

namespace speedtest
{
enum
{
  RECEIVER_PORT = 30001,
  SENDER_PORT   = RECEIVER_PORT,
#if !SPEEDTEST_VIA_UDS
  RECEIVER_CHANNEL_KIND = SPEEDTEST_SERVER_KIND | SPEEDTEST_SSL_MASK,
  SENDER_CHANNEL_KIND   = SPEEDTEST_CLIENT_KIND,
#else
  RECEIVER_CHANNEL_KIND = SPEEDTEST_SERVER_KIND | YCM_UDS,
  SENDER_CHANNEL_KIND   = SPEEDTEST_CLIENT_KIND | YCM_UDS,
#endif
};
#if !SPEEDTEST_VIA_UDS
#  define SPEEDTEST_SOCKET_NAME "127.0.0.1"
#  define SPEEDTEST_LISTEN_NAME "0.0.0.0"
#else

#  define SPEEDTEST_SOCKET_NAME "speedtest.socket"
#  define SPEEDTEST_LISTEN_NAME SPEEDTEST_SOCKET_NAME
#endif
} // namespace speedtest

static const double s_send_limit_time = 10; // max send time in seconds

static long long s_send_total_bytes = 0;
static long long s_recv_total_bytes = 0;

static double s_send_speed = 0; // bytes/s
static double s_recv_speed = 0;

static const long long s_kcp_send_interval = 100; // (us) in milliseconds
static const uint32_t s_kcp_conv           = 8633; // can be any, but must same with two endpoint

static const char* proto_name(int myproto)
{
  static const char* protos[] = {"null", "TCP", "UDP", "KCP"};
  return protos[myproto];
}

static void sbtoa(double speedInBytes, char* buf, size_t buf_len)
{
  double speedInBits = speedInBytes * 8;
  if (speedInBits < 1024)
    snprintf(buf, buf_len, "%gbits", speedInBits);
  else if (speedInBits < 1024 * 1024)
    snprintf(buf, buf_len, "%.1lfKbits", speedInBits / 1024);
  else if (speedInBits < 1024 * 1024 * 1024)
    snprintf(buf, buf_len, "%.1lfMbits", speedInBits / 1024 / 1024);
  else
    snprintf(buf, buf_len, "%.1lfGbits", speedInBits / 1024 / 1024 / 1024);
}

static void print_speed_detail(double interval, double time_elapsed)
{
  static double last_print_time     = 0;
  static long long send_total_bytes = 0;
  static long long recv_total_bytes = 0;
  if (((time_elapsed - last_print_time) > interval) && (send_total_bytes != s_send_total_bytes || recv_total_bytes != s_recv_total_bytes))
  {
    char str_send_speed[128], str_recv_speed[128];
    sbtoa(s_send_speed, str_send_speed, sizeof(str_send_speed));
    sbtoa(s_recv_speed, str_recv_speed, sizeof(str_recv_speed));
    printf("Speed: send=%s/s recv=%s/s, Total Time: %g(s), Total Bytes: send=%lld recv=%lld\n", str_send_speed, str_recv_speed, time_elapsed,
           s_send_total_bytes, s_recv_total_bytes);

    send_total_bytes = s_send_total_bytes;
    recv_total_bytes = s_recv_total_bytes;
    last_print_time  = time_elapsed;
  }
}

#if defined(YASIO_ENABLE_KCP)
void setup_kcp_transfer(transport_handle_t handle)
{
   auto kcp_handle = static_cast<io_transport_kcp*>(handle)->internal_object();
  ::ikcp_setmtu(kcp_handle, SPEEDTEST_KCP_MTU);
  ::ikcp_wndsize(kcp_handle, 256, 1024);
}
#endif

// The transport rely on low level proto UDP/TCP
void ll_send_repeated(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

  auto cb = [=](int, size_t bytes_transferred) {
    assert(bytes_transferred == obs->buffer().size());
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_total_bytes += bytes_transferred;
    s_send_speed = s_send_total_bytes / time_elapsed;
    ll_send_repeated(service, thandle, obs);
  };

  if (time_elapsed < s_send_limit_time)
    service->forward(thandle, obs->data(), obs->length(), cb);
}

void kcp_send_repeated(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

  highp_timer_ptr ignored_ret = service->schedule(std::chrono::microseconds(s_kcp_send_interval), [=](io_service&) {
    s_send_total_bytes += service->write(thandle, obs->buffer());
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_speed = s_send_total_bytes / time_elapsed;
    print_speed_detail(0.5, time_elapsed);
    if (time_elapsed < s_send_limit_time)
      return false;
    printf("===> sender finished!\n");
    return true; // tell this timer finished
  });
}

void start_sender(io_service& service)
{
  static const int PER_PACKET_SIZE = SPEEDTEST_KCP_MSS;
  static char buffer[PER_PACKET_SIZE];
  static obstream obs;
  obs.write_bytes(buffer, PER_PACKET_SIZE);

  service.set_option(YOPT_S_HRES_TIMER, 1);
  service.set_option(YOPT_S_FORWARD_PACKET, 1);

#if YASIO_SSL_BACKEND != 0
  service.set_option(YOPT_S_SSL_CACERT, SSLTEST_CACERT);
#endif
  service.start([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto thandle = event->transport();

          if (SPEEDTEST_TRANSFER_PROTOCOL != SPEEDTEST_PROTO_TCP)
          {
            // because some system's default sndbuf of udp is less than 64k, such as macOS.
            int sndbuf = 65536;
            xxsocket::set_last_errno(0);
            service.set_option(YOPT_B_SOCKOPT, static_cast<io_base*>(thandle), SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int));
            int ec = xxsocket::get_last_errno();
            if (ec != 0)
              YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));
          }
#if defined(YASIO_ENABLE_KCP)
          if (SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP)
          {
            setup_kcp_transfer(thandle);
            kcp_send_repeated(&service, thandle, &obs);
          }
          else
            ll_send_repeated(&service, thandle, &obs);
#else
          ll_send_repeated(&service, thandle, &obs);
#endif
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  printf("Start trasnfer test via %s after 170ms...\n", proto_name(SPEEDTEST_TRANSFER_PROTOCOL));
  std::this_thread::sleep_for(std::chrono::milliseconds(170));

#if SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP
  service.set_option(YOPT_C_KCP_CONV, 0, s_kcp_conv);
#endif

#if SPEEDTEST_TRANSFER_PROTOCOL != SPEEDTEST_PROTO_TCP
  service.set_option(YOPT_C_LOCAL_HOST, 0, SPEEDTEST_SOCKET_NAME);
#endif

  service.open(0, speedtest::SENDER_CHANNEL_KIND);
}

void start_receiver(io_service& service)
{
  static long long time_start   = yasio::highp_clock<>();
  static double last_print_time = 0;
  service.set_option(YOPT_S_FORWARD_PACKET, 1);
  service.set_option(YOPT_C_MOD_FLAGS, 0, YCF_REUSEADDR, 0);
  service.set_option(YOPT_S_HRES_TIMER, 1);
#if YASIO_SSL_BACKEND != 0
  service.set_option(YOPT_S_SSL_CERT, SSLTEST_CERT, SSLTEST_PKEY);
#endif
  service.start([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        s_recv_total_bytes += event->packet_view().size();
        auto time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
        s_recv_speed      = s_recv_total_bytes / time_elapsed;
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          if (SPEEDTEST_TRANSFER_PROTOCOL != SPEEDTEST_PROTO_TCP)
          {
            int sndbuf = 65536;
            xxsocket::set_last_errno(0);
            service.set_option(YOPT_B_SOCKOPT, static_cast<io_base*>(event->transport()), SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(int));
            int ec = xxsocket::get_last_errno();
            if (ec != 0)
              YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));
          }
#if defined(YASIO_ENABLE_KCP)
          if (SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP)
            setup_kcp_transfer(event->transport());
#endif
          printf("The sender connected...\n");
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

#if SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP
  service.set_option(YOPT_C_KCP_CONV, 0, s_kcp_conv);
#endif

  service.open(0, speedtest::RECEIVER_CHANNEL_KIND);
}

int main(int argc, char** argv)
{
  io_hostent receiver_ep(SPEEDTEST_LISTEN_NAME, speedtest::RECEIVER_PORT), sender_ep(SPEEDTEST_SOCKET_NAME, speedtest::SENDER_PORT);
  io_service receiver(&receiver_ep, 1), sender(&sender_ep, 1);

  const char* mode = "host";
  if (argc > 1)
    mode = argv[1];

  if (cxx20::ic::iequals(mode, "server"))
    start_receiver(receiver);
  else if (cxx20::ic::iequals(mode, "client"))
    start_sender(sender);
  else
  {
    start_receiver(receiver);
    start_sender(sender);
  }

  static long long time_start = yasio::highp_clock<>();
  while (true)
  {
    // main thread, print speed only, so sleep 200ms
    // !Note: sleep(10ms) will cost 0.3%CPU, 200ms %0CPU, tested on macbook pro 2019
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    print_speed_detail(0.5, time_elapsed);
  }
  return 0;
}
