#include "xxsocket.h"
#include "pcode_autog_client_messages.h"

using namespace purelib::inet;

static const int sinlen = sizeof(sockaddr_in);
static const int sinlen2 = 0x10;

obinarystream pcode_autog_begin_encode(uint16_t command_id)
{
    messages::MsgHeader hdr;
    hdr.command_id = command_id;
    hdr.length = 0;
    hdr.reserved = 0x5a5a;
    hdr.reserved2 = microtime();
    hdr.version = 1;
    return hdr.encode();
}

void test_https_connect()
{
    xxsocket tcpcli;
   // auto epv6 = xxsocket::resolve("www.baidu.com", 443);
    int n = tcpcli.xpconnect_n("www.baidu.com", 443, 3);
    auto flag = tcpcli.getipsv();
    printf("ip protocol support flag is:%d\n", flag);
    if (n == 0)
    {
        printf("connect https server success, [%s] --> [%s]", tcpcli.local_endpoint().to_string().c_str(), tcpcli.peer_endpoint().to_string().c_str());
    }
    else {
        printf("connect failed, code:%d,info:%s", xxsocket::get_last_errno(), xxsocket::get_error_msg(xxsocket::get_last_errno()));
    }
}

int main(int, char**)
{
    test_https_connect();

    getchar();

    // xxsocket will close socket automatically.

    return 0;
}

