#include "xxsocket.h"

using namespace thelib;
using namespace thelib::net;

#ifdef _WIN32
extern LPFN_ACCEPTEX __accept_ex;
extern LPFN_GETACCEPTEXSOCKADDRS __get_accept_ex_sockaddrs;
#endif

xxsocket::xxsocket(void) : fd(bad_sock)
{
}

xxsocket::xxsocket(socket_native_type h) : fd(h)
{
}

xxsocket::xxsocket(const xxsocket& right) : fd(bad_sock)
{
    this->replace(const_cast<xxsocket&>(right));
}

xxsocket& xxsocket::operator=(socket_native_type handle)
{
    if(!this->is_open()) {
        this->fd = handle;
    }
    return *this;
}

xxsocket& xxsocket::operator=(const xxsocket& right)
{
    return this->replace(const_cast<xxsocket&>(right));
}

xxsocket::xxsocket(int af, int type, int protocol) : fd(bad_sock)
{
    open(af, type, protocol);
}

xxsocket::~xxsocket(void)
{
    close();
}

xxsocket& xxsocket::replace(xxsocket& who)
{
    // avoid fd missing
    if(!is_open()) {
        this->fd = who.fd;
        who.fd = bad_sock;
    }
    return *this;
}

bool xxsocket::open(int af, int type, int protocol)
{
    if(bad_sock == this->fd) 
    {
        this->fd = ::socket(af, type, protocol);
    }
    return is_open();
}

#ifdef _WIN32
bool xxsocket::open_ex(int af, int type, int protocol)
{
    if(bad_sock == this->fd)
    {
        this->fd = ::WSASocket(af, type, protocol, nullptr, 0, WSA_FLAG_OVERLAPPED);

        DWORD dwBytes = 0;
        if(nullptr == __accept_ex)
        {
            GUID guidAcceptEx = WSAID_ACCEPTEX;
            (void)WSAIoctl(
                this->fd, 
                SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &guidAcceptEx, 
                sizeof(guidAcceptEx), 
                &__accept_ex, 
                sizeof(__accept_ex), 
                &dwBytes, 
                nullptr, 
                nullptr);
        }

        if(nullptr == __get_accept_ex_sockaddrs)
        {
            GUID guidGetAcceptExSockaddrs = WSAID_GETACCEPTEXSOCKADDRS;
            (void)WSAIoctl(
                this->fd, 
                SIO_GET_EXTENSION_FUNCTION_POINTER, 
                &guidGetAcceptExSockaddrs, 
                sizeof(guidGetAcceptExSockaddrs), 
                &__get_accept_ex_sockaddrs, 
                sizeof(__get_accept_ex_sockaddrs), 
                &dwBytes, 
                nullptr, 
                nullptr);
        }
    }
    return is_open();
}

bool xxsocket::accept_ex(
    __in SOCKET sockfd_listened,
    __in SOCKET sockfd_prepared,
    __in PVOID lpOutputBuffer,
    __in DWORD dwReceiveDataLength,
    __in DWORD dwLocalAddressLength,
    __in DWORD dwRemoteAddressLength,
    __out LPDWORD lpdwBytesReceived,
    __inout LPOVERLAPPED lpOverlapped)
{
    return __accept_ex(sockfd_listened, 
        sockfd_prepared,
        lpOutputBuffer,
        dwReceiveDataLength,
        dwLocalAddressLength,
        dwRemoteAddressLength,
        lpdwBytesReceived,
        lpOverlapped) != FALSE;
}

void xxsocket::translate_sockaddrs(
    PVOID lpOutputBuffer,
    DWORD dwReceiveDataLength,
    DWORD dwLocalAddressLength,
    DWORD dwRemoteAddressLength,
    sockaddr **LocalSockaddr,
    LPINT LocalSockaddrLength,
    sockaddr **RemoteSockaddr,
    LPINT RemoteSockaddrLength)
{
    __get_accept_ex_sockaddrs( 
        lpOutputBuffer,
        dwReceiveDataLength,
        dwLocalAddressLength,
        dwRemoteAddressLength,
        LocalSockaddr,
        LocalSockaddrLength,
        RemoteSockaddr,
        RemoteSockaddrLength);
}

#endif

bool xxsocket::is_open(void) const
{
    return this->fd != bad_sock;
}

socket_native_type xxsocket::release(void)
{
    socket_native_type result = this->fd;
    this->fd = bad_sock;
    return result;
}

socket_native_type xxsocket::get_handle(void) const
{
    return this->fd;
}

socket_native_type xxsocket::native_handle(void) const
{
    return this->fd;
}

