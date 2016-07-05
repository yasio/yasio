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
	auto epv6 = xxsocket::resolve("127.0.0.1", 81);
	tcpcli.open(epv6.af());
	auto flag = tcpcli.getipsv();
    if (tcpcli.connect_n(epv6, 3/* connect timeout: 3 seconds*/) == 0)
    {
        printf("connect https server success.");
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

