#ifndef _XSOCKET_H_
#define _XSOCKET_H_
#ifdef _WIN32
#include <WinSock2.h>
#include <Mswsock.h>
#include <Windows.h>
#include <Mstcpip.h>
#define skerrno WSAGetLastError()
typedef SOCKET socket_native_type; 
typedef int socklen_t;
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/poll.h>
// #include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <arpa/inet.h>
#define SD_RECEIVE SHUT_RD
#define SD_SEND SHUT_WR
#define SD_BOTH SHUT_RDWR
#define skerrno errno
#define closesocket close
#define ioctlsocket ioctl
typedef int socket_native_type;
#undef socket
#endif
#include <errno.h>
#include <string.h>
#include <sstream>
#include "simpleppdef.h"

#ifdef _WIN32

// A blocking operation was interrupted by a call to WSACancelBlockingCall.
#ifdef EINTR
#undef EINTR
#endif
#define EINTR            WSAEINTR                         

// The file handle supplied is not valid.
#ifdef EBADF
#undef EBADF
#endif
#define EBADF            WSAEBADF                         

// An attempt was made to access a socket in a way forbidden by its access permissions.
#ifdef EACCES
#undef EACCES
#endif
#define EACCES           WSAEACCES                        

// The system detected an invalid pointer address in attempting to use a pointer argument in a call.
#ifdef EFAULT
#undef EFAULT
#endif
#define EFAULT           WSAEFAULT

// An invalid argument was supplied.
#ifdef EINVAL
#undef EINVAL
#endif
#define EINVAL           WSAEINVAL

// Too many open sockets.
#ifdef EMFILE
#undef EMFILE
#endif
#define EMFILE           WSAEMFILE

// A non-blocking socket operation could not be completed immediately.
#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif
#define EWOULDBLOCK      WSAEWOULDBLOCK

// A blocking operation is currently executing.
#ifdef EINPROGRESS
#undef EINPROGRESS
#endif
#define EINPROGRESS      WSAEINPROGRESS

// An operation was attempted on a non-blocking socket that already had an operation in progress.
#ifdef EALREADY
#undef EALREADY
#endif
#define EALREADY         WSAEALREADY

// An operation was attempted on something that is not a socket.
#ifdef ENOTSOCK
#undef ENOTSOCK
#endif
#define ENOTSOCK         WSAENOTSOCK

// A required address was omitted from an operation on a socket.
#ifdef EDESTADDRREQ
#undef EDESTADDRREQ
#endif
#define EDESTADDRREQ     WSAEDESTADDRREQ

// A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram into was smaller than the datagram itself.
#ifdef EMSGSIZE
#undef EMSGSIZE
#endif
#define EMSGSIZE         WSAEMSGSIZE

// A protocol was specified in the socket function call that does not support the semantics of the socket type requested.
#ifdef EPROTOTYPE
#undef EPROTOTYPE
#endif
#define EPROTOTYPE       WSAEPROTOTYPE

// An unknown, invalid, or unsupported option or level was specified in a getsockopt or setsockopt call.
#ifdef ENOPROTOOPT
#undef ENOPROTOOPT
#endif
#define ENOPROTOOPT      WSAENOPROTOOPT

// The requested protocol has not been configured into the system, or no implementation for it exists.
#ifdef EPROTONOSUPPORT
#undef EPROTONOSUPPORT
#endif
#define EPROTONOSUPPORT  WSAEPROTONOSUPPORT

// The support for the specified socket type does not exist in this address family.
#ifdef ESOCKTNOSUPPORT
#undef ESOCKTNOSUPPORT
#endif
#define ESOCKTNOSUPPORT  WSAESOCKTNOSUPPORT

// The attempted operation is not supported for the type of object referenced.
#ifdef EOPNOTSUPP
#undef EOPNOTSUPP
#endif
#define EOPNOTSUPP       WSAEOPNOTSUPP