int xxsocket::set_nonblocking(bool nonblocking) const
{
    return set_nonblocking(this->fd, nonblocking);
}

int xxsocket::set_nonblocking(socket_native_type s, bool nonblocking)
{
    u_long argp = nonblocking;
    return ::ioctlsocket(s, FIONBIO, &argp);
}

int xxsocket::bind(const char* addr, unsigned short port) const
{
    ip::endpoint_v4 local(addr, port);

    return ::bind(this->fd, &local.internal, sizeof(local));
}

int xxsocket::listen(int backlog) const
{
    return ::listen(this->fd, backlog);
}

xxsocket xxsocket::accept(socklen_t )
{
    return ::accept(this->fd, nullptr, nullptr);
}

xxsocket xxsocket::accept_n(timeval& timeout)
{
    xxsocket result;

    set_nonblocking(true);

    fd_set fds_rd; 
    FD_ZERO(&fds_rd);
    FD_SET(this->fd, &fds_rd);

    if(::select(1, &fds_rd, 0, 0, &timeout) > 0) 
    {
        result = this->accept();
    }

    set_nonblocking(false);

    return result;
}

int xxsocket::connect(const char* addr, u_short port)
{
    return connect(this->fd, addr, port);
}

int xxsocket::connect(socket_native_type s, const char* addr, u_short port)
{
    ip::endpoint_v4 peer(addr, port);

    return ::connect(s, &peer.internal, sizeof(peer));
}

int xxsocket::connect_n(const char* addr, u_short port,  timeval& timeout)
{
    if(connect_n(this->fd, addr, port, timeout) != 0) {
        this->fd = bad_sock;
        return -1;
    }
    return 0;
}

int xxsocket::connect_n(socket_native_type s,const char* addr, u_short port, timeval& timeout)
{
    set_nonblocking(s, true);

    (void)connect(s, addr, port); // ignore return value

    fd_set fds_wr; 
    FD_ZERO(&fds_wr);
    FD_SET(s, &fds_wr);

    if(::select(1, nullptr, &fds_wr, nullptr, &timeout) > 0 && FD_ISSET(s, &fds_wr))
    { // connect successfully
        set_nonblocking(s, false);
        return 0;
    }

    // otherwise timeout or error occured
    if(s != bad_sock) 
        ::closesocket(s);
    return -1;
}

int xxsocket::get_sndbuf_size(void) const
{
    int size = -1;
    this->get_optval(SOL_SOCKET, SO_SNDBUF, size);
    return size;
}

int xxsocket::get_rcvbuf_size(void) const
{
    int size = -1;
    this->get_optval(SOL_SOCKET, SO_RCVBUF, size);
    return size;
}

void xxsocket::set_sndbuf_size(int size)
{
    this->set_optval(SOL_SOCKET, SO_SNDBUF, size);
}

void xxsocket::set_rcvbuf_size(int size)
{
    this->set_optval(SOL_SOCKET, SO_RCVBUF, size);
}

int xxsocket::send(const void* buf, int len, int flags) const
{
    int bytes_transferred = 0;
    int n = 0;
    do 
    {
        bytes_transferred += 
            ( n = ::send(this->fd,
            (char*)buf + bytes_transferred,
            len - bytes_transferred,
            flags
            ) );
    } while(bytes_transferred < len && n > 0);
    return bytes_transferred;
}

int xxsocket::send_n(const void* buf, int len, timeval* timeout, int flags)
{
    return xxsocket::send_n(this->fd, buf, len, timeout, flags);
}

int xxsocket::send_n(socket_native_type s, const void* buf, int len, timeval* timeout, int flags)
{
    int bytes_transferred;
    int n;
    int errcode = 0;
    int send_times = 0;

    for (bytes_transferred = 0;
        bytes_transferred < len;
        bytes_transferred += n)
    {
        // Try to transfer as much of the remaining data as possible.
        // Since the socket is in non-blocking mode, this call will not
        // block.
        n = xxsocket::send_i (s,
            (char *) buf + bytes_transferred,
            len - bytes_transferred,
            flags);
        //++send_times;
        // Check for errors.
        if (n <= 0)
        {
            // Check for possible blocking.
#ifdef _WIN32
            errcode = WSAGetLastError();       
#else
            errcode = errno;
#endif
            if (n == -1 &&
                (errcode == EAGAIN || errcode == EWOULDBLOCK || errcode == ENOBUFS || errcode == EINTR))
            {
                // Wait upto <timeout> for the blocking to subside.
                int const rtn = handle_write_ready (s, timeout);

                // Did select() succeed?
                if (rtn != -1)
                {
                    // Blocking subsided in <timeout> period.  Continue
                    // data transfer.
                    n = 0;
                    continue;
                }
            }

            // Wait in select() timed out or other data transfer or
            // select() failures.
            return n;
        }
    }

    return bytes_transferred;
}

