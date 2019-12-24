#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define YASIO_HAVE_KCP 1
#define YASIO_HEADER_ONLY 1
#define YASIO_DISABLE_SPSC_QUEUE 1

#include "yasio/yasio.hpp"
#include "yasio/ibstream.hpp"
#include "yasio/obstream.hpp"

using namespace yasio;
using namespace yasio::inet;

// KCP speed: 1.3MB/s(windows), 1.8MB/s(linux)
// UDP speed: 1.3GB/s(windows), 2.2GB/s(linux)
#define USE_KCP 1

#if USE_KCP
#  define TRANSFER_PROTOCOL YCM_KCP_CLIENT
#else
#  define TRANSFER_PROTOCOL YCM_UDP_CLIENT
#endif

static double s_time_elapsed  = 0;
static const double s_send_limit_time = 20; // max send time in seconds
static int s_bytes_sent = 0;

void udp_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  auto cb = [=] { udp_send_repeat_forever(service, thandle, obs); };

  service->write(thandle, obs->buffer(), cb);
}

void kcp_send_repeat_forever(io_service* service, transport_handle_t thandle, obstream* obs)
{
  service->write(thandle, obs->buffer());

  if (s_time_elapsed < s_send_limit_time)
    service->schedule(std::chrono::milliseconds(1),
                      [=](bool) { kcp_send_repeat_forever(service, thandle, obs); });
}

void start_sender(io_service& service)
{
  static const int PER_PACKET_SIZE =
      TRANSFER_PROTOCOL == YCM_KCP_CLIENT ? YASIO_SZ(60, k) : YASIO_SZ(63, k);
  static char buffer[PER_PACKET_SIZE];
  static obstream obs;
  obs.write_bytes(buffer, PER_PACKET_SIZE);
  deadline_timer timer(service);

  service.start_service([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          auto thandle = event->transport();

          if (TRANSFER_PROTOCOL == YCM_KCP_CLIENT)
            kcp_send_repeat_forever(&service, thandle, &obs);
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
  static long long total_bytes  = 0;
  static long long time_start   = yasio::highp_clock<>();
  static double last_print_time = 0;
  service.set_option(YOPT_S_DEFERRED_EVENT, 0);
  service.start_service([&](event_ptr event) {
    switch (event->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(event->packet());
        total_bytes += packet.size();
        auto time_elapsed = (yasio::highp_clock<>() - time_start) / 1000000.0;
        auto speed        = total_bytes / time_elapsed;
        if ((time_elapsed - last_print_time) > 0.5)
        {
          if (speed < 1024)
            printf("Speed: %gB/s, Total Time: %g(s), Total Bytes: %lld\n", speed, time_elapsed,
                   total_bytes);
          else if (speed < 1024 * 1024)
            printf("Speed: %.1lfKB/s, Total Time: %g(s), Total Bytes: %lld\n", speed / 1024,
                   time_elapsed, total_bytes);
          else if (speed < 1024 * 1024 * 1024)
            printf("Speed: %.1lfMB/s, Total Time: %g(s), Total Bytes: %lld\n", speed / 1024 / 1024,
                   time_elapsed, total_bytes);
          else
            printf("Speed: %.1lfGB/s, Total Time: %g(s), Total Bytes: %lld\n",
                   speed / 1024 / 1024 / 1024, time_elapsed, total_bytes);
          last_print_time = time_elapsed;
        }

        s_time_elapsed = time_elapsed;

        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
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

  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    sender.dispatch();
  }
  return 0;
}
