#include "../../src/xxsocket.h"
using namespace purelib::net;

int main(int, char**)
{
    xxsocket tcpcli;

    if (tcpcli.pconnect_n("www.baidu.com", 443, 3/* connect timeout: 3 seconds*/) == 0)
    {
        printf("connect https serve success.");
    }
    else {
        printf("connect failed:%s", xxsocket::get_error_msg(xxsocket::get_last_errno()));
    }

    getchar();

    // xxsocket will close socket automatically.

    return 0;
}

