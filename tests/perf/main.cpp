
#include "yasio/detail/utils.hpp"
#include "yasio/xxsocket.hpp"

using namespace yasio;

int main()
{
  YASIO_LOG("Testing pod_vector");
  auto start = yasio::highp_clock();
  
  
  YASIO_LOG("cost: %lf(s)\n",
           (yasio::highp_clock() - start) / (double)std::micro::den);
  return 0;
}