// The protocol family has not been configured into the system or no implementation for it exists.
#ifdef EPFNOSUPPORT
#undef EPFNOSUPPORT
#endif
#define EPFNOSUPPORT     WSAEPFNOSUPPORT

// An address incompatible with the requested protocol was used.
#ifdef EAFNOSUPPORT
#undef EAFNOSUPPORT
#endif
#define EAFNOSUPPORT     WSAEAFNOSUPPORT

// Only one usage of each socket address (protocol/network address/port) is normally permitted.
#ifdef EADDRINUSE
#undef EADDRINUSE
#endif
#define EADDRINUSE       WSAEADDRINUSE

// The requested address is not valid in its context.
#ifdef EADDRNOTAVAIL
#undef EADDRNOTAVAIL
#endif
#define EADDRNOTAVAIL    WSAEADDRNOTAVAIL

// A socket operation encountered a dead network.
#ifdef ENETDOWN
#undef ENETDOWN
#endif
#define ENETDOWN         WSAENETDOWN

// A socket operation was attempted to an unreachable network.
#ifdef ENETUNREACH
#undef ENETUNREACH
#endif
#define ENETUNREACH      WSAENETUNREACH

// The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress.
#ifdef ENETRESET
#undef ENETRESET
#endif
#define ENETRESET        WSAENETRESET

// An established connection was aborted by the software in your host machine.
#ifdef ECONNABORTED
#undef ECONNABORTED
#endif
#define ECONNABORTED     WSAECONNABORTED

// An existing connection was forcibly closed by the remote host.
#ifdef ECONNRESET
#undef ECONNRESET
#endif
#define ECONNRESET       WSAECONNRESET

// An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full.
#ifdef ENOBUFS
#undef ENOBUFS
#endif
#define ENOBUFS          WSAENOBUFS

// A connect request was made on an already connected socket.
#ifdef EISCONN
#undef EISCONN
#endif
#define EISCONN          WSAEISCONN

// A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using a sendto call) no address was supplied.
#ifdef ENOTCONN
#undef ENOTCONN
#endif
#define ENOTCONN         WSAENOTCONN

// A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call.
#ifdef ESHUTDOWN
#undef ESHUTDOWN
#endif
#define ESHUTDOWN        WSAESHUTDOWN

// Too many references to some kernel object.
#ifdef ETOOMANYREFS
#undef ETOOMANYREFS
#endif
#define ETOOMANYREFS     WSAETOOMANYREFS

// A connection attempt failed because the connected party did not properly respond after a period of time, or established connection failed because connected host has failed to respond.
#ifdef ETIMEDOUT
#undef ETIMEDOUT
#endif
#define ETIMEDOUT        WSAETIMEDOUT

// No connection could be made because the target machine actively refused it.
#ifdef ECONNREFUSED
#undef ECONNREFUSED
#endif
#define ECONNREFUSED     WSAECONNREFUSED

// Cannot translate name.
#ifdef ELOOP
#undef ELOOP
#endif
#define ELOOP            WSAELOOP

// Name component or name was too long.
#ifdef ENAMETOOLONG
#undef ENAMETOOLONG
#endif
#define ENAMETOOLONG     WSAENAMETOOLONG

// A socket operation failed because the destination host was down.
#ifdef EHOSTDOWN
#undef EHOSTDOWN
#endif
#define EHOSTDOWN        WSAEHOSTDOWN

// A socket operation was attempted to an unreachable host.
#ifdef EHOSTUNREACH
#undef EHOSTUNREACH
#endif
#define EHOSTUNREACH     WSAEHOSTUNREACH

#endif

// shoulde close connection condition when retval of recv <= 0
#define exxr_should_close(n, errcode) \
    ( ((n) == 0) \
    || ((n) < 0 \
        && (errcode) != EAGAIN \
        && (errcode) != EINTR) ) 


// shoulde close connection condition when retval of send <= 0
#define exxs_should_close(n, errcode) \
    ( ((n) == 0) \
    || ((n) < 0 \
       && (errcode) != EAGAIN \
       && (errcode) != EWOULDBLOCK \
       && (errcode) != ENOBUFS \
       && (errcode) != EINTR) ) 

