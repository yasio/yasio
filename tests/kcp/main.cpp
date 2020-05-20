#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

#include "yasio/kcp/ikcp.h"

using namespace yasio;
using namespace yasio::inet;

// KCP speed: 1.3MB/s(windows), 1.8MB/s(linux)
// UDP speed: 1.3GB/s(windows), 2.2GB/s(linux)
#define USE_KCP 1

#if USE_KCP
#  define TRANSFER_PROTOCOL YCK_KCP_CLIENT
#else
#  define TRANSFER_PROTOCOL YCK_UDP_CLIENT
#endif

static const double s_send_limit_time = 10; // max send time in seconds

static long long s_send_total_bytes = 0;
static long long s_recv_total_bytes = 0;

static double s_send_speed = 0; // bytes/s
static double s_recv_speed = 0;

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
  static double last_print_time = 0;
  if ((time_elapsed - last_print_time) > interval)
  {
    char str_send_speed[128], str_recv_speed[128];
    sbtoa(s_send_speed, str_send_speed);
    sbtoa(s_recv_speed, str_recv_speed);

    printf("Speed: send=%s/s recv=%s/s, Total Time: %g(s), Total Bytes: send=%lld recv=%lld\n",
           str_send_speed, str_recv_speed, time_elapsed, s_send_total_bytes, s_recv_total_bytes);

    last_print_time = time_elapsed;
  }
}

void setup_kcp_transfer(transport_handle_t handle)
{
  auto kcp_handle = static_cast<io_transport_kcp*>(handle)->internal_object();
  ::ikcp_setmtu(kcp_handle, YASIO_SZ(63, k));
  ::ikcp_wndsize(kcp_handle, 4096, 8192);
  kcp_handle->interval = 0;
}

void udp_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

  s_send_total_bytes += obs->length();
  auto cb                       = [=](int, size_t bytes_transferred) {
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_speed = s_send_total_bytes / time_elapsed;
    udp_send_repeat_forever(service, thandle, obs);
  };

  if (time_elapsed < s_send_limit_time)
     service->write(thandle, obs->buffer(), cb);
}

void kcp_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  static long long time_start   = yasio::highp_clock<>();
  static double time_elapsed    = 0;
  static double last_print_time = 0;

  while (time_elapsed < s_send_limit_time)
  {
    s_send_total_bytes += service->write(thandle, obs->buffer());
    time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
    s_send_speed = s_send_total_bytes / time_elapsed;
    print_speed_detail(0.5, time_elapsed);

#if !defined(_WIN32)
    std::this_thread::sleep_for(std::chrono::microseconds(3000));
#endif
  }
}

void start_sender(io_service& service)
{
  static const int PER_PACKET_SIZE =
      TRANSFER_PROTOCOL == YCK_KCP_CLIENT ? YASIO_SZ(62, k) : YASIO_SZ(63, k);
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
          // because some system's default sndbuf of udp is less than 64k, such as macOS.
          int sndbuf = 65536;
          xxsocket::set_last_errno(0);
          service.set_option(YOPT_SOCKOPT, static_cast<io_base*>(thandle), SOL_SOCKET, SO_SNDBUF,
                             &sndbuf, sizeof(int));
          int ec = xxsocket::get_last_errno();
          if (ec != 0)
            YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));

          if (TRANSFER_PROTOCOL == YCK_KCP_CLIENT)
          {
            setup_kcp_transfer(thandle);
            kcp_send_repeat_forever(&service, thandle, &obs);
          }
          else
            udp_send_repeat_forever(&service, thandle, &obs);
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(170));

  service.set_option(YOPT_C_LOCAL_PORT, 0, 30001);
  service.open(0, TRANSFER_PROTOCOL);
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
          int sndbuf = 65536;
          xxsocket::set_last_errno(0);
          service.set_option(YOPT_SOCKOPT, static_cast<io_base*>(event->transport()), SOL_SOCKET,
                             SO_SNDBUF, &sndbuf, sizeof(int));
          int ec = xxsocket::get_last_errno();
          if (ec != 0)
            YASIO_LOG("set_option failed, ec=%d, detail:%s", ec, xxsocket::strerror(ec));

          if (TRANSFER_PROTOCOL == YCK_KCP_CLIENT)
            setup_kcp_transfer(event->transport());
          printf("start recive data...\n");
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  service.set_option(YOPT_C_LOCAL_PORT, 0, 30002);
  service.open(0, TRANSFER_PROTOCOL);
}

int main(int, char**)
{
  io_hostent receiver_ep("127.0.0.1", 30001), sender_ep("127.0.0.1", 30002);
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
