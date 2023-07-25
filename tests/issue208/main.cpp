#include "yasio/yasio.hpp"

using namespace yasio;
deadline_timer_ptr udpHeartTimer = nullptr;

void start_exprie_timer();
timer_cb_t create_timer_cb();

io_service& get_service() { return *yasio_shared_service(); }

static yasio::highp_time_t getTimeStamp()
{
  return yasio::highp_clock<yasio::system_clock_t>() / 1000;
}

void start_exprie_timer()
{
  if (udpHeartTimer)
  {
    udpHeartTimer->expires_from_now();
    udpHeartTimer->async_wait(create_timer_cb());
  }
  else
  {
    udpHeartTimer = get_service().schedule(std::chrono::seconds(4), create_timer_cb());
  }
}

timer_cb_t create_timer_cb()
{
  return [](io_service& s) {
    if (udpHeartTimer->expired())
    {
      printf("udp timer %lld\n", getTimeStamp());
      start_exprie_timer();
    }
    else
    {
      printf("create_timer_cb start_exprie_timer\n");
    }
    return true;
  };
}

int main()
{
  auto& service = get_service();
  service.set_option(YOPT_S_HRES_TIMER, 1);
  service.set_option(YOPT_C_REMOTE_ENDPOINT, 0, "www.ip138.com", 80);
  service.start([&](event_ptr&& ev) {
    switch (ev->kind())
    {
      case YEK_PACKET: {
        auto packet = std::move(ev->packet());
        fwrite(packet.data(), packet.size(), 1, stdout);
        fflush(stdout);
        break;
      }
      case YEK_CONNECT_RESPONSE:
        if (ev->status() == 0)
        {
          start_exprie_timer();

          auto transport = ev->transport();
          if (ev->cindex() == 0)
          {
            obstream obs;
            obs.write_bytes("GET /index.htm HTTP/1.1\r\n");

            obs.write_bytes("Host: www.ip138.com\r\n");

            obs.write_bytes("User-Agent: Mozilla/5.0 (Windows NT 10.0; "
                            "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
                            "Chrome/79.0.3945.117 Safari/537.36\r\n");
            obs.write_bytes("Accept: */*;q=0.8\r\n");
            obs.write_bytes("Connection: Close\r\n\r\n");

            service.write(transport, std::move(obs.buffer()));
          }
        }
        break;
      case YEK_CONNECTION_LOST:
        printf("The connection is lost.\n");
        break;
    }
  });
  // open channel 0 as tcp client
  service.open(0, YCK_TCP_CLIENT);

  std::this_thread::sleep_for(std::chrono::microseconds(1000 * 1000));
  printf("tmp timer call at %lld\n", getTimeStamp());
  service.schedule(std::chrono::milliseconds(1), [](io_service&) {
    printf("tmp timer start at %lld\n", getTimeStamp());
    return false;
  });

  getchar();
}