int xxsocket::recv(void* buf, int len, int flags) const
{
    int bytes_transfrred = 0;
    int n = 0;
    do
    {
        bytes_transfrred += 
            ( n = ::recv(this->fd,
            (char*)buf + bytes_transfrred,
            len - bytes_transfrred,
            flags
            ) );

    } while(bytes_transfrred < len && n > 0);
    return bytes_transfrred;
}

int xxsocket::recv_n(void* buf, int len, timeval* timeout, int flags) const
{
    return recv_n(this->fd, buf, len, timeout, flags);
}

int xxsocket::recv_n(socket_native_type s, void* buf, int len, timeval* timeout, int flags)
{
    int bytes_transferred;
    int n;
    int errcode = 0;

    for (bytes_transferred = 0;
        bytes_transferred < len;
        bytes_transferred += n)
    {
        // Try to transfer as much of the remaining data as possible.
        // Since the socket is in non-blocking mode, this call will not
        // block.
        n = recv_i(s,
            static_cast <char *> (buf) + bytes_transferred,
            len - bytes_transferred,
            flags);

        // Check for errors.
        if (n <= 0)
        {
            // Check for possible blocking.
#ifdef _WIN32
            errcode = WSAGetLastError();       
#else
            errcode = errno;
#endif
            if (n == -1 &&
                errcode == EAGAIN || errcode == EINTR)
            {
                // Wait upto <timeout> for the blocking to subside.
                int const rtn = handle_read_ready (s, timeout);

                // Did select() succeed?
                if (rtn != -1)
                {
                    // Blocking subsided in <timeout> period.  Continue
                    // data transfer.
                    n = 0;
                    continue;
                }
            }

            // Wait in select() timed out or other data transfer or
            // select() failures.
            return n;
        }
    }

    return bytes_transferred;
}

int xxsocket::send(void* buf, int len, int slicelen, int flags) const
{
    char* ptr = (char*)buf;
    int bytes_leavings = len;
    while(bytes_leavings > 0)
    {
        if(bytes_leavings > slicelen)
        {
            bytes_leavings -= this->send(ptr, slicelen, flags);
            ptr += slicelen;
        }
        else
        {
            bytes_leavings -= this->send(ptr, bytes_leavings, flags);
        }
    }
    return len - bytes_leavings;
}

int xxsocket::recv(void* buf, int len, int slicelen, int flags) const
{
    char* ptr = (char*)buf;
    int bytes_leavings = len;
    while(bytes_leavings > 0)
    {
        if(bytes_leavings > slicelen)
        {
            bytes_leavings -= this->recv_i(ptr, slicelen, flags);
            ptr += slicelen;
        }
        else
        {
            bytes_leavings -= this->recv_i(ptr, bytes_leavings, flags);
        }
    }
    return len - bytes_leavings;
}

int xxsocket::send_i(const void* buf, int len, int flags) const
{
    return ::send(
        this->fd,
        (const char*)buf,
        len,
        flags);
}

int xxsocket::send_i(socket_native_type s, const void* buf, int len, int flags)
{
    return ::send(s,
        (const char*)buf,
        len,
        flags);
}

int xxsocket::recv_i(void* buf, int len, int flags) const
{
    return recv_i(this->fd, buf, len, flags);
}

int xxsocket::recv_i(socket_native_type s, void* buf, int len, int flags)
{
    return ::recv(
        s,
        (char*)buf,
        len,
        flags);
}

int xxsocket::recvfrom_i(void* buf, int len, ip::endpoint_v4& from, int flags) const
{
    socklen_t addrlen = sizeof(from);
    return ::recvfrom(this->fd,
        (char*)buf,
        len,
        flags,
        &from.internal,
        &addrlen
        );
}

int xxsocket::sendto_i(const void* buf, int len, ip::endpoint_v4& to, int flags) const
{
    return ::sendto(this->fd,
        (const char*)buf,
        len,
        flags,
        &to.internal,
        sizeof(to)
        );
}


int xxsocket::handle_write_ready(timeval* timeo) const
{
    return handle_write_ready(this->fd, timeo);
}

int xxsocket::handle_write_ready(socket_native_type s, timeval* timeo)
{
    fd_set fds_wr;
    FD_ZERO(&fds_wr);
    FD_SET(s, &fds_wr);
    int ret = ::select(s + 1, nullptr, &fds_wr, nullptr, timeo);
    return ret;
}

