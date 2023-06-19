#include "yasio/utils.hpp"
#include "yasio/yasio.hpp"

using namespace yasio;

int main()
{
  xxsocket sock;
  const char* addrip = "114.114.114.114";
  u_short port       = 9099;
  YASIO_LOG("connecting %s:%u...", addrip, port);
  auto start = yasio::highp_clock();
  if (sock.xpconnect_n(addrip, port, std::chrono::seconds(3)) != 0)
    YASIO_LOG("connect %s failed, the timeout should be approximately equal to 3(s), real: "
           "%lf(s); but when the host network adapter isn't connected, it may be approximately "
           "equal to 0(s)\n",
           addrip, (yasio::highp_clock() - start) / (double)std::micro::den);
  else
    YASIO_LOG("Aha connect %s succeed, cost: %lf(s)\n", addrip,
           (yasio::highp_clock() - start) / (double)std::micro::den);
  return 0;
}
