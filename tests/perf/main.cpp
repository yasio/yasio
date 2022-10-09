
#include "yasio/detail/utils.hpp"
#include "yasio/xxsocket.hpp"

#include "pod_vector.h"
#include "xx_podvector.h"

#include <stdio.h>

using namespace yasio;

int main()
{
  try
  {
    std::_Xlength_error("ff");
  }
  catch (std::exception& ex)
  {
    printf("%s", ex.what());
  }
  int64_t count, start;
  int realloc_hints;

  constexpr int NUM_TIMES = 100000;

  for (int k = 0; k < 3; ++k)
  {
    printf("######## Test round %d ######### \n", k + 1);

    /* xx pod vector */
    printf("Testing xx_pod_vector...");
    count = 0;
    realloc_hints = 0;
    start = yasio::highp_clock();
    for (int i = 0; i < NUM_TIMES; ++i)
    {
      xx::PodVector<int> pv;
      pv.Reserve(1000);
      for (int j = 0; j < 1000; ++j)
      {
        pv.Emplace(j);
      }
      for (int j = 0; j < 1000; ++j)
      {
        count += pv[j];
      }
      // realloc_hints = pv.realloc_hits;
    }
    printf("--> count: %lld, cost: %lf(s), realloc_hits: %d\n", count,
           (yasio::highp_clock() - start) / (double)std::micro::den, realloc_hints);

    /* ax pod vector */
    printf("Testing ax_pod_vector...");
    count = 0;
    realloc_hints = 0;
    start = yasio::highp_clock();
    for (int i = 0; i < NUM_TIMES; ++i)
    {
      ax::pod_vector<int> pv;
      pv.reserve(1000);
      for (int j = 0; j < 1000; ++j)
      {
        pv.emplace_back(j);
      }
      for (int j = 0; j < 1000; ++j)
      {
        count += pv[j];
      }
    }
    printf("--> count: %lld, cost: %lf(s), realloc_hits: %d\n", count,
           (yasio::highp_clock() - start) / (double)std::micro::den, realloc_hints);

    
    /* std vector */
    printf("Testing std_vector...");
    count = 0;
    start = yasio::highp_clock();
    for (int i = 0; i < NUM_TIMES; ++i)
    {
      std::vector<int> pv;
      pv.reserve(1000);
      for (int j = 0; j < 1000; ++j)
      {
        pv.emplace_back(j);
      }
      for (int j = 0; j < 1000; ++j)
      {
        count += pv[j];
      }

      pv.clear();
      pv.shrink_to_fit();
    }

    printf("--> count: %lld, cost: %lf(s)\n", count,
           (yasio::highp_clock() - start) / (double)std::micro::den);

    printf("\n\n");
  }
  return 0;
}
