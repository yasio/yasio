
#include "yasio/detail/utils.hpp"
#include "yasio/xxsocket.hpp"
#include "yasio/pod_vector.hpp"
#include "yasio/xx_podvector.hpp"

using namespace yasio;

int main()
{
  int64_t count, start;

  YASIO_LOG("Testing xx_pod_vector");
  count = 0;
  start = yasio::highp_clock();
  for(int i = 0; i < 100000; ++i) {
    PodVector<int> pv;
    for (int j = 0; j < 1000; ++j) {
      pv.Emplace(j);
    }
    for (int j = 0; j < 1000; ++j) {
      count += pv[j];
    }
  }
  YASIO_LOG("xxpodv --> count: %lld, cost: %lf(s)\n", count
           (yasio::highp_clock() - start) / (double)std::micro::den);
  
  YASIO_LOG("Testing pod_vector");
  count = 0;
  start = yasio::highp_clock();
  for(int i = 0; i < 100000; ++i) {
    ax::pod_vector<int> pv;
    for (int j = 0; j < 1000; ++j) {
      pv.emplace(j);
    }
    for (int j = 0; j < 1000; ++j) {
      count += pv[j];
    }
  }
  YASIO_LOG("axpodv --> count: %lld, cost: %lf(s)\n", count
           (yasio::highp_clock() - start) / (double)std::micro::den);
   
  return 0;
}
