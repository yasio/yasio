#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "yasio_http/HttpClient.h"
#include "yasio/xxsocket.hpp"
#include "yasio/yasio.hpp"
#include "yasio/obstream.hpp"

#include <fstream>

using namespace yasio;
using namespace yasio_ext::network;

#define CHROME_UA "User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/108.0.5359.125 Safari/537.36"

void yasioTest()
{
  auto httpClient = HttpClient::getInstance();
  httpClient->setDispatchOnWorkThread(true);

  auto request = new HttpRequest();

  request->setHeaders(std::vector<std::string>{CHROME_UA});
  request->setUrl("https://www.baidu.com");
  request->setRequestType(HttpRequest::Type::GET);

  auto response = httpClient->sendSync(request);
  request->release();

  if (response)
  {
    if (response->getResponseCode() == 200)
    {
      printf("request succeed.\n\n");
      auto responseData = response->getResponseData();
      fwrite(responseData->data(), responseData->size(), 1, stdout);
    }
    else
      printf("request failed with %d", response->getResponseCode());
    response->release();
  }
  HttpClient::destroyInstance();
}

int main(int, char**)
{
  yasioTest();

  return 0;
}