namespace thelib {

namespace net {

#define _make_value(b1,b2,b3,b4) ( ( ((uint32_t)(b4) << 24) & 0xff000000 ) | ( ((uint32_t)(b3) << 16) & 0x00ff0000 ) | ( ((uint32_t)(b2) << 8) & 0x0000ff00 ) | ( (uint32_t)(b1) & 0x000000ff ) )



static const socket_native_type bad_sock = (socket_native_type)-1;
static const u_long blocking = 0;
static const u_long nonblocking = 1;


namespace ip {

#pragma pack(push,1)
// ip packet
struct ip_header
{
    // header size; 5+
    unsigned char  header_length:4; 

    // IP version: 0100/0x04(IPv4), 0110/0x05(IPv6)
    unsigned char  version:4; 

    // type of service: 
    union {
        unsigned char  value;
        struct {
            unsigned char priority:3;
            unsigned char D:1; // delay: 0(normal), 1(as little as possible)
            unsigned char T:1; // throughput: 0(normal), 1(as big as possible)
            unsigned char R:1; // reliability: 0(normal), 1(as big as possible)
            unsigned char C:1; // transmission cost: 0(normal), 1(as little as possible)
            unsigned char reserved:1; // always be zero
        } detail;
    } TOS; 

    // total size, header + data; MAX length is: 65535
    unsigned short total_length;

    // identifier: all split small packet set as the same value.
    unsigned short identifier;

    // flags and frag
    // unsigned short flags:3;
    // unsigned short frag:13;
    unsigned short flags_and_frag;

    // time of living, decreased by route, if zero, this packet will by dropped
    // avoid foward looply.
    unsigned char  TTL;

    // protocol
    // 1: ICMP
    // 2: IGMP
    // 6: TCP
    // 0x11/17: UDP
    // 0x58/88: IGRP
    // 0x59/89: OSPF
    unsigned char  protocol;// TCP / UDP / Other

    // check header of IP-PACKET 's correctness.
    unsigned short checksum;

    
    typedef union 
    {
        unsigned int value;
        struct {
            unsigned int   B1:8, B2:8, B3:8, B4:8;
        } detail;
    } dotted_decimal_t;

    // source ip address
    dotted_decimal_t src_ip;

