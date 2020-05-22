#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

#include "yasio/kcp/ikcp.h"

#if defined(_MSC_VER)
#  pragma comment(lib, "Winmm.lib")
#endif

using namespace yasio;
using namespace yasio::inet;

/*
Devices:
  - Windows: Intel(R) Core(TM) i7-9700 CPU @ 3.00GHz / Windows 10(10.0.19041.264)
  - Linux: Intel(R) Xeon(R) Platinum 8163 CPU @ 2.50GHz / Ubuntu 20.04

Architecture: X64

Compiling:
  - Windows: VS2019 MSVC 14.25.28610
    Optimize Flag: /O2
    Commands:
      - mkdir build/build_w64
      - cd build/build_w64
      - cmake ../../
      - cmake --build . --config Release --target speedtest
  - Linux: gcc version 9.3.0 (Ubuntu 9.3.0-10ubuntu2)
    Optimize Flag: /O3
    Commands:
      - mkdir -p build/build_linux
      - cd build/build_linux
      - cmake ../../ -DCMAKE_BUILD_TYPE=Release
      - cmake --build . --config Release --target speedtest

Total Time: 10(s)

Results:
  - TCP speed: 2.8GB/s(Windows), 2.3GB/s(Linux)
  - UDP speed: 2.8GB/s(windows), 2.6GB/s(Linux)
  - KCP speed: 29MB/s(Windows), 16~46MB/s(linux)
*/

#define SPEEDTEST_PROTO_TCP 1
#define SPEEDTEST_PROTO_UDP 2
#define SPEEDTEST_PROTO_KCP 3

#define SPEEDTEST_TRANSFER_PROTOCOL SPEEDTEST_PROTO_KCP

#if SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_TCP
#  define SPEEDTEST_DEFAULT_KIND YCK_TCP_CLIENT
#elif SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_UDP
#  define SPEEDTEST_DEFAULT_KIND YCK_UDP_CLIENT
#elif SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP
#  define SPEEDTEST_DEFAULT_KIND YCK_KCP_CLIENT
#else
#  error                                                                                           \
      "please define SPEEDTEST_TRANSFER_PROTOCOL to one of SPEEDTEST_PROTO_TCP, SPEEDTEST_PROTO_UDP, SPEEDTEST_PROTO_KCP"
#endif

#if SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_TCP
namespace speedtest
{
enum
{
  RECEIVER_PORT         = 3001,
  SENDER_PORT           = RECEIVER_PORT,
  RECEIVER_CHANNEL_KIND = YCK_TCP_SERVER,
  SENDER_CHANNEL_KIND   = SPEEDTEST_DEFAULT_KIND,
};
}
#else
namespace speedtest
{
enum
{
  RECEIVER_PORT         = 3001,
  SENDER_PORT           = 3002,
  RECEIVER_CHANNEL_KIND = SPEEDTEST_DEFAULT_KIND,
  SENDER_CHANNEL_KIND   = SPEEDTEST_DEFAULT_KIND,
};
}
#endif

static const double s_send_limit_time = 10; // max send time in seconds

static long long s_send_total_bytes = 0;
static long long s_recv_total_bytes = 0;

static double s_send_speed = 0; // bytes/s
static double s_recv_speed = 0;

static const char* proto_name(int myproto)
{
  static const char* protos[] = {"null", "TCP", "UDP", "KCP"};
  return protos[myproto];
}

static void sbtoa(double speedInBytes, char* buf)
{
  if (speedInBytes < 1024)
    sprintf(buf, "%gB", speedInBytes);
  else if (speedInBytes < 1024 * 1024)
    sprintf(buf, "%.1lfKB", speedInBytes / 1024);
  else if (speedInBytes < 1024 * 1024 * 1024)
    sprintf(buf, "%.1lfMB", speedInBytes / 1024 / 1024);
  else
    sprintf(buf, "%.1lfGB", speedInBytes / 1024 / 1024 / 1024);
}

static void print_speed_detail(double interval, double time_elapsed)
{
  static double last_print_time     = 0;
  static long long send_total_bytes = 0;
  static long long recv_total_bytes = 0;
  if (((time_elapsed - last_print_time) > interval) &&
      (send_total_bytes != s_send_total_bytes || recv_total_bytes != s_recv_total_bytes))
  {
    char str_send_speed[128], str_recv_speed[128];
    sbtoa(s_send_speed, str_send_speed);
    sbtoa(s_recv_speed, str_recv_speed);

    printf("Speed: send=%s/s recv=%s/s, Total Time: %g(s), Total Bytes: send=%lld recv=%lld\n",
           str_send_speed, str_recv_speed, time_elapsed, s_send_total_bytes, s_recv_total_bytes);

    send_total_bytes = s_send_total_bytes;
    recv_total_bytes = s_recv_total_bytes;
    last_print_time  = time_elapsed;
  }
}

