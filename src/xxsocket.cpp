#include "xxsocket.h"
#include <fcntl.h>

using namespace purelib;
using namespace purelib::net;

#if defined( _WIN32 ) && !defined( _WP8 )
extern LPFN_ACCEPTEX __accept_ex;
extern LPFN_GETACCEPTEXSOCKADDRS __get_accept_ex_sockaddrs;
#endif

xxsocket::xxsocket(void) : fd(bad_sock)
{
}

xxsocket::xxsocket(socket_native_type h) : fd(h)
{
}

xxsocket::xxsocket(xxsocket&& right) : fd(bad_sock)
{
    swap(right);
}

xxsocket& xxsocket::operator=(socket_native_type handle)
{
    if(!this->is_open()) {
        this->fd = handle;
    }
    return *this;
}

xxsocket& xxsocket::operator=(xxsocket&& right)
{
    return swap(right);
}

xxsocket::xxsocket(int af, int type, int protocol) : fd(bad_sock)
{
    open(af, type, protocol);
}

xxsocket::~xxsocket(void)
{
    close();
}

xxsocket& xxsocket::swap(xxsocket& who)
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
#if !defined(WP8)
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
#endif
    return false;
}

#if !defined(WP8)
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

xxsocket xxsocket::accept_n(timeval* timeout)
{
    xxsocket result;

    set_nonblocking(true);

    fd_set fds_rd; 
    FD_ZERO(&fds_rd);
    FD_SET(this->fd, &fds_rd);

    if(::select(1, &fds_rd, 0, 0, timeout) > 0) 
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

int xxsocket::connect_n(const char* addr, u_short port, long timeout_sec)
{
    auto timeout = make_tv(timeout_sec);
    return connect_n(addr, port, &timeout);
}

int xxsocket::connect_n(const char* addr, u_short port,  timeval* timeout)
{
    if(xxsocket::connect_n(this->fd, addr, port, timeout) != 0) {
        return -1;
    }
    return 0;
}

int xxsocket::connect_n(socket_native_type s,const char* addr, u_short port, timeval* timeout)
{
//#ifdef _WIN32
//    set_nonblocking(s, true);
//
//    (void)connect(s, addr, port); // ignore return value
//
//    fd_set fds_wr; 
//    FD_ZERO(&fds_wr);
//    FD_SET(s, &fds_wr);
//
//    if (::select(s + 1, nullptr, &fds_wr, nullptr, timeout) > 0 && FD_ISSET(s, &fds_wr))
//    { // connect successfully
//        set_nonblocking(s, false);
//        return 0;
//    }
//
//    // otherwise timeout or error occured
//    if(s != bad_sock) 
//        ::closesocket(s);
//    return -1;
//#else
    fd_set rset, wset;
    int n, error = 0;
#ifdef _WIN32
    set_nonblocking(s, true);
#else
    int flags = ::fcntl(s, F_GETFL, 0);
    ::fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
    if ((n = xxsocket::connect(s, addr, port)) < 0) {
        error = xxsocket::get_last_errno();
        if (error != EINPROGRESS && error != EWOULDBLOCK)
            return -1;
    }

    /* Do whatever we want while the connect is taking place. */
    if (n == 0)
        goto done; /* connect completed immediately */

    FD_ZERO(&rset);
    FD_SET(s, &rset);
    wset = rset;

    if ((n = ::select(s + 1, &rset, &wset, NULL, timeout)) == 0) {
        ::closesocket(s);  /* timeout */
        xxsocket::set_last_errno(ETIMEDOUT);
        return (-1);
    }

    if (FD_ISSET(s, &rset) || FD_ISSET(s, &wset)) {
        socklen_t len = sizeof(error);
        if (::getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
            return (-1);  /* Solaris pending error */
    }
    else
        return -1;
done:
#ifdef _MSC_VER
    set_nonblocking(s, false);
#else
    ::fcntl(s, F_SETFL, flags);  /* restore file status flags */
#endif

    if (error != 0) {
        ::closesocket(s); /* just in case */
        xxsocket::set_last_errno(error);
        return (-1);
    }
    return (0);

//#endif
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

int xxsocket::send_n(const void* buf, int len, long timeout_sec, int flags)
{
    auto timeout = make_tv(timeout_sec);
    return send_n(this->fd, buf, len, &timeout, flags);
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

int xxsocket::recv_n(void* buf, int len, long timeout_sec, int flags) const
{
    auto timeout = make_tv(timeout_sec);
    return recv_n(this->fd, buf, len, &timeout, flags);
}

int xxsocket::recv_n(void* buf, int len, timeval* timeout, int flags) const
{
    return recv_n(this->fd, buf, len, timeout, flags);
}

int xxsocket::recv_n(socket_native_type s, void* buf, int len, timeval* timeout, int flags)
{
    int bytes_transferred;
    int n;
    int ec = 0;

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
            ec = WSAGetLastError();       
#else
            ec = errno; // socket errno
#endif
            if (n == -1 &&
                ( ec == EAGAIN 
                || ec == EINTR 
                || ec == EWOULDBLOCK
                || ec == EINPROGRESS) )
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

int xxsocket::handle_connect_ready(socket_native_type s, timeval* timeo)
{
    fd_set fds_wr;
    FD_ZERO(&fds_wr);
    FD_SET(s, &fds_wr);

    if (::select(0, nullptr, &fds_wr, nullptr, timeo) > 0 && FD_ISSET(s, &fds_wr))
    { // connect successfully
        return 0;
    }
    return -1;
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

ip::endpoint_v4 xxsocket::local_endpoint(void) const
{
    return local_endpoint(this->fd);
}

ip::endpoint_v4 xxsocket::local_endpoint(socket_native_type fd)
{
    ip::endpoint_v4 v4;
    socklen_t socklen = sizeof(v4);
    getsockname(fd, &v4.internal, &socklen);
    return v4;
}

ip::endpoint_v4 xxsocket::peer_endpoint(void) const
{
    return peer_endpoint(this->fd);
}

ip::endpoint_v4 xxsocket::peer_endpoint(socket_native_type fd)
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
#if defined(_WIN32) && !defined(WP8)
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

bool xxsocket::available(void) const
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

int xxsocket::get_last_errno(void)
{
#if defined(_WIN32)
    return ::WSAGetLastError();
#else
    return errno;
#endif
}

void xxsocket::set_last_errno(int error)
{
#if defined(_WIN32)
    ::WSASetLastError(error);
#else
    errno = error;
#endif
}

const char* xxsocket::get_error_msg(int error)
{
#if defined(_MSC_VER)
    static char error_msg[256];
    /*LPVOID lpMsgBuf = nullptr;*/
    ::FormatMessageA(
        /*FORMAT_MESSAGE_ALLOCATE_BUFFER |*/
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), // english language
        error_msg,
        sizeof(error_msg),
        NULL
        );

    /*if (lpMsgBuf != nullptr) {
        strcpy(error_msg, (const char*)lpMsgBuf);
        ::LocalFree(lpMsgBuf);
    }*/
    return error_msg;
#else
    return strerror(error);
#endif
}

// initialize win32 socket library
#if defined(_WIN32 ) && !defined(WP8)
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

