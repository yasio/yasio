#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "../../src/yasio.h"
#include "../../src/ibinarystream.h"
#include "../../src/obinarystream.h"

#if defined(_WIN32)
#  include <Shlwapi.h>
#  pragma comment(lib, "shlwapi.lib")
#  define strcasestr StrStrIA
#endif

using namespace purelib::inet;

template <size_t _Size> void append_string(std::vector<char> &packet, const char (&message)[_Size])
{
  packet.insert(packet.end(), message, message + _Size - 1);
}

int main(int, char **)
{
  purelib::inet::io_hostent endpoints[] = {{"203.162.71.67", 80} // http client
                                           ,
                                           {"www.ip138.com", 80}
                                           //  { "www.ip138.com", 80 },  // http client
                                           ,
                                           {"192.168.103.183", 59281},
                                           {"192.168.103.183", 59281}}; // udp client

  obinarystream obs;
  obs.push24();
  obs.write_i24(16777215);
  obs.write_i24(16777213);
  obs.write_i24(259);
  obs.write_i24(16777217); // uint24 value overflow test
  obs.pop24();

  ibinarystream ibs(obs.data(), obs.length());
  auto n  = ibs.read_i24();
  auto v1 = ibs.read_i24();
  auto v2 = ibs.read_i24();
  auto v3 = ibs.read_i24();
  auto v4 = ibs.read_i24();

  myasio->set_option(YASIO_OPT_TCP_KEEPALIVE, 60, 30, 3);

  resolv_fn_t resolv = [](std::vector<ip::endpoint> &endpoints, const char *hostname,
                          unsigned short port) {
    return myasio->resolve(endpoints, hostname, port);
  };
  myasio->set_option(YASIO_OPT_RESOLV_FUNCTION, &resolv);

  deadline_timer t0(*myasio);

  std::vector<std::shared_ptr<io_transport>> transports;
  myasio->set_option(YASIO_OPT_LFIB_PARAMS, 16384, -1, 0, 0);
  myasio->set_option(YASIO_OPT_LOG_FILE, "yasio.log");

  transport_ptr transport2;
  transport_ptr transport3;
  myasio->start_service(endpoints, _ARRAYSIZE(endpoints), [&](event_ptr event) {
    switch (event->type())
    {
      case YASIO_EVENT_RECV_PACKET:
      {
        auto packet = event->take_packet();
        packet.push_back('\0');
        printf("index:%d, receive data:%s", event->transport()->channel_index(), packet.data());
        break;
      }
      case YASIO_EVENT_CONNECT_RESPONSE:
        if (event->status() == 0)
        {
          if (event->channel_index() == 3)
            transport3 = event->transport();

          else if (event->channel_index() == 2)
            transport2 = event->transport();
        }
        // if (event->status() == 0)
        //{
        //  auto transport = event->transport();
        //  std::vector<char> packet;
        //  append_string(packet, "GET /index.htm HTTP/1.1\r\n");
        //
        //  if (transport->channel_index() == 0)
        //    append_string(packet, "Host: 203.162.71.67\r\n");
        //  else
        //    append_string(packet, "Host: www.ip138.com\r\n");
        //
        //  append_string(packet, "User-Agent: Mozilla/5.0 (Windows NT 10.0; "
        //                        "WOW64) AppleWebKit/537.36 (KHTML, like Gecko) "
        //                        "Chrome/51.0.2704.106 Safari/537.36\r\n");
        //  append_string(packet, "Accept: */*;q=0.8\r\n");
        //  append_string(packet, "Connection: Close\r\n\r\n");
        //
        //  transports.push_back(transport);
        //
        //  myasio->write(transport, std::move(packet));
        //}
        break;
      case YASIO_EVENT_CONNECTION_LOST:
        printf("The connection is lost(user end)!\n");
        break;
    }
  });

  std::this_thread::sleep_for(std::chrono::seconds(1));
  // myasio->open(0);
  // myasio->open(1);
  myasio->open(2, CHANNEL_UDP_SERVER);

  deadline_timer t1(*myasio);
  t0.expires_from_now(std::chrono::seconds(3));
  t0.async_wait([&](bool) { // called at network thread
    printf("open channel 3 to connect udp server.\n");
    myasio->open(3, CHANNEL_UDP_CLIENT);

    t1.expires_from_now(std::chrono::seconds(3), true);
    t1.async_wait([&](bool) { // called at network thread
      std::vector<char> packet;
      append_string(packet, "hello server!\n");
      myasio->write(transport3, std::move(packet));

      append_string(packet, "hello client!\n");
      myasio->write(transport2, std::move(packet));
    });
  });

  time_t duration = 0;
  while (true)
  {
    myasio->dispatch_events();
    if (duration >= 600000)
    {
      for (auto transport : transports)
        myasio->close(transport);
      break;
    }
    duration += 50;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  std::this_thread::sleep_for(std::chrono::seconds(60));

  myasio->stop_service();

  return 0;
}
