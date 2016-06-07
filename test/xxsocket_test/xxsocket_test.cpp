#include "../../src/xxsocket.h"
using namespace purelib::net;

void test_https_connect()
{
    xxsocket tcpcli;

    if (tcpcli.pconnect_n("www.baidu.com", 443, 3/* connect timeout: 3 seconds*/) == 0)
    {
        printf("connect https server success.");
    }
    else {
        printf("connect failed:%s", xxsocket::get_error_msg(xxsocket::get_last_errno()));
    }
}

int main(int, char**)
{
    test_https_connect();

    getchar();

    // xxsocket will close socket automatically.

    return 0;
}