int xxsocket::handle_read_ready(timeval* timeo) const
{
    /*fd_set fds_rd;
    FD_ZERO(&fds_rd);
    FD_SET(this->fd, &fds_rd);
    int ret = ::select(this->fd + 1, &fds_rd, nullptr, nullptr, timeo);
    return ret;*/
    return handle_read_ready(this->fd, timeo);
}

int xxsocket::handle_read_ready(socket_native_type s, timeval* timeo)
{
    fd_set fds_rd;
    FD_ZERO(&fds_rd);
    FD_SET(s, &fds_rd);
    int ret = ::select(s + 1, &fds_rd, nullptr, nullptr, timeo);
    return ret;
}

ip::endpoint_v4 xxsocket::get_local_addr(void) const
{
    return get_local_addr(this->fd);
}

ip::endpoint_v4 xxsocket::get_local_addr(socket_native_type fd)
{
    ip::endpoint_v4 v4;
    socklen_t socklen = sizeof(v4);
    getsockname(fd, &v4.internal, &socklen);
    return v4;
}

ip::endpoint_v4 xxsocket::get_peer_addr(void) const
{
    return get_peer_addr(this->fd);
}

ip::endpoint_v4 xxsocket::get_peer_addr(socket_native_type fd)
{
    ip::endpoint_v4 v4;
    socklen_t socklen = sizeof(v4);
    getpeername(fd, &v4.internal, &socklen);
    return v4;
}

int xxsocket::set_keepalive(int flag, int idle, int interval, int probes)
{
    return set_keepalive(this->fd, flag, idle, interval, probes);
}

int xxsocket::set_keepalive(socket_native_type s, int flag, int idle, int interval, int probes)
{
#ifdef _WIN32
    tcp_keepalive buffer_in;
    buffer_in.onoff = flag;
    buffer_in.keepalivetime = idle;
    buffer_in.keepaliveinterval = interval;

    return WSAIoctl(s,
        SIO_KEEPALIVE_VALS,
        &buffer_in,
        sizeof(buffer_in),
        nullptr,
        0,
        (DWORD*)&probes,
        nullptr,
        nullptr);
#else
    int errcnt = 0;
    errcnt += set_optval(s, SOL_SOCKET, SO_KEEPALIVE, flag);
    //errcnt += set_optval(s, SOL_TCP, TCP_KEEPIDLE, idle);
    //errcnt += set_optval(s, SOL_TCP, TCP_KEEPINTVL, interval);
    //errcnt += set_optval(s, SOL_TCP, TCP_KEEPCNT, probes);
    return errcnt;
#endif
}

//int xxsocket::ioctl(long cmd, u_long* argp) const
//{
//    return ::ioctlsocket(this->fd, cmd, argp);
//}

xxsocket::operator socket_native_type(void) const
{
    return this->fd;
}

bool xxsocket::is_alive(void) const
{
    return this->send_i("", 0) != -1;
}

int xxsocket::shutdown(int how) const
{
    return ::shutdown(this->fd, how);
}

void xxsocket::close(void)
{
    if(is_open())
    {
        ::closesocket(this->fd);
        this->fd = bad_sock;
    }
}

void _naked_mark xxsocket::init_ws32_lib(void)
{
#if defined(_WIN32) && !defined(_WIN64)
    _asm ret;
#else
    return;
#endif
}

// initialize win32 socket library
#ifdef _WIN32
LPFN_ACCEPTEX __accept_ex = nullptr;
LPFN_GETACCEPTEXSOCKADDRS __get_accept_ex_sockaddrs = nullptr;
#endif

#ifdef _WIN32
namespace {

    struct ws2_32_gc
    {
        ws2_32_gc(void)
        {
            WSADATA dat;
            WSAStartup(0x0202, &dat);
        }
        ~ws2_32_gc(void)
        {
            WSACleanup();
        }
    };

    ws2_32_gc __ws32_lib_gc;
};
#endif

/* select usage:
char dat;
fd_set fds_rd;
FD_ZERO(&fds_rd);
FD_SET(fd, &fds_rd);
timeval timeo;
timeo.sec = 5;
timeo.usec = 500000;
switch( ::select(fd + 1, &fds_rd, nullptr, nullptr, &timeo) )
{
case -1:  // select error
break;
case 0:   // select timeout
break;
default:  // can read
if(sock.recv_i(&dat, sizeof(dat), MSG_PEEK) < 0)
{
return -1;
}
;
}
*/

