#include "xxsocket.h"
#include "endian_portable.h"
using namespace purelib::net;
/*
"115.159.188.219""::FFFF:115:159:188:219"
*/

// ipv4: 15
// ipv6: 39
// port: 5

INT
WSAAPI
inet_pton_xp(
    _In_                                      INT             Family,
    _In_                                      PCSTR           pszAddrString,
    _Out_writes_bytes_(sizeof(IN6_ADDR))      PVOID           pAddrBuf
);

int main(int, char**)
{
#if 0
    char hostname[256] = { 0 };

    gethostname(hostname, sizeof(hostname));

    //// ipv4
    //hostent* hostinfo;
    //if ((hostinfo = gethostbyname(hostname)) == NULL) 
    //{
    //    errno = GetLastError();
    //    fprintf(stderr, "gethostbyname Error:%d\n", errno);
    //    return 1;
    //}
    //LPCSTR ip;
    //while (*(hostinfo->h_addr_list) != NULL) 
    //{
    //    ip = inet_ntoa(*(struct in_addr *) *hostinfo->h_addr_list);
    //    printf("ipv4 addr = %s\n\n", ip);
    //    hostinfo->h_addr_list++;
    //}

    // ipv4 & ipv6
    addrinfo hint, *ailist;
    memset(&hint, 0x0, sizeof(hint));
    hint.ai_flags = AI_PASSIVE;
    hint.ai_family = AF_UNSPEC; // AF_UNSPEC: Do we need IPv6 ?
    hint.ai_socktype = 0; // SOCK_STREAM;
    getaddrinfo(nullptr, "60000", &hint, &ailist);
    /*if (iretval < 0)
    {
        char str_error[100];
        strcpy(str_error, (char *)gai_strerror(errno));
        printf("str_error = %s", str_error);
        return 0;
    }*/
    if (ailist != nullptr) {
        for (auto aip = ailist; aip != NULL; aip = aip->ai_next)
        {
            switch (aip->ai_family) {
            case AF_INET6: {
                auto sinp6 = (struct sockaddr_in6 *)aip->ai_addr;
                int i;
                printf("ipv6 addr = ");
                for (i = 0; i < 16; i++)
                {
                    if (((i - 1) % 2) && (i > 0))
                    {
                        printf(":");
                    }
                    printf("%02x", sinp6->sin6_addr.u.Byte[i]);
                }
                printf(" \n");
                printf(" \n"); }
                           break;
            case AF_INET:
                break;
            }
        }
    }

    freeaddrinfo(ailist);
#endif
    auto flags = xxsocket::getinetpv();

    xxsocket s;
    s.pconnect("fe80::f1:584e:dfb4:4d95", 1033);
    int ec = xxsocket::get_last_errno();

    auto localep = s.local_endpoint();
    auto localaddr = localep.to_string();

    s.send("hello world", sizeof("hello world"));

    s.close();

    return 0;
}