    // destination ip address
    dotted_decimal_t dst_ip;
} ;

struct psd_header
{
    unsigned long  src_addr;
    unsigned long  dst_addr;
    char           mbz;
    char           protocol;
    unsigned short tcp_length;
} ;

struct tcp_header
{
    unsigned short src_port;
    unsigned short dst_port;
    unsigned int   seqno;
    unsigned int   ackno;
    unsigned char  header_length:4;
    unsigned char  reserved:4;
    unsigned char  flg_fin:1, flg_syn:1, flg_rst:1, flg_psh:1, flg_ack:1, flg_urg:1, flg_reserved:2;
    unsigned short win_length;
    unsigned short checksum;
    unsigned short urp; // emergency
} ;

struct udp_header
{
    unsigned short src_port;
    unsigned short dst_port;
    unsigned short length;
    unsigned short checksum;
} ;

struct icmp_header
{
    unsigned char  type; // 8bit type
    unsigned char  code; // 8bit code
    unsigned short checksum; // 16bit check sum
    unsigned short id; // identifier: usually use process id
    unsigned short seqno; // message sequence NO.
    unsigned int   timestamp; // timestamp
} ;

struct eth_header
{
    unsigned dst_eth[6];
    unsigned src_eth[6];
    unsigned eth_type;
} ;

struct arp_header
{
    unsigned short arp_hw; // format of hardware address
    unsigned short arp_pro; // format of protocol address
    unsigned char  arp_hlen; // length of hardware address
    unsigned char  arp_plen; // length of protocol address
    unsigned short arp_op; // arp operation
    unsigned char  arp_oha[6]; // sender hardware address
    unsigned long  arp_opa; // sender protocol address
    unsigned char  arp_tha; // target hardware address
    unsigned long  arp_tpa; // target protocol address;
} ;

struct arp_packet
{
    eth_header ethhdr;
    arp_header arphdr;
} ;
#pragma pack(pop)

union endpoint_v4
{
public:
    endpoint_v4(void) 
    {
        ::memset(&this->operating, 0x0, sizeof(this->operating));
    }
    endpoint_v4(const char* addr, unsigned short port)
    {
        this->operating.sin_family = AF_INET;
        this->operating.sin_addr.s_addr = addr ? inet_addr(addr) : 0;
        this->operating.sin_port = htons(port);
    }
    std::string to_string_full(void) const
    {
        std::stringstream ss;
        ss << this->to_string() << ":" << this->port();
        return ss.str();
    }
    std::string to_string(void) const
    {
        return std::string(inet_ntoa(this->operating.sin_addr));
    }
    void to_string_full(char* buffer) const // not safe, if use, please confirm buffer enough
    {
        sprintf(buffer, "%s:%u", inet_ntoa(this->operating.sin_addr), this->port());
    }
    void to_string(char* buffer) const // // not safe, if use, please confirm buffer enough
    {
        strcpy(buffer, inet_ntoa(this->operating.sin_addr));
    }
    unsigned short port(void) const
    {
        return ntohs(operating.sin_port);
    }
    sockaddr internal;
    sockaddr_in operating;
};

};

/*
** a socket helper
*/
class xxsocket
{
public:

    // Construct a empty socket object
    xxsocket(void); 

    // Construct with a exist socket handle
    xxsocket(socket_native_type handle);

    // Construct with a exist socket, it will replace the source
    xxsocket(const xxsocket&);     

    xxsocket& operator=(socket_native_type handle);

    // Construct with a exist socket, it will replace the source
    xxsocket& operator=(const xxsocket&);

    // See also as function: open
    xxsocket(int af, int type, int protocol);

    ~xxsocket(void); 

    // replace other
    xxsocket& replace(xxsocket& who);

    /* @brief: Open new socket
    ** @params: 
    **        af      : Usually is [AF_INET] 
    **        type    : [SOCK_STREAM-->TCP] and [SOCK_DGRAM-->UDP]
    **        protocol: Usually is [0]
    ** @returns: false: check reason by errno
    */
    bool open(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);
#ifdef _WIN32
    bool open_ex(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);

    static bool accept_ex(
    __in SOCKET sockfd_listened,
    __in SOCKET sockfd_prepared,
    __in PVOID lpOutputBuffer,
    __in DWORD dwReceiveDataLength,
    __in DWORD dwLocalAddressLength,
    __in DWORD dwRemoteAddressLength,
    __out LPDWORD lpdwBytesReceived,
    __inout LPOVERLAPPED lpOverlapped);

    static void translate_sockaddrs(
        __in PVOID lpOutputBuffer,
        __in DWORD dwReceiveDataLength,
        __in DWORD dwLocalAddressLength,
        __in DWORD dwRemoteAddressLength,
        __deref_out_bcount(*LocalSockaddrLength) sockaddr **LocalSockaddr,
        __out LPINT LocalSockaddrLength,
        __deref_out_bcount(*RemoteSockaddrLength) sockaddr **RemoteSockaddr,
        __out LPINT RemoteSockaddrLength);
#endif

    /** Is this socket opened **/
    bool is_open(void) const;

    /** Get the socket fd value **/
    socket_native_type get_handle(void) const;

    socket_native_type native_handle(void) const;

    // release handle only;
    socket_native_type release(void);

    /* @brief: Set this socket io mode to nonblocking
    ** @params: 
    **
    ** @returns: [0] succeed, otherwise, a value of SOCKET_ERROR is returned.
    */
    int set_nonblocking(bool nonblocking) const;
    static int set_nonblocking(socket_native_type s, bool nonblocking);


