
#include "yasio/detail/utils.hpp"
#include "yasio/xxsocket.hpp"
#include "yasio/pod_vector.hpp"
#include "yasio/xx_podvector.hpp"

#include <stdio.h>

using namespace yasio;

int main()
{
  int64_t count, start;

  printf("Testing xx_pod_vector\n");
  count = 0;
  start = yasio::highp_clock();
  for(int i = 0; i < 100000; ++i) {
    xx::PodVector<int> pv;
    for (int j = 0; j < 1000; ++j) {
      pv.Emplace(j);
    }
    for (int j = 0; j < 1000; ++j) {
      count += pv[j];
    }
  }
  printf("xxpodv --> count: %lld, cost: %lf(s)\n", count, 
           (yasio::highp_clock() - start) / (double)std::micro::den);
  
  printf("\nTesting pod_vector\n");
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
  printf("axpodv --> count: %lld, cost: %lf(s)\n", count, 
           (yasio::highp_clock() - start) / (double)std::micro::den);
   

  printf("\nTesting std_vector\n");
  count = 0;
  start = yasio::highp_clock();
  for(int i = 0; i < 100000; ++i) {
    std::vector<int> pv;
    for (int j = 0; j < 1000; ++j) {
      pv.emplace_back(j);
    }
    for (int j = 0; j < 1000; ++j) {
      count += pv[j];
    }
  }
  printf("stdv --> count: %lld, cost: %lf(s)\n", count, 
           (yasio::highp_clock() - start) / (double)std::micro::den);
  return 0;
}
