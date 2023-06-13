#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio_http/HttpClient.h"
#include "yasio/yasio.hpp"

#include <fstream>

using namespace yasio;
using namespace yasio_ext::network;

#define CHROME_UA "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/114.0.0.0 Safari/537.36"

void yasioTest()
{
  auto httpClient = HttpClient::getInstance();
  httpClient->setDispatchOnWorkThread(true);

  auto request = new HttpRequest();

  request->setHeaders(std::vector<std::string>{CHROME_UA});
  request->setUrl("https://github.com/yasio");
  request->setRequestType(HttpRequest::Type::Get);

  auto response = httpClient->sendSync(request);
  request->release();

  if (response)
  {
    if (response->getResponseCode() == 200)
    {
      auto responseData = response->getResponseData();
      // fwrite(responseData->data(), responseData->size(), 1, stdout);
      printf("===>request succeed\n");
    }
    else
      printf("===>request failed with %d\n", response->getResponseCode());
    printf("%s", "===>response headers:\n");
    for (auto& header : response->getResponseHeaders())
      printf("\t%s: %s\n", header.first.c_str(), header.second.c_str());
    response->release();
  }
  HttpClient::destroyInstance();
}

int main(int, char**)
{
#if defined(_WIN32)
  SetConsoleOutputCP(CP_UTF8);
#endif

  yasioTest();

  return 0;
}