    /* @brief: Set this socket io mode to blocking
    ** @params: 
    **
    ** @returns: [0] succeed, otherwise, a value of SOCKET_ERROR is returned.
    */
    //int set_blocking(void) const;


    /* @brief: Set this socket io mode
    ** @params: 
    **        mode: [nonblocking] or [blocking]
    **
    ** @returns: [0] succeed, otherwise, a value of SOCKET_ERROR is returned.
    */
    //int set_mode(u_long mode = nonblocking) const;


    /* @brief: Associates a local address with this socket
    ** @params: 
    **        addr: four point address, if set nullptr, the socket will listen at any.
    **        port: @$$#s
    ** @returns: 
    **         If no error occurs, bind returns [0]. Otherwise, it returns SOCKET_ERROR
    */
    int bind(const char* addr, unsigned short port) const;


    /* @brief: Places this socket in a state in which it is listening for an incoming connection
    ** @params: 
    **        Ommit
    **
    ** @returns: 
    **         If no error occurs, bind returns [0]. Otherwise, it returns SOCKET_ERROR
    */
    int listen(int backlog = SOMAXCONN) const;


    /* @brief: Permits an incoming connection attempt on this socket
    ** @params: 
    **        addrlen: Usually is [sizeof (sockaddr)]
    **
    ** @returns: 
    **        If no error occurs, accept returns a new socket on which 
    **        the actual connection is made. 
    **        Otherwise, a value of [nullptr] is returned
    */
    xxsocket accept(socklen_t addrlen = sizeof(sockaddr));


    /* @brief: Permits an incoming connection attempt on this socket
    ** @params: 
    **        timeout : milliseconds of waiting for new connection
    **
    ** @returns: 
    **        If no error occurs, accept returns a new socket on which 
    **        the actual connection is made. 
    **        Otherwise, a value of [nullptr] is returned
    */
    xxsocket accept_n(timeval& timeout);
    // static xxsocket accept_n(socket_native_type s, timeval& timeout);


    /* @brief: Establishes a connection to a specified this socket
    ** @params: 
    **        addr: Usually is a IPV4 address
    **        port: Server Listenning Port
    ** 
    ** @returns: 
    **         If no error occurs, returns [0]. 
    **         Otherwise, it returns SOCKET_ERROR
    */
    int connect(const char* addr, u_short port);
    static int connect(socket_native_type s, const char* addr, u_short port);


    /* @brief: Establishes a connection to a specified this socket with nonblocking
    ** @params: 
    **        timeout: connection timeout, millseconds
    **           
    ** @returns: [0].succeed, [-1].failed
    */
    int connect_n(const char* addr, u_short port, timeval& timeout);
    static int connect_n(socket_native_type s,const char* addr, u_short port, timeval& timeout);

    
    /*
    ** @brief: socket buffer size operators
    ** 
    **
    */
    int  get_sndbuf_size(void) const;
    int  get_rcvbuf_size(void) const;
    void set_sndbuf_size(int size);
    void set_rcvbuf_size(int size);


    /* @brief: Sends data on this connected socket
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes sent, 
    **         which can be less than the number requested to be sent in the len parameter. 
    **         Otherwise, a value of SOCKET_ERROR is returned.
    */
    int send(const void* buf, int len, int flags = 0) const;


   /* @brief: nonblock send
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes sent, 
    **         Oterwise, If retval <=0, mean error occured, and should close socket. 
    */
    int send_n(const void* buf, int len, timeval* timeout, int flags = 0);
    static int send_n(socket_native_type s, const void* buf, int len, timeval* timeout, int flags = 0);


