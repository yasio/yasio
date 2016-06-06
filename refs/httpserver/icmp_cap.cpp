#include <stdio.h> 
#include <winsock2.h> 
#include <ws2tcpip.h> 
#include "thelib/utils/xxsocket.h"

using namespace thelib::net;

#define ADJUST_CHKSUM_TOS(tos_new, tos_org, chksum) \
    do { \
    s32 accumulate = 0; \
    if (chksum == 0) break; \
    if (tos_new != tos_org){ \
    accumulate += (tos_org); \
    accumulate -= (tos_new); \
    } \
        else break; \
        accumulate += ntohs(chksum); \
        if (accumulate < 0) { \
        accumulate = -accumulate; \
        accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
        accumulate += accumulate >> 16; \
        chksum = htons((__u16) ~accumulate); \
        } else { \
        accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
        accumulate += accumulate >> 16; \
        chksum = htons((__u16) accumulate); \
        } \
    }while(0)

#define ADJUST_CHKSUM_NAT(ip_new, ip_org, chksum) \
    do { \
    s32 accumulate = 0; \
    if (chksum == 0) break; \
    if (((ip_new) != 0) && ((ip_org) != 0)){ \
    accumulate = ((ip_org) & 0xffff); \
    accumulate += (( (ip_org) >> 16 ) & 0xffff); \
    accumulate -= ((ip_new) & 0xffff); \
    accumulate -= (( (ip_new) >> 16 ) & 0xffff); \
    } \
        else break; \
        accumulate += ntohs(chksum); \
        if (accumulate < 0) { \
        accumulate = -accumulate; \
        accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
        accumulate += accumulate >> 16; \
        chksum = htons((__u16) ~accumulate); \
        } else { \
        accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
        accumulate += accumulate >> 16; \
        chksum = htons((__u16) accumulate); \
        } \
    }while(0)

#define ADJUST_CHKSUM_NAPT(port_new, port_org, chksum) \
    do { \
    s32 accumulate = 0; \
    if (chksum == 0) break; \
    if (((port_new) != 0) && ((port_org) != 0)){ \
    accumulate += (port_org); \
    accumulate -= (port_new); \
    } \
    accumulate += ntohs(chksum); \
    if (accumulate < 0) { \
    accumulate = -accumulate; \
    accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
    accumulate += accumulate >> 16; \
    chksum = htons((__u16) ~accumulate); \
    } else { \
    accumulate = (accumulate >> 16) + (accumulate & 0xffff); \
    accumulate += accumulate >> 16; \
    chksum = htons((__u16) accumulate); \
    } \
    }while(0)

#define PROTOCOL_STRING_ICMP_TXT "ICMP"
#define PROTOCOL_STRING_TCP_TXT "TCP"
#define PROTOCOL_STRING_UDP_TXT "UDP"
#define PROTOCOL_STRING_SPX_TXT "SPX"
#define PROTOCOL_STRING_NCP_TXT "NCP"
#define PROTOCOL_STRING_UNKNOW_TXT "UNKNOWN"

// Standard TCP flags 
#define URG 0x20 
#define ACK 0x10 
#define PSH 0x08 
#define RST 0x04 
#define SYN 0x02 
#define FIN 0x01

typedef struct _iphdr //定义IP首部 
{ 
    unsigned char h_verlen; //4位首部长度+4位IP版本号 
    unsigned char tos; //8位服务类型TOS 
    unsigned short total_len; //16位总长度（字节） 
    unsigned short ident; //16位标识 
    unsigned short frag_and_flags; //3位标志位 
    unsigned char ttl; //8位生存时间 TTL 
    unsigned char proto; //8位协议 (TCP, UDP 或其他) 
    unsigned short checksum; //16位IP首部校验和 
    unsigned int sourceIP; //32位源IP地址 
    unsigned int destIP; //32位目的IP地址 
}IP_HEADER; 

typedef struct _tcphdr //定义TCP首部 
{ 
    USHORT th_sport; //16位源端口 
    USHORT th_dport; //16位目的端口 
    unsigned int th_seq; //32位序列号 
    unsigned int th_ack; //32位确认号 
    unsigned char th_lenres;//4位首部长度/6位保留字 
    unsigned char th_flag; //6位标志位
    USHORT th_win; //16位窗口大小
    USHORT th_sum; //16位校验和
    USHORT th_urp; //16位紧急数据偏移量
}TCP_HEADER; 