void setup_kcp_transfer(transport_handle_t handle)
{
  auto kcp_handle = static_cast<io_transport_kcp*>(handle)->internal_object();
  ::ikcp_setmtu(kcp_handle, YASIO_SZ(63, k));
  ::ikcp_wndsize(kcp_handle, 4096, 8192);
  kcp_handle->interval = 0;
}

// The transport rely on low level proto UDP/TCP
void ll_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

  auto cb = [=](int, size_t bytes_transferred) {
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_total_bytes += bytes_transferred;
    s_send_speed = s_send_total_bytes / time_elapsed;
    ll_send_repeat_forever(service, thandle, obs);
  };

  if (time_elapsed < s_send_limit_time)
    service->write(thandle, obs->buffer(), cb);
}

void kcp_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

#if defined(_MSC_VER)
  ///////////////////////////////////////////////////////////////////////////
  /////////////// changing timer resolution
  ///////////////////////////////////////////////////////////////////////////
  UINT TARGET_RESOLUTION = 1; // 1 millisecond target resolution
  TIMECAPS tc;
  UINT wTimerRes = 0;
  if (TIMERR_NOERROR == timeGetDevCaps(&tc, sizeof(TIMECAPS)))
  {
    wTimerRes = (std::min)((std::max)(tc.wPeriodMin, TARGET_RESOLUTION), tc.wPeriodMax);
    timeBeginPeriod(wTimerRes);
  }
#endif

  while (time_elapsed < s_send_limit_time)
  {
    s_send_total_bytes += service->write(thandle, obs->buffer());
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_speed = s_send_total_bytes / time_elapsed;
    print_speed_detail(0.5, time_elapsed);

    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }

#if defined(_MSC_VER)
  ///////////////////////////////////////////////////////////////////////////
  /////////////// restoring timer resolution
  ///////////////////////////////////////////////////////////////////////////
  if (wTimerRes != 0)
  {
    timeEndPeriod(wTimerRes);
  }
#endif
}

void start_sender(io_service& service)
{
  static const int PER_PACKET_SIZE = YASIO_SZ(62, k);
  static char buffer[PER_PACKET_SIZE];
  static obstream obs;
  obs.write_bytes(buffer, PER_PACKET_SIZE);
  deadline_timer timer(service);

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
            service.set_option(YOPT_B_SOCKOPT, static_cast<io_base*>(thandle), SOL_SOCKET, SO_SNDBUF,
                               &sndbuf, sizeof(int));
            int ec = xxsocket::get_last_errno();
            if (ec != 0)
              YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));
          }
          if (SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP)
          {
            setup_kcp_transfer(thandle);
            kcp_send_repeat_forever(&service, thandle, &obs);
          }
          else
            ll_send_repeat_forever(&service, thandle, &obs);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  printf("Start trasnfer test via %s after 170ms...\n", proto_name(SPEEDTEST_TRANSFER_PROTOCOL));
  std::this_thread::sleep_for(std::chrono::milliseconds(170));
#if SPEEDTEST_TRANSFER_PROTOCOL != SPEEDTEST_PROTO_TCP
  service.set_option(YOPT_C_LOCAL_PORT, 0, speedtest::RECEIVER_PORT);
#endif
  service.open(0, speedtest::SENDER_CHANNEL_KIND);
}

static io_service* s_sender;
void start_receiver(io_service& service)
{
  static long long time_start   = yasio::highp_clock<>();
  static double last_print_time = 0;
  service.set_option(YOPT_S_DEFERRED_EVENT, 0);
  service.start([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        s_recv_total_bytes += event->packet().size();
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
            service.set_option(YOPT_B_SOCKOPT, static_cast<io_base*>(event->transport()),
                               SOL_SOCKET,
                               SO_SNDBUF, &sndbuf, sizeof(int));
            int ec = xxsocket::get_last_errno();
            if (ec != 0)
              YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));
          }
          if (SPEEDTEST_TRANSFER_PROTOCOL == SPEEDTEST_PROTO_KCP)
            setup_kcp_transfer(event->transport());
          printf("The sender connected...\n");
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

#if SPEEDTEST_TRANSFER_PROTOCOL != SPEEDTEST_PROTO_TCP
  service.set_option(YOPT_C_LOCAL_PORT, 0, speedtest::SENDER_PORT);
#endif
  service.open(0, speedtest::RECEIVER_CHANNEL_KIND);
}

int main(int, char**)
{
  io_hostent receiver_ep("127.0.0.1", speedtest::RECEIVER_PORT),
      sender_ep("127.0.0.1", speedtest::SENDER_PORT);
  io_service receiver(&receiver_ep, 1), sender(&sender_ep, 1);

  s_sender = &sender;
  start_receiver(receiver);
  start_sender(sender);

  static long long time_start = yasio::highp_clock<>();
  while (true)
  { // main thread, print speed only
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sender.dispatch();

    auto time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    print_speed_detail(0.5, time_elapsed);
  }
  return 0;
}