    /* @brief: Receives data from this connected socket or a bound connectionless socket. 
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, recv returns the number of bytes received and 
    **         the buffer pointed to by the buf parameter will contain this data received.
    **         If the connection has been gracefully closed, the return value is [0].
    */
    int recv(void* buf, int len, int flags = 0) const;

    
    /* @brief: nonblock recv
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes recvived, 
    **         Oterwise, If retval <=0, mean error occured, and should close socket. 
    */
    int recv_n(void* buf, int len, timeval* timeout, int flags = 0) const;
    static int recv_n(socket_native_type s, void* buf, int len, timeval* timeout, int flags = 0);


    /* @brief: Sends data on this connected socket
    ** @params:
    **       slicelen: slice length;
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes sent, 
    **         which can be less than the number requested to be sent in the len parameter. 
    **         Otherwise, a value of SOCKET_ERROR is returned.
    */
    int send(void* buf, int len, int slicelen, int flags = 0) const;


    /* @brief: Receives data from this connected socket or a bound connectionless socket. 
    ** @params: slicelen: slice length
    **           
    ** @returns: 
    **         If no error occurs, recv returns the number of bytes received and 
    **         the buffer pointed to by the buf parameter will contain this data received.
    **         If the connection has been gracefully closed, the return value is [0].
    */
    int recv(void* buf, int len, int slicelen, int flags = 0) const;


    /* @brief: Sends data on this connected socket
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes sent, 
    **         which can be less than the number requested to be sent in the len parameter. 
    **         Otherwise, a value of SOCKET_ERROR is returned.
    */
    int send_i(const void* buf, int len, int flags = 0) const;
    static int send_i(socket_native_type fd, const void* buf, int len, int flags = 0);


    /* @brief: Receives data from this connected socket or a bound connectionless socket. 
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, recv returns the number of bytes received and 
    **         the buffer pointed to by the buf parameter will contain this data received.
    **         If the connection has been gracefully closed, the return value is [0].
    */
    int recv_i(void* buf, int len, int flags = 0) const;
    static int recv_i(socket_native_type s, void* buf, int len, int flags);


    /* @brief: Sends data on this connected socket
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, send returns the total number of bytes sent, 
    **         which can be less than the number requested to be sent in the len parameter. 
    **         Otherwise, a value of SOCKET_ERROR is returned.
    */
    int sendto_i(const void* buf, int len, ip::endpoint_v4& to, int flags = 0) const;


    /* @brief: Receives a datagram and stores the source address
    ** @params: omit
    **           
    ** @returns: 
    **         If no error occurs, recv returns the number of bytes received and 
    **         the buffer pointed to by the buf parameter will contain this data received.
    **         If the connection has been gracefully closed, the return value is [0].
    */
    int recvfrom_i(void* buf, int len, ip::endpoint_v4& peer, int flags = 0) const;

    int handle_write_ready(timeval* timeo) const;
    static int handle_write_ready(socket_native_type s, timeval* timeo);
  
    int handle_read_ready(timeval* timeo) const;
    static int handle_read_ready(socket_native_type s, timeval* timeo);

    /* @brief: Get local address info
    ** @params : None
    **   
    ** @returns: 
    */ 
    ip::endpoint_v4 get_local_addr(void) const;
    static ip::endpoint_v4 get_local_addr(socket_native_type);


    /* @brief: Get peer address info
    ** @params : None
    **   
    ** @returns: 
    *  @remark: if this a listening socket fd, will return "0.0.0.0:0"
    */
    ip::endpoint_v4 get_peer_addr(void) const;
    static ip::endpoint_v4 get_peer_addr(socket_native_type);


    /* @brief: Configure TCP keepalive
    ** @params : flag:     1.on, 0.off
    **           idle:     time to send keepalive when no data interaction
    **           interval: keepalive send interval
    **           probes:   times to try when no response
    **   
    ** @returns: [0].successfully
    **          [<0].one or more errors occured
    */
    int set_keepalive(int flag = 1, int idle = 7200, int interval = 75, int probes = 10);
    static int set_keepalive(socket_native_type s, int flag = 1, int idle = 7200, int interval = 75, int probes = 10);