char* GetProtocolText(int Protocol)
{
    switch (Protocol)
    {
    case IPPROTO_ICMP : //1 control message protocol 
        return PROTOCOL_STRING_ICMP_TXT;
    default:
        return "";//PROTOCOL_STRING_UNKNOW_TXT;
    }
}

bool parse_ip_hdr(const char* p, int len, ip::ip_header& iph)
{
    if(len < sizeof(iph)) {
        return false;
    }

    /* uint8_t lv = *p++;

    iph.header_length = lv;
    iph.version = lv >> 4;*/

    memcpy(&iph, p, sizeof(iph));
    iph.total_length = ntohs(iph.total_length);
    iph.checksum = ntohs(iph.checksum);
    return true;
}

unsigned short checksum(unsigned short*buf, int nword)
{
    unsigned long sum;

    for(sum = 0; nword > 0; nword--)

        sum += *buf++;

    sum = (sum>>16) + (sum&0xffff);

    sum += (sum>>16);

    return~sum;

}

/// socket(AF_INET, SOCK_RAW, IPPROTO_IP);

int main()
{
    xxsocket::init_ws32_lib();

    xxsocket sock(AF_INET, SOCK_RAW, IPPROTO_IP);
    sock.bind("0.0.0.0", 0);
    sock.ioctl(SIO_RCVALL, true);
    sock.ioctl(SIO_RCVALL_IF, true);

    static char buf[65535];

    char* wptr = buf;
    int bytes_transferred = 0;
    int n = 0;

    ip::endpoint_v4 endpoint;
    ip::ip_header iph;
    while(true)
    {
        // read ip header
        do {
            n = sock.recvfrom_i(wptr, sizeof (buf) - bytes_transferred, endpoint);
            wptr += n;
            bytes_transferred += n;
        } while(bytes_transferred < sizeof(ip::ip_header));

        // parse ip header
        if(parse_ip_hdr(buf, bytes_transferred, iph)) {

            // checksum
            unsigned short result = checksum((unsigned short*)buf, sizeof(ip::ip_header) / 2);

            // output protocol type
            if(iph.protocol == IPPROTO_ICMP) {
                printf("recv icmp, ip packet size:%d, source:%d.%d.%d.%d, dest:%d.%d.%d.%d\n",
                    iph.total_length, 
                    iph.src_ip.detail.B1, iph.src_ip.detail.B2, iph.src_ip.detail.B3, iph.src_ip.detail.B4,
                    iph.dst_ip.detail.B1, iph.dst_ip.detail.B2, iph.dst_ip.detail.B3, iph.dst_ip.detail.B4);
                //printf("receive icmp, ip packet size:%d, peer: %s\n", iph.total_length, endpoint.to_string().c_str());
            }
            else if(iph.protocol == IPPROTO_TCP) {
                printf("recv tcp, ip packet size:%d, source:%d.%d.%d.%d, dest:%d.%d.%d.%d\n",
                    iph.total_length, 
                    iph.src_ip.detail.B1, iph.src_ip.detail.B2, iph.src_ip.detail.B3, iph.src_ip.detail.B4,
                    iph.dst_ip.detail.B1, iph.dst_ip.detail.B2, iph.dst_ip.detail.B3, iph.dst_ip.detail.B4);
                //printf("receive tcp, ip packet size:%d, peer: %s\n", iph.total_length, endpoint.to_string().c_str());
            }
            else if(iph.protocol == IPPROTO_UDP) {
                printf("recv udp, ip packet size:%d, source:%d.%d.%d.%d, dest:%d.%d.%d.%d\n",
                    iph.total_length, 
                    iph.src_ip.detail.B1, iph.src_ip.detail.B2, iph.src_ip.detail.B3, iph.src_ip.detail.B4,
                    iph.dst_ip.detail.B1, iph.dst_ip.detail.B2, iph.dst_ip.detail.B3, iph.dst_ip.detail.B4);
                //printf("receive udp, ip packet size:%d, peer: %s\n", iph.total_length, endpoint.to_string().c_str());
            }

            // read ip body
            while(bytes_transferred < iph.total_length)
            {
                n = sock.recvfrom_i(wptr, sizeof(buf) - bytes_transferred, endpoint);
                wptr += n;
                bytes_transferred += n;
            }
        }
        else {
            printf("parse ip header failed!");
        }
    }

    return 0;
}
