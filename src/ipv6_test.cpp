#include "xxsocket.h"
#include "endian_portable.h"
using namespace purelib::net;

/*
"115.159.188.219""::FFFF:115:159:188:219"
*/

INT
WSAAPI
inet_pton_xp(
    INT             Family,
    PCSTR           pszAddrString,
    PVOID           pAddrBuf
)
{
    if (Family == AF_INET) {
        in_addr* pAddr = (in_addr*)pAddrBuf;
        return sscanf(pszAddrString, "%d.%d.%d.%d", &pAddr->s_net, &pAddr->s_host, &pAddr->s_lh, &pAddr->s_impno);
    }
    else {
        in6_addr* pAddr = (in6_addr*)pAddrBuf;
        return sscanf(pszAddrString, "%d:%d:%d:%d:%d:%d:%d:%d", 
            &pAddr->s6_bytes[0], &pAddr->s6_bytes[1], &pAddr->s6_bytes[2], &pAddr->s6_bytes[3],
            &pAddr->s6_bytes[4], &pAddr->s6_bytes[5], &pAddr->s6_bytes[6], &pAddr->s6_bytes[7]);
        return 0;
    }
}

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
#if 0
    in_addr addr4, addr41;
    memset(&addr4, 0x0, sizeof(addr4));
    memset(&addr41, 0x0, sizeof(addr41));
    addr4.s_addr = inet_addr("0.0.0.0");
    auto n = inet_pton_xp(AF_INET, "0.0.0.0", &addr41);

    in6_addr addr6, addr61;
    memset(&addr6, 0x0, sizeof(addr6));
    memset(&addr61, 0x0, sizeof(addr61));
    inet_pton(AF_INET6, "fe80::a0b2:dded:8b90:50c5", &addr6);
    inet_pton_xp(AF_INET6, "fe80::a0b2:dded:8b90:50c5", &addr61);

    
    auto flags = xxsocket::getipsv();

    auto v6addr = xxsocket::resolve_v6("0.0.0.0", 2033);

    std::string addrname = v6addr.to_string();
#endif
    xxsocket s;
    s.connect("fe80::f1:584e:dfb4:4d95", 1033);
    int ec = xxsocket::get_last_errno();

    ip::endpoint ep("192.168.1.10", 8002);
    auto checks = ep.to_string();

    auto localep = s.local_endpoint();
    auto localaddr = localep.to_string();

    s.send("hello world", sizeof("hello world"));

    s.close();

    return 0;
}