    /* @brief: Sets the socket option
    ** @params : 
    **        level: The level at which the option is defined (for example, SOL_SOCKET).
    **      optname: The socket option for which the value is to be set (for example, SO_BROADCAST). 
    **               The optname parameter must be a socket option defined within the specified level, 
    **               or behavior is undefined. 
    **       optval: The option value.
    **
    ** @returns: If no error occurs, getsockopt returns zero. Otherwise, a value of SOCKET_ERROR is returned
    */
    template<typename T>
    int set_optval(int level, int optname, const T& optval); 

    template<typename T>
    static int set_optval(socket_native_type, int level, int optname, const T& optval);


    /* @brief: Retrieves a socket option
    ** @params : 
    **     level: The level at which the option is defined. Example: SOL_SOCKET. 
    **   optname: The socket option for which the value is to be retrieved. 
    **            Example: SO_ACCEPTCONN. The optname value must be a socket option defined within the specified level, or behavior is undefined. 
    **    optval: A variable to the buffer in which the value for the requested option is to be returned.
    **    
    ** @returns: If no error occurs, getsockopt returns zero. Otherwise, a value of SOCKET_ERROR is returned
    */
    template<typename T>
    int get_optval(int level, int optname, T& optval, socklen_t = sizeof(T)) const;

    template<typename T>
    static int get_optval(socket_native_type s, int level, int optname, T& optval, socklen_t = sizeof(T));

    
    /* @brief: control the socket
    ** @params : 
    **          see MSDN or man page
    ** @returns: If no error occurs, getsockopt returns zero. Otherwise, a value of SOCKET_ERROR is returned
    **
    **
    **
    */
    template<typename _T>
    int ioctl(long cmd, const _T& argp) const;

    template<typename _T>
    static int ioctl(socket_native_type s, long cmd, const _T& argp);


    /* @brief: is a clinet socket alive
    ** @params : 
    **          see MSDN or man page
    ** @returns: If no error occurs, getsockopt returns zero. Otherwise, a value of SOCKET_ERROR is returned
    */
    bool is_alive(void) const;


    /* @brief: Disables sends or receives on this socket
    ** @params: 
    **        how: [SD_SEND] or [SD_RECEIVE] or [SD_BOTH]
    **
    ** @returns: [0] succeed, otherwise, a value of SOCKET_ERROR is returned.
    */
    int shutdown(int how = SD_BOTH) const;


   /* @brief: close sends
    ** @params: 
    **        non
    **
    ** @returns: [0] succeed, otherwise, a value of SOCKET_ERROR is returned.
    */
    void close(void);

    operator socket_native_type(void) const;

	/// <summary>
	/// this function just for windows platform
	/// </summary>
    static void init_ws32_lib(void);

private:
    socket_native_type   fd;
};

template<typename T> inline
    int xxsocket::set_optval(int level, int optname, const T& optval)
{
    return set_optval(this->fd, level, optname, optval);
}

template<typename T> inline
    int xxsocket::get_optval(int level, int optname, T& optval, socklen_t optlen) const
{
    return get_optval(this->fd, level, optname, optval, optlen);
}

template<typename T> inline
    int xxsocket::set_optval(socket_native_type s, int level, int optname, const T& optval)
{
    return ::setsockopt(s, level, optname, (char*)&optval, sizeof(T));
}

template<typename T> inline
    int xxsocket::get_optval(socket_native_type s, int level, int optname, T& optval, socklen_t optlen)
{
    return ::getsockopt(s, level, optname, (char*)&optval, &optlen);
}

template<typename _T> inline
    int xxsocket::ioctl(long cmd, const _T& value) const
    {
        return xxsocket::ioctl(this->fd, cmd, value);
    }

    template<typename _T> inline
    int xxsocket::ioctl(socket_native_type s, long cmd, const _T& value)
    {
        u_long argp = value;
        return ::ioctlsocket(s, cmd, &argp);
    }

}; /* namespace: simplepp_1_13_201307::net */

}; /* namespace: simplepp_1_13_201307 */

#include "endian.h"

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

