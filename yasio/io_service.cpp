//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2024 HALX99

HAL: Hardware Abstraction Layer
X99: Intel X99 Mainboard Platform

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef YASIO__IO_SERVICE_CPP
#define YASIO__IO_SERVICE_CPP
#if !defined(YASIO_HEADER_ONLY)
#  include "yasio/io_service.hpp"
#endif
#include <limits>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "yasio/thread_name.hpp"

#if defined(YASIO_SSL_BACKEND)
#  include "yasio/ssl.hpp"
#endif

#include "yasio/wtimer_hres.hpp"

#if defined(YASIO_ENABLE_KCP)
struct yasio_kcp_options {
  int kcp_conv_ = 0;

  int kcp_nodelay_  = 1;
  int kcp_interval_ = 10; // 10~100ms
  int kcp_resend_   = 2;
  int kcp_ncwnd_    = 1;

  int kcp_sndwnd_ = 32;
  int kcp_rcvwnd_ = 128;

  int kcp_mtu_ = 1400;
  // kcp fast model the RTO min is 30.
  int kcp_minrto_ = 30;
};
#endif

#if defined(YASIO_USE_CARES)
#  include "yasio/impl/ares.hpp"
#endif

// clang-format off
#define YASIO_KLOG_CP(level, format, ...)                                                                                      \
  do                                                                                                                           \
  {                                                                                                                            \
    auto& __cprint = __get_cprint();                                                                                           \
    auto __msg           = ::yasio::strfmt(127, "[yasio][%lld]" format "\n", ::yasio::clock<system_clock_t>(), ##__VA_ARGS__); \
    if (__cprint)                                                                                                              \
      __cprint(level, __msg.c_str());                                                                                          \
    else                                                                                                                       \
      YASIO_LOG_TAG("", "%s", __msg.c_str());                                                                                  \
  } while (false)
// clang-format on

#define YASIO_KLOGD(format, ...) YASIO_KLOG_CP(YLOG_D, format, ##__VA_ARGS__)
#define YASIO_KLOGI(format, ...) YASIO_KLOG_CP(YLOG_I, format, ##__VA_ARGS__)
#define YASIO_KLOGW(format, ...) YASIO_KLOG_CP(YLOG_W, format, ##__VA_ARGS__)
#define YASIO_KLOGE(format, ...) YASIO_KLOG_CP(YLOG_E, format, ##__VA_ARGS__)

#if !defined(YASIO_VERBOSE_LOG)
#  define YASIO_KLOGV(fmt, ...) (void)0
#else
#  define YASIO_KLOGV(format, ...) YASIO_KLOG_CP(YLOG_V, format, ##__VA_ARGS__)
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 6320 6322 4996)
#endif

namespace yasio
{

YASIO__NS_INLINE
namespace inet
{
namespace
{

enum : uint8_t
{ // op masks and stop flags
  YOPM_OPEN     = 1,
  YOPM_CLOSE    = 1 << 1,
  YSTF_STOP     = 1 << 2,
  YSTF_FINALIZE = 1 << 3
};

enum
{
  /* whether udp server enable multicast service */
  YCPF_MCAST = 1 << 17,

  /* whether multicast loopback, if 1, local machine can recv self multicast packet */
  YCPF_MCAST_LOOPBACK = 1 << 18,

  /* whether host dirty */
  YCPF_HOST_DIRTY = 1 << 19,

  /* whether port dirty */
  YCPF_PORT_DIRTY = 1 << 20,

  /* host is domain name, needs query via internet */
  YCPF_NEEDS_QUERY = 1 << 21,

  /// below is byte2 of private flags (25~32) are mutable, and will be cleared automatically when connect flow done, see clear_mutable_flags.
};

namespace
{
// By default we will wait no longer than 5 minutes. This will ensure that
// any changes to the system clock are detected after no longer than this.
static const int yasio__max_wait_usec = 5 * 60 * 1000 * 1000LL;
// the max transport alloc size
static const size_t yasio__max_tsize = (std::max)({sizeof(io_transport_tcp), sizeof(io_transport_udp), sizeof(io_transport_ssl), sizeof(io_transport_kcp)});
static const int yasio__udp_mss = static_cast<int>((std::numeric_limits<uint16_t>::max)() - (sizeof(yasio::ip::ip_hdr_st) + sizeof(yasio::ip::udp_hdr_st)));
} // namespace
struct yasio__global_state {
  enum
  {
    INITF_SSL   = 1,
    INITF_CARES = 2,
  };
  yasio__global_state(const print_fn2_t& custom_print)
  {
    auto __get_cprint = [&]() -> const print_fn2_t& { return custom_print; };
#if defined(YASIO_SSL_BACKEND) && YASIO_SSL_BACKEND == 1
#  if OPENSSL_VERSION_NUMBER >= 0x10100000 && !defined(LIBRESSL_VERSION_NUMBER)
    if (OPENSSL_init_ssl(0, nullptr) == 1)
    {
      yasio__setbits(this->init_flags_, INITF_SSL);
      ERR_clear_error();
    }
#  endif
#endif
#if defined(YASIO_USE_CARES)
    int ares_status = ::ares_library_init(ARES_LIB_INIT_ALL);
    if (ares_status == 0)
      yasio__setbits(init_flags_, INITF_CARES);
    else
      YASIO_KLOGI("[global] c-ares library init failed, status=%d, detail:%s", ares_status, ::ares_strerror(ares_status));
#  if defined(__ANDROID__)
    ares_status = ::yasio__ares_init_android();
    if (ares_status != 0)
      YASIO_KLOGI("[global] c-ares library init android failed, status=%d, detail:%s", ares_status, ::ares_strerror(ares_status));
#  endif
#endif
    // print version & transport alloc size
    YASIO_KLOGI("[global] the yasio-%x.%x.%x is initialized, the size of per transport is %d when object_pool enabled.", (YASIO_VERSION_NUM >> 16) & 0xff,
                (YASIO_VERSION_NUM >> 8) & 0xff, YASIO_VERSION_NUM & 0xff, yasio__max_tsize);
    YASIO_KLOGI("[global] sizeof(io_event)=%d", sizeof(io_event));
  }
  ~yasio__global_state()
  {
#if defined(YASIO_USE_CARES)
    if (yasio__testbits(this->init_flags_, INITF_CARES))
      ::ares_library_cleanup();
#endif
  }

  int init_flags_ = 0;
  print_fn2_t cprint_;
};
static yasio__global_state& yasio__shared_globals(const print_fn2_t& prt = nullptr)
{
  static yasio__global_state __global_state(prt);
  return __global_state;
}
} // namespace

/// highp_timer
void highp_timer::async_wait(timer_cb_t cb) { service_.schedule_timer(this, std::move(cb)); }
void highp_timer::cancel()
{
  if (!expired())
    service_.remove_timer(this);
}

std::chrono::microseconds highp_timer::wait_duration() const
{
  return std::chrono::duration_cast<std::chrono::microseconds>(this->expire_time_ - service_.current_time_);
}

/// io_send_op
int io_send_op::perform(io_transport* transport, const void* buf, int n, int& error) { return transport->write_cb_(buf, n, nullptr, error); }

/// io_sendto_op
int io_sendto_op::perform(io_transport* transport, const void* buf, int n, int& error)
{
  return transport->write_cb_(buf, n, std::addressof(destination_), error);
}

/// io_channel
io_channel::io_channel(io_service& service, int index) : io_base(), service_(service), timer_(service), user_timer_(service)
{
  socket_     = std::make_shared<xxsocket>();
  state_      = io_base::state::CLOSED;
  index_      = index;
  decode_len_ = [this](void* ptr, int len) { return this->__builtin_decode_len(ptr, len); };
}
#if defined(YASIO_ENABLE_KCP)
io_channel::~io_channel()
{
  if (kcp_options_)
    delete kcp_options_;
}
yasio_kcp_options& io_channel::kcp_options()
{
  if (!kcp_options_)
    kcp_options_ = new yasio_kcp_options();
  return *kcp_options_;
}
#endif
#if defined(YASIO_SSL_BACKEND)
yssl_ctx_st* io_channel::get_ssl_context(bool client) const
{
  if (client)
    return service_.ssl_roles_[YSSL_CLIENT];
  auto& ctx = service_.ssl_roles_[YSSL_SERVER];
  return (ctx) ? ctx : service_.init_ssl_context(YSSL_SERVER);
}
#endif
const print_fn2_t& io_channel::__get_cprint() const { return get_service().options_.print_; }
std::string io_channel::format_destination() const
{
  if (yasio__testbits(properties_, YCPF_NEEDS_QUERY))
    return yasio::strfmt(127, "%s(%s):%u", remote_host_.c_str(), !remote_eps_.empty() ? remote_eps_[0].ip().c_str() : "undefined", remote_port_);

  return yasio::strfmt(127, "%s:%u", remote_host_.c_str(), remote_port_);
}
void io_channel::enable_multicast(const char* addr, int loopback)
{
  yasio__setbits(properties_, YCPF_MCAST);

  if (loopback)
    yasio__setbits(properties_, YCPF_MCAST_LOOPBACK);

  if (addr)
    multiaddr_.as_in(addr, (u_short)0);
}
void io_channel::join_multicast_group()
{
  if (socket_->is_open())
  {
    // interface
    switch (multiif_.af())
    {
      case AF_INET:
        socket_->set_optval(IPPROTO_IP, IP_MULTICAST_IF, multiif_.in4_.sin_addr);
        break;
      case AF_INET6:
        socket_->set_optval(IPPROTO_IPV6, IP_MULTICAST_IF, multiif_.in6_.sin6_scope_id);
        break;
    }

    int loopback = yasio__testbits(properties_, YCPF_MCAST_LOOPBACK) ? 1 : 0;
    socket_->set_optval(multiaddr_.af() == AF_INET ? IPPROTO_IP : IPPROTO_IPV6, multiaddr_.af() == AF_INET ? IP_MULTICAST_LOOP : IPV6_MULTICAST_LOOP, loopback);
    // ttl
    socket_->set_optval(multiaddr_.af() == AF_INET ? IPPROTO_IP : IPPROTO_IPV6, multiaddr_.af() == AF_INET ? IP_MULTICAST_TTL : IPV6_MULTICAST_HOPS,
                        YASIO_DEFAULT_MULTICAST_TTL);
    if (configure_multicast_group(true) != 0)
    {
      int ec = xxsocket::get_last_errno();
      YASIO_KLOGE("[index: %d] join to multicast group %s failed, ec=%d, detail:%s", this->index_, multiaddr_.to_string().c_str(), ec, xxsocket::strerror(ec));
    }
  }
}
void io_channel::disable_multicast()
{
  yasio__clearbits(properties_, YCPF_MCAST);
  yasio__clearbits(properties_, YCPF_MCAST_LOOPBACK);

  if (socket_->is_open())
    configure_multicast_group(false);
}
int io_channel::configure_multicast_group(bool onoff)
{
  if (multiaddr_.af() == AF_INET)
  { // ipv4
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = multiif_.in4_.sin_addr.s_addr;
    mreq.imr_multiaddr        = multiaddr_.in4_.sin_addr;
    return socket_->set_optval(IPPROTO_IP, onoff ? IP_ADD_MEMBERSHIP : IP_DROP_MEMBERSHIP, &mreq, (int)sizeof(mreq));
  }
  else
  { // ipv6
    struct ipv6_mreq mreq_v6;
    mreq_v6.ipv6mr_interface = multiif_.in6_.sin6_scope_id;
    mreq_v6.ipv6mr_multiaddr = multiaddr_.in6_.sin6_addr;
    return socket_->set_optval(IPPROTO_IPV6, onoff ? IPV6_JOIN_GROUP : IPV6_LEAVE_GROUP, &mreq_v6, (int)sizeof(mreq_v6));
  }
}
void io_channel::set_host(cxx17::string_view host)
{
  if (this->remote_host_ != host)
  {
    cxx17::assign(this->remote_host_, host);
    yasio__setbits(properties_, YCPF_HOST_DIRTY);
  }
}
void io_channel::set_port(u_short port)
{
  if (port == 0)
    return;
  if (this->remote_port_ != port)
  {
    this->remote_port_ = port;
    yasio__setbits(properties_, YCPF_PORT_DIRTY);
  }
}
int io_channel::__builtin_decode_len(void* d, int n)
{
  int loffset = uparams_.length_field_offset;
  int lsize   = uparams_.length_field_length;
  if (loffset >= 0)
  {
    assert(lsize >= 1 && lsize <= YASIO_SSIZEOF(int));
    int len = 0;
    if (n >= (loffset + lsize))
    {
      ::memcpy(&len, (uint8_t*)d + loffset, lsize);
      if (!uparams_.no_bswap)
        len = yasio::network_to_host(len, lsize);
      len += uparams_.length_adjustment;
      if (len > uparams_.max_frame_length)
        len = -1;
    }
    return len;
  }
  return n;
}
// -------------------- io_transport ---------------------
io_transport::io_transport(io_channel* ctx, xxsocket_ptr&& s) : ctx_(ctx)
{
  this->state_  = io_base::state::OPENED;
  this->socket_ = std::move(s);
#if !defined(YASIO_MINIFY_EVENT)
  this->ud_.ptr = nullptr;
#endif
}
const print_fn2_t& io_transport::__get_cprint() const { return ctx_->get_service().options_.print_; }
int io_transport::write(io_send_buffer&& buffer, completion_cb_t&& handler)
{
  int n = static_cast<int>(buffer.size());
  send_queue_.emplace(cxx14::make_unique<io_send_op>(std::move(buffer), std::move(handler)));
  get_service().wakeup();
  return n;
}
int io_transport::do_read(int revent, int& error, highp_time_t&) { return this->call_read(buffer_ + offset_, sizeof(buffer_) - offset_, revent, error); }
bool io_transport::do_write(highp_time_t& wait_duration)
{
  bool ret = false;
  do
  {
    if (!socket_->is_open())
      break;

    int error = 0;
    auto wrap = send_queue_.peek();
    if (wrap)
    {
      auto& v = *wrap;
      if (call_write(v.get(), error) < 0)
      {
        this->set_last_errno(error, yasio::io_base::error_stage::WRITE);
        break;
      }
    }

    bool no_wevent = send_queue_.empty();
    if (yasio__unlikely(!no_wevent))
    { // still have work to do
      no_wevent = (error != EWOULDBLOCK && error != EAGAIN && error != ENOBUFS);
      if (!no_wevent)
      { // system kernel buffer full
        if (!pollout_registerred_)
        {
          get_service().io_watcher_.mod_event(socket_->native_handle(), socket_event::write, 0);
          pollout_registerred_ = true;
        }
      }
      else
        wait_duration = 0;
    }
    if (no_wevent && pollout_registerred_)
    {
      get_service().io_watcher_.mod_event(socket_->native_handle(), 0, socket_event::write);
      pollout_registerred_ = false;
    }
    ret = true;
  } while (false);

  return ret;
}
int io_transport::call_read(void* data, int size, int revent, int& error)
{
  int n = read_cb_(data, size, revent, error);
  if (n > 0)
  {
    ctx_->bytes_transferred_ += n;
    return n;
  }
  if (n < 0)
  {
    if (xxsocket::not_recv_error(error))
      return (error = 0); // status ok, clear error
    return n;
  }
  if (yasio__testbits(ctx_->properties_, YCM_TCP))
  {
    error = yasio::errc::eof;
    return -1;
  }
  return 0;
}
int io_transport::call_write(io_send_op* op, int& error)
{
  int n = op->perform(this, op->buffer_.data() + op->offset_, static_cast<int>(op->buffer_.size() - op->offset_), error);
  if (n > 0)
  {
    // #performance: change offset only, remain data will be send at next frame.
    op->offset_ += n;
    if (op->offset_ == op->buffer_.size())
      this->complete_op(op, 0);
  }
  else if (n < 0)
  {
    if (xxsocket::not_send_error(error))
      n = 0;
    else if (yasio__testbits(ctx_->properties_, YCM_UDP))
    { // !!! For udp, simply drop the op instead trigger handle close,
      // on android device, the error will be 'EPERM' when app in background.
      this->complete_op(op, error);
      n = 0;
    }
  }
  return n;
}
void io_transport::complete_op(io_send_op* op, int error)
{
  YASIO_KLOGV("[index: %d] write complete, bytes transferred: %d/%d", this->cindex(), static_cast<int>(op->offset_), static_cast<int>(op->buffer_.size()));
  if (op->handler_)
    op->handler_(error, op->offset_);
  send_queue_.pop();
}
void io_transport::set_primitives()
{
  if (yasio__testbits(ctx_->properties_, YCM_TCP))
  {
    this->write_cb_ = [this](const void* data, int len, const ip::endpoint*, int& error) {
      int n = socket_->send(data, len, YASIO_MSG_FLAG);
      if (n < 0)
        error = xxsocket::get_last_errno();
      return n;
    };
  }
  else // UDP
  {
    this->write_cb_ = [this](const void* data, int len, const ip::endpoint*, int& error) {
      int n = socket_->send(data, (std::min)(len, yasio__udp_mss), YASIO_MSG_FLAG);
      if (n < 0)
        error = xxsocket::get_last_errno();
      return n;
    };
  }
  this->read_cb_ = [this](void* data, int len, int revent, int& error) {
    if (revent)
    {
      int n = socket_->recv(data, len);
      if (n < 0)
        error = xxsocket::get_last_errno();
      return n;
    }

    error = EWOULDBLOCK;
    return -1;
  };
}
// -------------------- io_transport_tcp ---------------------
inline io_transport_tcp::io_transport_tcp(io_channel* ctx, xxsocket_ptr&& s) : io_transport(ctx, std::forward<xxsocket_ptr>(s)) {}
// ----------------------- io_transport_ssl ----------------
#if defined(YASIO_SSL_BACKEND)
io_transport_ssl::io_transport_ssl(io_channel* ctx, xxsocket_ptr&& sock) : io_transport_tcp(ctx, std::forward<xxsocket_ptr>(sock))
{
  this->state_ = io_base::state::CONNECTING; // for ssl, inital state shoud be connecing for ssl handshake
  bool client  = yasio__testbits(ctx->properties_, YCM_CLIENT);
  this->ssl_   = yssl_new(ctx->get_ssl_context(client), static_cast<int>(this->socket_->native_handle()), ctx->remote_host_.c_str(), client);
}
int io_transport_ssl::do_ssl_handshake(int& error)
{
  int ret = yssl_do_handshake(ssl_, error);
  // handshake succeed, because we invoke handshake in call_read, so we emit EWOULDBLOCK to mark ssl transport status `ok`
  if (ret == 0 && !error)
  {
    this->state_   = io_base::state::OPENED;
    this->read_cb_ = [this](void* data, int len, int revent, int& error) {
      if (revent)
        return yssl_read(ssl_, data, len, error);
      error = EWOULDBLOCK;
      return -1;
    };
    this->write_cb_ = [this](const void* data, int len, const ip::endpoint*, int& error) { return yssl_write(ssl_, data, len, error); };

    YASIO_KLOGD("[index: %d] the connection #%u <%s> --> <%s> is established.", ctx_->index_, this->id_, this->local_endpoint().to_string().c_str(),
                this->remote_endpoint().to_string().c_str());
    get_service().fire_event(ctx_->index_, YEK_ON_OPEN, 0, this);

    error = EWOULDBLOCK;
  }
  else
  {
    if (error == EWOULDBLOCK)
      get_service().wakeup();
    else
    { // handshake failed, print reason
      char buf[256] = {0};
      YASIO_KLOGE("[index: %d] do_ssl_handshake fail with: %s", ctx_->index_, yssl_strerror(ssl_, ret, buf, sizeof(buf)));
      if (yasio__testbits(ctx_->properties_, YCM_CLIENT))
      {
        YASIO_KLOGE("[index: %d] connect server %s failed, ec=%d, detail:%s", ctx_->index_, ctx_->format_destination().c_str(), error,
                    io_service::strerror(error));
        get_service().fire_event(ctx_->index(), YEK_ON_OPEN, error, ctx_);
      }
    }
  }
  return -1;
}
void io_transport_ssl::do_ssl_shutdown()
{
  if (ssl_)
    yssl_shutdown(ssl_, this->error_ == yasio::errc::shutdown_by_localhost);
}
void io_transport_ssl::set_primitives()
{
  this->read_cb_ = [this](void* /*data*/, int /*len*/, int /*revent*/, int& error) { return do_ssl_handshake(error); };
}
#endif
// ----------------------- io_transport_udp ----------------
io_transport_udp::io_transport_udp(io_channel* ctx, xxsocket_ptr&& s) : io_transport(ctx, std::forward<xxsocket_ptr>(s)) {}
io_transport_udp::~io_transport_udp() {}
ip::endpoint io_transport_udp::remote_endpoint() const { return !connected_ ? this->peer_ : socket_->peer_endpoint(); }
const ip::endpoint& io_transport_udp::ensure_destination() const
{
  if (this->destination_.af() != AF_UNSPEC)
    return this->destination_;
  return (this->destination_ = this->peer_);
}
void io_transport_udp::confgure_remote(const ip::endpoint& peer)
{
  if (connected_) // connected, update peer is pointless and useless
    return;
  this->peer_ = peer;
  if (!yasio__testbits(ctx_->properties_, YCPF_MCAST) || !yasio__testbits(ctx_->properties_, YCM_CLIENT))
    this->connect(); // multicast client, don't bind multicast address for we can recvfrom non-multicast address
}
void io_transport_udp::connect()
{
  if (connected_)
    return;

  if (this->peer_.af() == AF_UNSPEC)
  {
    if (ctx_->remote_eps_.empty())
      return;
    this->peer_ = ctx_->remote_eps_[0];
  }

  int retval = this->socket_->connect_n(this->peer_);
  connected_ = (retval == 0);
  set_primitives();
}
void io_transport_udp::disconnect()
{
#if defined(__linux__)
  auto ifaddr = this->socket_->local_endpoint();
#endif
  int retval = this->socket_->disconnect();
#if defined(__linux__)
  if (retval == 0)
  { // Because some of linux will unbind when disconnect succeed, so try to rebind
    ifaddr.ip(ctx_->local_host_.empty() ? YASIO_ADDR_ANY(ifaddr.af()) : ctx_->local_host_.c_str());
    this->socket_->bind(ifaddr);
  }
#else
  YASIO__UNUSED_PARAM(retval);
#endif
  connected_ = false;
  set_primitives();
}
int io_transport_udp::write(io_send_buffer&& buffer, completion_cb_t&& handler)
{
  return connected_ ? io_transport::write(std::move(buffer), std::move(handler)) : write_to(std::move(buffer), ensure_destination(), std::move(handler));
}
int io_transport_udp::write_to(io_send_buffer&& buffer, const ip::endpoint& to, completion_cb_t&& handler)
{
  int n = static_cast<int>(buffer.size());
  send_queue_.emplace(cxx14::make_unique<io_sendto_op>(std::move(buffer), std::move(handler), to));
  get_service().wakeup();
  return n;
}
void io_transport_udp::set_primitives()
{
  if (connected_)
    io_transport::set_primitives();
  else
  {
    this->write_cb_ = [this](const void* data, int len, const ip::endpoint* destination, int& error) {
      assert(destination);
      int n = socket_->sendto(data, (std::min)(len, yasio__udp_mss), *destination);
      if (n < 0)
      {
        error = xxsocket::get_last_errno();
        if (!xxsocket::not_send_error(error))
          YASIO_KLOGW("[index: %d] write udp socket failed, ec=%d, detail:%s", this->cindex(), error, io_service::strerror(error));
      }
      return n;
    };
    this->read_cb_ = [this](void* data, int len, int revent, int& error) {
      if (revent)
      {
        ip::endpoint peer;
        int n = socket_->recvfrom(data, len, peer);
        if (n > 0)
          this->peer_ = peer;
        if (n < 0)
          error = xxsocket::get_last_errno();
        return n;
      }
      error = EWOULDBLOCK;
      return -1;
    };
  }
}
int io_transport_udp::handle_input(char* data, int bytes_transferred, int& /*error*/, highp_time_t&)
{ // pure udp, dispatch to upper layer directly
  auto& service = get_service();
  if (!service.options_.forward_packet_)
    service.fire_event(this->cindex(), io_packet{data, data + bytes_transferred}, this);
  else
    service.forward_packet(this->cindex(), io_packet_view{data, bytes_transferred}, this);
  return bytes_transferred;
}

#if defined(YASIO_ENABLE_KCP)
// ----------------------- io_transport_kcp ------------------
io_transport_kcp::io_transport_kcp(io_channel* ctx, xxsocket_ptr&& s) : io_transport_udp(ctx, std::forward<xxsocket_ptr>(s))
{
  auto& kopts = ctx->kcp_options();
  this->kcp_  = ::ikcp_create(static_cast<IUINT32>(kopts.kcp_conv_), this);
  ::ikcp_nodelay(this->kcp_, kopts.kcp_nodelay_, kopts.kcp_interval_, kopts.kcp_resend_, kopts.kcp_ncwnd_);
  ::ikcp_wndsize(this->kcp_, kopts.kcp_sndwnd_, kopts.kcp_rcvwnd_);
  ::ikcp_setmtu(this->kcp_, kopts.kcp_mtu_);
  // Because of nodelaying config will change the value. so setting RTO min after call ikcp_nodely.
  this->kcp_->rx_minrto = kopts.kcp_minrto_;

  this->rawbuf_.resize(yasio__max_rcvbuf);
  ::ikcp_setoutput(this->kcp_, [](const char* buf, int len, ::ikcpcb* /*kcp*/, void* user) {
    auto t         = (io_transport_kcp*)user;
    int ignored_ec = 0;
    return t->underlaying_write_cb_(buf, len, std::addressof(t->ensure_destination()), ignored_ec);
  });
}
io_transport_kcp::~io_transport_kcp() { ::ikcp_release(this->kcp_); }

void io_transport_kcp::set_primitives()
{
  io_transport_udp::set_primitives();
  underlaying_write_cb_ = write_cb_;
  write_cb_             = [this](const void* data, int len, const ip::endpoint*, int& error) {
    int nsent = ::ikcp_send(kcp_, static_cast<const char*>(data), len /*(std::min)(static_cast<int>(kcp_->mss), len)*/);
    if (nsent > 0)
    {
      ::ikcp_flush(kcp_);
      expire_time_ = 0;
    }
    else
      error = EMSGSIZE; // emit message too long
    return nsent;
  };
}
bool io_transport_kcp::do_write(highp_time_t& wait_duration)
{
  bool ret = io_transport_udp::do_write(wait_duration);

  const auto current = static_cast<IUINT32>(std::chrono::duration_cast<std::chrono::milliseconds>(get_service().current_time_.time_since_epoch()).count());
  if (((IINT32)(current - expire_time_)) >= 0)
  {
    ::ikcp_update(kcp_, current);
    expire_time_ = ::ikcp_check(kcp_, current);
  }

  return ret;
}
int io_transport_kcp::do_read(int revent, int& error, highp_time_t& wait_duration)
{
  int n = this->call_read(&rawbuf_.front(), static_cast<int>(rawbuf_.size()), revent, error);
  if (n > 0)
    this->handle_input(rawbuf_.data(), n, error, wait_duration);
  if (!error)
  { // !important, should always try to call ikcp_recv when no error occured.
    n = ::ikcp_recv(kcp_, buffer_ + offset_, sizeof(buffer_) - offset_);
    if (n > 0) // If got data from kcp, don't wait
      wait_duration = 0;
    else if (n < 0)
      n = 0; // EAGAIN/EWOULDBLOCK
  }
  return n;
}
int io_transport_kcp::handle_input(char* buf, int len, int& error, highp_time_t& wait_duration)
{
  // ikcp in event always in service thread, so no need to lock
  if (0 == ::ikcp_input(kcp_, buf, len))
  {
    expire_time_ = 0;
    return len;
  }
  // simply regards -1,-2,-3 as error and trigger connection lost event.
  error = yasio::errc::invalid_packet;
  return -1;
}
#endif
// ------------------------ io_service ------------------------
void io_service::init_globals(const yasio::inet::print_fn2_t& prt) { yasio__shared_globals(prt).cprint_ = prt; }
void io_service::cleanup_globals() { yasio__shared_globals().cprint_ = nullptr; }
unsigned int io_service::tcp_rtt(transport_handle_t transport) { return transport->is_open() ? transport->socket_->tcp_rtt() : 0; }
io_service::io_service() { this->initialize(nullptr, 1); }
io_service::io_service(int channel_count) { this->initialize(nullptr, channel_count); }
io_service::io_service(const io_hostent& channel_ep) { this->initialize(&channel_ep, 1); }
io_service::io_service(const std::vector<io_hostent>& channel_eps)
{
  this->initialize(!channel_eps.empty() ? channel_eps.data() : nullptr, static_cast<int>(channel_eps.size()));
}
io_service::io_service(const io_hostent* channel_eps, int channel_count) { this->initialize(channel_eps, channel_count); }
io_service::~io_service()
{
  this->do_stop(YSTF_FINALIZE);
  this->finalize();
}
void io_service::start(event_cb_t cb)
{
  if (state_ == io_service::state::IDLE)
  {
    auto& global_state = yasio__shared_globals();
    if (!this->options_.print_)
      this->options_.print_ = global_state.cprint_;

    if (cb)
      options_.on_event_ = std::move(cb);
    this->state_ = io_service::state::RUNNING;
    if (!options_.no_new_thread_)
    {
      this->worker_    = std::thread(&io_service::run, this);
      this->worker_id_ = worker_.get_id();
    }
    else
    {
      this->worker_id_ = std::this_thread::get_id();
      run();
      handle_stop();
    }
  }
}
void io_service::stop() { do_stop(YSTF_STOP); }
void io_service::do_stop(uint8_t flags)
{
  if (this->state_ <= io_service::state::IDLE)
    return;
  if (!this->stop_flag_)
  {
    this->stop_flag_ = flags;
    if (this->state_ == io_service::state::RUNNING)
    {
      for (auto c : channels_)
        this->close(c->index());
      this->wakeup();
      this->handle_stop();
    }
  }
  else
    this->handle_stop();
}
void io_service::handle_stop()
{
  if (this->worker_.joinable())
  {
    if (std::this_thread::get_id() == this->worker_id_)
    {
      if (yasio__testbits(this->stop_flag_, YSTF_FINALIZE))
        std::terminate(); // we don't want, but...
      xxsocket::set_last_errno(EAGAIN);
      return;
    }
    this->worker_.join();
  }

  if (this->state_ != state::AT_EXITING)
    return;

  if (this->options_.deferred_event_ && !this->events_.empty())
    this->dispatch((std::numeric_limits<int>::max)());
  clear_transports();
  this->timer_queue_.clear();
  this->stop_flag_ = 0;
  this->worker_id_ = std::thread::id{};
  this->state_     = io_service::state::IDLE;
}
void io_service::initialize(const io_hostent* channel_eps, int channel_count)
{
#if defined(YASIO_SSL_BACKEND)
  ssl_roles_[YSSL_CLIENT] = ssl_roles_[YSSL_SERVER] = nullptr;
#endif

  this->wait_duration_ = this->sched_freq_;

  // at least one channel
  if (channel_count < 1)
    channel_count = 1;

  options_.resolv_ = [this](std::vector<ip::endpoint>& eps, const char* host, unsigned short port) { return this->resolve(eps, host, port); };

  // create channels
  create_channels(channel_eps, channel_count);

#if !defined(YASIO_USE_CARES)
  life_mutex_ = std::make_shared<cxx17::shared_mutex>();
  life_token_ = std::make_shared<life_token>();
#endif
  this->state_ = io_service::state::IDLE;
}
void io_service::finalize()
{
  if (this->state_ == io_service::state::IDLE)
  {
#if !defined(YASIO_USE_CARES)
    std::unique_lock<cxx17::shared_mutex> lck(*life_mutex_);
    life_token_.reset();
#endif
    destroy_channels();

    options_.on_event_ = nullptr;
    options_.resolv_   = nullptr;

    for (auto o : tpool_)
      ::operator delete(o);
    tpool_.clear();

    this->state_ = io_service::state::UNINITIALIZED;
  }
}
void io_service::create_channels(const io_hostent* channel_eps, int channel_count)
{
  for (auto i = 0; i < channel_count; ++i)
  {
    auto channel = new io_channel(*this, i);
    if (channel_eps != nullptr)
      channel->set_address(channel_eps[i].host_, channel_eps[i].port_);
    channels_.push_back(channel);
  }
}
void io_service::destroy_channels()
{
  this->channel_ops_.clear();
  for (auto channel : channels_)
  {
    channel->timer_.cancel();
    cleanup_io(channel);
    delete channel;
  }
  channels_.clear();
}
void io_service::clear_transports()
{
  transport_map_.clear();
  for (auto transport : transports_)
  {
    cleanup_io(transport);
    yasio::invoke_dtor(transport);
    this->tpool_.push_back(transport);
  }
  transports_.clear();
}
size_t io_service::dispatch(int max_count)
{
  if (options_.on_event_)
    this->events_.consume(max_count, options_.on_event_);
  return this->events_.count();
}

#if defined(_WIN32)
template <typename _Ty>
struct minimal_optional {
  template <typename... _Args>
  void emplace(_Args&&... args)
  {
    new (&unintialized_memory_[0]) _Ty(std::forward<_Args>(args)...);
    has_value_ = true;
  }
  ~minimal_optional()
  {
    if (has_value_)
    {
      auto p = (_Ty*)&unintialized_memory_[0];
      p->~_Ty();
    }
  }
  uint8_t unintialized_memory_[sizeof(_Ty)];
  bool has_value_ = false;
};
#endif
void io_service::run()
{
  yasio::set_thread_name("yasio");

#if defined(_WIN32)
  minimal_optional<yasio::wtimer_hres> __timer_hres_man;
  if (options_.hres_timer_)
    __timer_hres_man.emplace();
#endif

#if defined(YASIO_SSL_BACKEND)
  init_ssl_context(YSSL_CLIENT); // by default, init ssl client context
#endif
#if defined(YASIO_USE_CARES)
  recreate_ares_channel();
  ares_socket_t ares_socks[ARES_GETSOCK_MAXNUM] = {0};
#endif

  do
  {
    this->current_time_ = yasio::steady_clock_t::now();

    auto waitd_usec = get_timeout(this->wait_duration_); // Gets current wait duration
#if defined(YASIO_USE_CARES)
    /**
     * retrieves the set of file descriptors which the calling application should poll io,
     * after poll_io, for ares invoke flow, refer to:
     * https://c-ares.org/ares_fds.html
     * https://c-ares.org/ares_timeout.html
     * https://c-ares.org/ares_process_fd.html
     */
    auto ares_nfds = ares_get_fds(ares_socks, waitd_usec);
#endif

    if (waitd_usec > 0)
    {
      YASIO_KLOGV("[core] poll_io max_nfds=%d, waiting... %.3f milliseconds", io_watcher_.max_descriptor(), waitd_usec / static_cast<float>(std::milli::den));
      int retval = io_watcher_.poll_io(waitd_usec);
      YASIO_KLOGV("[core] poll_io waked up, retval=%d", retval);
      if (retval < 0)
      {
        int ec = xxsocket::get_last_errno();
        YASIO_KLOGI("[core] poll_io failed, max_fd=%d ec=%d, detail:%s\n", io_watcher_.max_descriptor(), ec, io_service::strerror(ec));
        if (ec != EBADF)
          continue; // Try again.
        break;
      }
    }

#if defined(YASIO_USE_CARES)
    // process events for name resolution.
    do_ares_process_fds(ares_socks, ares_nfds);
#endif

    // process active transports
    process_transports();

    // process active channels
    process_channels();

    // process timeout timers
    process_timers();

    // process deferred events if auto dispatch enabled
    process_deferred_events();

  } while (!this->stop_flag_ || !this->transports_.empty());

#if defined(YASIO_USE_CARES)
  destroy_ares_channel();
#endif
#if defined(YASIO_SSL_BACKEND)
  cleanup_ssl_context(YSSL_CLIENT);
  cleanup_ssl_context(YSSL_SERVER);
#endif

  this->state_ = io_service::state::AT_EXITING;
}
void io_service::process_transports()
{
  // preform transports
  for (auto iter = transports_.begin(); iter != transports_.end();)
  {
    auto transport = *iter;
    bool ok        = (do_read(transport) && do_write(transport));
    if (ok)
    {
      int opm = transport->opmask_ | transport->ctx_->opmask_ | this->stop_flag_;
      if (0 == opm)
      { // no open/close/stop operations request
        ++iter;
        continue;
      }
      if (transport->error_ == 0)
        transport->error_ = yasio::errc::shutdown_by_localhost;
    }

    handle_close(transport);
    iter = transports_.erase(iter);
  }
}
void io_service::process_channels()
{
  if (!this->channel_ops_.empty())
  {
    // perform active channels
    std::lock_guard<std::recursive_mutex> lck(this->channel_ops_mtx_);
    for (auto iter = this->channel_ops_.begin(); iter != this->channel_ops_.end();)
    {
      auto ctx    = *iter;
      bool finish = true;
      if (yasio__testbits(ctx->properties_, YCM_CLIENT))
      {
        if (yasio__testbits(ctx->opmask_, YOPM_OPEN))
        {
          yasio__clearbits(ctx->opmask_, YOPM_OPEN);
          ctx->state_ = io_base::state::RESOLVING;
        }

        if (ctx->state_ == io_base::state::RESOLVING)
        {
          if (do_resolve(ctx) == 0)
            do_connect(ctx);
          else if (ctx->error_ != EINPROGRESS)
            handle_connect_failed(ctx, ctx->error_);
        }
        else if (ctx->state_ == io_base::state::CONNECTING)
          do_connect_completion(ctx);
        finish = ctx->error_ != EINPROGRESS;
      }
      else if (yasio__testbits(ctx->properties_, YCM_SERVER))
      {
        auto opmask = ctx->opmask_;
        if (yasio__testbits(opmask, YOPM_OPEN))
          do_accept(ctx);
        else if (yasio__testbits(opmask, YOPM_CLOSE))
          cleanup_channel(ctx);

        finish = (ctx->state_ != io_base::state::OPENED);
        if (!finish)
          do_accept_completion(ctx);
        else
          ctx->bytes_transferred_ = 0;
      }

      if (finish)
        iter = this->channel_ops_.erase(iter);
      else
        ++iter;
    }
  }
}
void io_service::close(int index)
{
  // Gets channel context
  auto channel = channel_at(index);
  if (!channel)
    return;

  if (!yasio__testbits(channel->opmask_, YOPM_CLOSE))
  {
    yasio__clearbits(channel->opmask_, YOPM_OPEN);
    if (channel->socket_->is_open())
    {
      yasio__setbits(channel->opmask_, YOPM_CLOSE);
      this->wakeup();
    }
  }
}
void io_service::close(transport_handle_t transport)
{
  if (!yasio__testbits(transport->opmask_, YOPM_CLOSE))
  {
    yasio__setbits(transport->opmask_, YOPM_CLOSE);
    this->wakeup();
  }
}
bool io_service::is_open(transport_handle_t transport) const { return transport->is_open(); }
bool io_service::is_open(int index) const
{
  auto ctx = channel_at(index);
  return ctx != nullptr && ctx->state_ == io_base::state::OPENED;
}
bool io_service::open(size_t index, int kind)
{
  assert((kind > 0 && kind <= 0xff) && ((kind & (kind - 1)) != 0));
  auto ctx = channel_at(index);
  if (ctx != nullptr)
  {
    yasio__setlobyte(ctx->properties_, kind & 0xff);
    if (yasio__testbits(kind, YCM_TCP))
      ctx->socktype_ = SOCK_STREAM;
    else if (yasio__testbits(kind, YCM_UDP))
      ctx->socktype_ = SOCK_DGRAM;
    return open_internal(ctx);
  }
  return false;
}
io_channel* io_service::channel_at(size_t index) const { return (index < channels_.size()) ? channels_[index] : nullptr; }
void io_service::handle_close(transport_handle_t thandle)
{
  auto ctx          = thandle->ctx_;
  auto error        = thandle->error_;
  const bool client = yasio__testbits(ctx->properties_, YCM_CLIENT);

  if (yasio__testbits(ctx->properties_, YCM_UDP))
    transport_map_.erase(thandle->remote_endpoint());

  if (yasio__testbits(ctx->properties_, YCM_KCP))
  {
    if (--nsched_ <= 0) // if no sched transport, reset sched_freq to max wait 5mins
      sched_freq_ = yasio__max_wait_usec;
  }
  if (thandle->state_ == io_base::state::OPENED)
  { // @Because we can't retrive peer endpoint when connect reset by peer, so use id to trace.
    YASIO_KLOGD("[index: %d] the connection #%u is lost, ec=%d, where=%d, detail:%s", ctx->index_, thandle->id_, error, (int)thandle->error_stage_,
                io_service::strerror(error));
    this->fire_event(ctx->index(), YEK_ON_CLOSE, error, thandle);
  }
#if defined(YASIO_SSL_BACKEND)
  if (yasio__testbits(ctx->properties_, YCM_SSL))
    static_cast<io_transport_ssl*>(thandle)->do_ssl_shutdown();
#endif
  if (yasio__testbits(ctx->properties_, YCM_TCP) && error == yasio::errc::shutdown_by_localhost)
    thandle->socket_->shutdown();
  cleanup_io(thandle);
  deallocate_transport(thandle);
  if (client)
  {
    yasio__clearbits(ctx->opmask_, YOPM_CLOSE);
    cleanup_channel(ctx, false);
  }
}
int io_service::write(transport_handle_t transport, sbyte_buffer buffer, completion_cb_t handler)
{
  if (transport && transport->is_open())
    return !buffer.empty() ? transport->write(io_send_buffer{std::move(buffer)}, std::move(handler)) : 0;
  else
  {
    YASIO_KLOGE("write failed, the connection not ok!");
    return -1;
  }
}
int io_service::forward(transport_handle_t transport, const void* buf, size_t len, completion_cb_t handler)
{
  if (transport && transport->is_open())
    return len != 0 ? transport->write(io_send_buffer{(const char*)buf, len}, std::move(handler)) : 0;
  else
  {
    YASIO_KLOGE("write failed, the connection not ok!");
    return -1;
  }
}
int io_service::write_to(transport_handle_t transport, sbyte_buffer buffer, const ip::endpoint& to, completion_cb_t handler)
{
  if (transport && transport->is_open())
    return !buffer.empty() ? transport->write_to(io_send_buffer{std::move(buffer)}, to, std::move(handler)) : 0;
  else
  {
    YASIO_KLOGE("write_to failed, the connection not ok!");
    return -1;
  }
}
int io_service::forward_to(transport_handle_t transport, const void* buf, size_t len, const ip::endpoint& to, completion_cb_t handler)
{
  if (transport && transport->is_open())
    return len != 0 ? transport->write_to(io_send_buffer{(const char*)buf, len}, to, std::move(handler)) : 0;
  else
  {
    YASIO_KLOGE("write_to failed, the connection not ok!");
    return -1;
  }
}
void io_service::do_connect(io_channel* ctx)
{
  assert(!ctx->remote_eps_.empty());
  if (ctx->socket_->is_open())
    cleanup_io(ctx);

  ctx->state_ = io_base::state::CONNECTING;
  auto& ep    = ctx->remote_eps_[0];
  YASIO_KLOGD("[index: %d] connecting server %s(%s):%u...", ctx->index_, ctx->remote_host_.c_str(), ep.ip().c_str(), ctx->remote_port_);
  if (ctx->socket_->popen(ep.af(), ctx->socktype_))
  {
    int ret = 0;
    if (yasio__testbits(ctx->properties_, YCF_REUSEADDR))
      ctx->socket_->reuse_address(true);
    if (yasio__testbits(ctx->properties_, YCF_EXCLUSIVEADDRUSE))
      ctx->socket_->exclusive_address(true);

    if (!yasio__testbits(ctx->properties_, YCM_UDS) && (!ctx->local_host_.empty() || ctx->local_port_ != 0 || yasio__testbits(ctx->properties_, YCM_UDP)))
    { // Don't invoke socket.bind for tcp when not require bind network inteface or port explicitly
      auto ifaddr = ctx->local_host_.empty() ? YASIO_ADDR_ANY(ep.af()) : ctx->local_host_.c_str();
      ret         = ctx->socket_->bind(ifaddr, ctx->local_port_);
      if (ret < 0 && xxsocket::get_last_errno() == EPERM)
      {
        /*
        Supress EPERM: on macos the bind will always fail with EPERM when runs in code signing sandbox mode
        even through provide ADDR_ANY and port=0 to require bind a random local endpoint, but on other systems,
        require call bind before connect for UDP sockets.
        refer issue: https://github.com/yasio/yasio/issues/441
        */
        YASIO_KLOGW("[warning] bind %s:%u fail", ifaddr, ctx->local_port_);
        ret = 0;
      }
    }

    if (ret == 0)
    {
      // tcp connect directly, for udp do not need to connect.
      if (yasio__testbits(ctx->properties_, YCM_TCP))
        ret = xxsocket::connect(ctx->socket_->native_handle(), ep);
      // join the multicast group for udp
      if (yasio__testbits(ctx->properties_, YCPF_MCAST))
        ctx->join_multicast_group();
    }

    if (ret < 0)
    { // setup non-blocking connect
      int error = xxsocket::get_last_errno();
      if (error != EINPROGRESS && error != EWOULDBLOCK)
        this->handle_connect_failed(ctx, error);
      else
      {
        ctx->set_last_errno(EINPROGRESS);
        io_watcher_.mod_event(ctx->socket_->native_handle(), socket_event::readwrite, 0);
        ctx->timer_.expires_from_now(std::chrono::microseconds(options_.connect_timeout_));
        ctx->timer_.async_wait_once([ctx](io_service& thiz) {
          if (ctx->state_ != io_base::state::OPENED)
            thiz.handle_connect_failed(ctx, ETIMEDOUT);
        });
      }
    }
    else if (ret == 0)
    { // connect server successful immediately.
      io_watcher_.mod_event(ctx->socket_->native_handle(), socket_event::read, 0);
      handle_connect_succeed(ctx, ctx->socket_);
    } // !!!NEVER GO HERE
  }
  else
    this->handle_connect_failed(ctx, xxsocket::get_last_errno());
}

void io_service::do_connect_completion(io_channel* ctx)
{
  assert(ctx->state_ == io_base::state::CONNECTING);
  if (ctx->state_ == io_base::state::CONNECTING)
  {
    if (io_watcher_.is_ready(ctx->socket_->native_handle(), socket_event::readwrite))
    {
      int error = -1;
      if (ctx->socket_->get_optval(SOL_SOCKET, SO_ERROR, error) >= 0 && error == 0)
      {
        // The nonblocking tcp handshake complete, remove write event avoid high-CPU occupation
        io_watcher_.mod_event(ctx->socket_->native_handle(), 0, socket_event::write);
        handle_connect_succeed(ctx, ctx->socket_);
      }
      else
        handle_connect_failed(ctx, error);
      ctx->timer_.cancel();
    }
  }
}
#if defined(YASIO_SSL_BACKEND)
yssl_ctx_st* io_service::init_ssl_context(ssl_role role)
{
  auto ctx         = role == YSSL_CLIENT ? yssl_ctx_new(yssl_options{yasio__c_str(options_.cafile_), nullptr, true})
                                         : yssl_ctx_new(yssl_options{yasio__c_str(options_.crtfile_), yasio__c_str(options_.keyfile_), false});
  ssl_roles_[role] = ctx;
  return ctx;
}
void io_service::cleanup_ssl_context(ssl_role role)
{
  auto& ctx = ssl_roles_[role];
  if (ctx)
    yssl_ctx_free(ctx);
}
#endif
#if defined(YASIO_USE_CARES)
void io_service::ares_work_started() { ++ares_outstanding_work_; }
void io_service::ares_work_finished()
{
  if (ares_outstanding_work_ > 0)
    --ares_outstanding_work_;
}
void io_service::ares_sock_state_cb(void* data, socket_native_type socket_fd, int readable, int writable)
{
  auto service = (io_service*)data;
  int events   = socket_event::null;
  if (readable)
    events |= socket_event::read;
  if (writable)
    events |= socket_event::write;
  if (events != 0)
    service->io_watcher_.mod_event(socket_fd, events, 0);
  else
    service->io_watcher_.mod_event(socket_fd, 0, socket_event::readwrite);
}
void io_service::ares_getaddrinfo_cb(void* data, int status, int /*timeouts*/, ares_addrinfo* answerlist)
{
  auto ctx              = (io_channel*)data;
  auto& current_service = ctx->get_service();
  current_service.ares_work_finished();
  if (status == ARES_SUCCESS && answerlist != nullptr)
  {
    for (auto ai = answerlist->nodes; ai != nullptr; ai = ai->ai_next)
    {
      if (ai->ai_family == AF_INET6 || ai->ai_family == AF_INET)
        ctx->remote_eps_.push_back(ip::endpoint(ai->ai_addr));
    }
  }

  auto __get_cprint = [&]() -> const print_fn2_t& { return current_service.options_.print_; };
  if (!ctx->remote_eps_.empty())
  {
    ctx->query_success_time_ = highp_clock();
#  if defined(YASIO_ENABLE_ARES_PROFILER)
    YASIO_KLOGD("[index: %d] ares_getaddrinfo_cb: query %s succeed, cost:%g(ms)", ctx->index_, ctx->remote_host_.c_str(),
                (ctx->query_success_time_ - ctx->query_start_time_) / 1000.0);
#  endif
  }
  else
  {
    ctx->set_last_errno(yasio::errc::resolve_host_failed);
    YASIO_KLOGE("[index: %d] ares_getaddrinfo_cb: query %s failed, status=%d, detail:%s", ctx->index_, ctx->remote_host_.c_str(), status,
                ::ares_strerror(status));
  }
  current_service.wakeup();
}
int io_service::ares_get_fds(socket_native_type* socks, highp_time_t& waitd_usec)
{
  int nfds = 0;
  if (ares_outstanding_work_)
  {
    int bitmask = ::ares_getsock(this->ares_, socks, ARES_GETSOCK_MAXNUM);
    for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i)
    {
      int events = socket_event::null;
      if (ARES_GETSOCK_READABLE(bitmask, i))
        events |= socket_event::read;
      if (ARES_GETSOCK_WRITABLE(bitmask, i))
        events |= socket_event::write;
      if (events)
        ++nfds;
      else
        break;
    }

    if (nfds)
    {
      timeval waitd_tv = {(decltype(timeval::tv_sec))(waitd_usec / std::micro::den), (decltype(timeval::tv_usec))(waitd_usec % std::micro::den)};
      ::ares_timeout(this->ares_, &waitd_tv, &waitd_tv);
      waitd_usec = waitd_tv.tv_sec * std::micro::den + waitd_tv.tv_usec;
    }
  }
  return nfds;
}
void io_service::do_ares_process_fds(socket_native_type* socks, int nfds)
{
  for (auto i = 0; i < nfds; ++i)
  {
    auto fd = socks[i];
    ::ares_process_fd(this->ares_, io_watcher_.is_ready(fd, socket_event::read) ? fd : ARES_SOCKET_BAD,
                      io_watcher_.is_ready(fd, socket_event::write) ? fd : ARES_SOCKET_BAD);
  }
}
void io_service::recreate_ares_channel()
{
  if (ares_)
    destroy_ares_channel();

  int optmask                = ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES | ARES_OPT_SOCK_STATE_CB /* | ARES_OPT_LOOKUPS*/;
  ares_options options       = {};
  options.timeout            = static_cast<int>(this->options_.dns_queries_timeout_ / std::milli::den);
  options.tries              = this->options_.dns_queries_tries_;
  options.sock_state_cb      = io_service::ares_sock_state_cb;
  options.sock_state_cb_data = this;
#  if defined(__linux__) && !defined(__ANDROID__)
  if (yasio::is_regular_file(YASIO_SYSTEMD_RESOLV_PATH))
  {
    options.resolvconf_path = strndup(YASIO_SYSTEMD_RESOLV_PATH, YASIO_SYSTEMD_RESOLV_PATH_LEN);
    optmask |= ARES_OPT_RESOLVCONF;
  }
#  endif
  int status = ::ares_init_options(&ares_, &options, optmask);
  if (status == ARES_SUCCESS)
  {
    YASIO_KLOGD("[c-ares] create channel succeed");
    config_ares_name_servers();
  }
  else
    YASIO_KLOGE("[c-ares] create channel failed, status=%d, detail:%s", status, ::ares_strerror(status));
}
void io_service::config_ares_name_servers()
{
  std::string nscsv;
  // list all dns servers for resov problem diagnosis
  ares_addr_port_node* name_servers = nullptr;
  const char* what                  = "system";
  if (!options_.name_servers_.empty())
  {
    ::ares_set_servers_ports_csv(ares_, options_.name_servers_.c_str());
    what = "custom";
  }
  int status = ::ares_get_servers_ports(ares_, &name_servers);
  if (status == ARES_SUCCESS)
  {
    for (auto ns = name_servers; ns != nullptr; ns = ns->next)
      if (endpoint{ns->family, &ns->addr, static_cast<u_short>(ns->udp_port)}.format_to(nscsv, endpoint::fmt_default | endpoint::fmt_no_local))
        nscsv.push_back(',');
    if (!nscsv.empty()) // if no valid name server, use predefined fallback dns
      YASIO_KLOGI("[c-ares] use %s dns: %s", what, nscsv.c_str());
    else
    {
      status = ::ares_set_servers_csv(ares_, YASIO_FALLBACK_NAME_SERVERS);
      if (status == 0)
        YASIO_KLOGW("[c-ares] set fallback dns: '%s' succeed", YASIO_FALLBACK_NAME_SERVERS);
      else
        YASIO_KLOGE("[c-ares] set fallback dns: '%s' failed, detail: %s", YASIO_FALLBACK_NAME_SERVERS, ::ares_strerror(status));
    }
    ::ares_free_data(name_servers);
  }
}
void io_service::destroy_ares_channel()
{
  if (ares_ != nullptr)
  {
    ::ares_cancel(this->ares_);
    ::ares_destroy(this->ares_);
    this->ares_ = nullptr;
  }
}
#endif
void io_service::do_accept(io_channel* ctx)
{ // channel is server
  cleanup_channel(ctx);

  ip::endpoint ep;
  if (!yasio__testbits(ctx->properties_, YCM_UDS))
  {
    // server: don't need resolve, don't use remote_eps_
    auto ifaddr = ctx->remote_host_.empty() ? YASIO_ADDR_ANY(local_address_family()) : ctx->remote_host_.c_str();
    ep.as_in(ifaddr, ctx->remote_port_);
  }
#if defined(YASIO_ENABLE_UDS) && YASIO__HAS_UDS
  else
  {
    ep.as_un(ctx->remote_host_.c_str());
    ::unlink(ctx->remote_host_.c_str());
  }
#endif
  int error                  = -1;
  io_base::error_stage where = io_base::error_stage::NONE;
  do
  {
    xxsocket::set_last_errno(0);
    if (!ctx->socket_->popen(ep.af(), ctx->socktype_))
    {
      where = io_base::error_stage::OPEN_SOCKET;
      break;
    }

    if (yasio__testbits(ctx->properties_, YCF_REUSEADDR))
      ctx->socket_->reuse_address(true);
    if (yasio__testbits(ctx->properties_, YCF_EXCLUSIVEADDRUSE))
      ctx->socket_->exclusive_address(false);
    if (ctx->socket_->bind(ep) != 0)
    {
      where = io_base::error_stage::BIND_SOCKET;
      break;
    }

    if (yasio__testbits(ctx->properties_, YCM_TCP) && ctx->socket_->listen(YASIO_SOMAXCONN) != 0)
    {
      where = io_base::error_stage::LISTEN_SOCKET;
      break;
    }

    ctx->state_ = io_base::state::OPENED;
    if (yasio__testbits(ctx->properties_, YCM_UDP))
    {
      if (yasio__testbits(ctx->properties_, YCPF_MCAST))
        ctx->join_multicast_group();
      ctx->buffer_.resize(yasio__max_rcvbuf);
    }
    io_watcher_.mod_event(ctx->socket_->native_handle(), socket_event::read, 0);
    YASIO_KLOGI("[index: %d] open server succeed, socket.fd=%d listening at %s...", ctx->index_, (int)ctx->socket_->native_handle(), ep.to_string().c_str());
    error = 0;
  } while (false);

  if (error < 0)
  {
    error = xxsocket::get_last_errno();
    YASIO_KLOGE("[index: %d] open server failed during stage %d, ec=%d, detail:%s", where, ctx->index_, error, io_service::strerror(error));
    ctx->socket_->close();
    ctx->state_ = io_base::state::CLOSED;
  }
#if defined(YASIO_ENABLE_PASSIVE_EVENT)
  this->fire_event(ctx->index_, YEK_ON_OPEN, error, ctx, 1);
#endif
}
void io_service::do_accept_completion(io_channel* ctx)
{
  if (ctx->state_ == io_base::state::OPENED)
  {
    int error = 0;
    if (io_watcher_.is_ready(ctx->socket_->native_handle(), socket_event::read) && ctx->socket_->get_optval(SOL_SOCKET, SO_ERROR, error) >= 0 && error == 0)
    {
      if (yasio__testbits(ctx->properties_, YCM_TCP))
      {
        socket_native_type sockfd{invalid_socket};
        error = ctx->socket_->paccept(sockfd);
        if (error == 0)
          handle_connect_succeed(ctx, std::make_shared<xxsocket>(sockfd));
        else // The non-blocking tcp accept failed can be ignored.
          YASIO_KLOGV("[index: %d] socket.fd=%d, accept failed, ec=%u", ctx->index_, (int)ctx->socket_->native_handle(), error);
      }
      else // YCM_UDP
      {
        ip::endpoint peer;
        int n = ctx->socket_->recvfrom(&ctx->buffer_.front(), static_cast<int>(ctx->buffer_.size()), peer);
        if (n > 0)
        {
          YASIO_KLOGV("[index: %d] recvfrom peer: %s succeed.", ctx->index_, peer.to_string().c_str());
          auto transport = static_cast<io_transport_udp*>(do_dgram_accept(ctx, peer, error));
          if (transport)
          {
            if (transport->handle_input(ctx->buffer_.data(), n, error, this->wait_duration_) < 0)
            {
              transport->error_ = error;
              close(transport);
            }
          }
          else
            YASIO_KLOGE("[index: %d] do_dgram_accept failed, ec=%d, detail:%s", ctx->index_, error, this->strerror(error));
        }
        else if (n < 0)
        {
          error = xxsocket::get_last_errno();
          if (!xxsocket::not_recv_error(error))
            YASIO_KLOGE("[index: %d] recvfrom failed, ec=%d, detail:%s", ctx->index_, error, this->strerror(error));
        }
      }
    }
  }
}
int io_service::local_address_family() const
{
  if (!yasio__testbits(ipsv_, ipsv_ipv4))
    ipsv_ = static_cast<u_short>(xxsocket::getipsv());
  return ((ipsv_ & ipsv_ipv4) || !ipsv_) ? AF_INET : AF_INET6;
}
transport_handle_t io_service::do_dgram_accept(io_channel* ctx, const ip::endpoint& peer, int& error)
{
  /*
    Because Bind() the client socket to the socket address of the listening socket.  On
    Linux this essentially passes the responsibility for receiving data for the client
    session from the well-known listening socket, to the newly allocated client socket.  It
    is important to note that this behavior is not the same on other platforms, like
    Windows (unfortunately), detail see:
    https://blog.grijjy.com/2018/08/29/creating-high-performance-udp-servers-on-windows-and-linux
    https://cloud.tencent.com/developer/article/1004555
    So we emulate thus by ourself, don't care the performance, just a workaround implementation.

    Notes:
      a. for win32: we check exists udp clients by ourself, and only write operation can be
         perform on transports, the read event still routed by channel.
      b. for non-win32 multicast: same with win32, because the kernel can't route same udp peer as 1
         transport when the peer always sendto multicast address.
  */
  // both win32 and unix(like) should check does remote endpoint already assoc with a transport
  auto it = this->transport_map_.find(peer);
  if (it != this->transport_map_.end())
    return it->second;

  auto new_sock = std::make_shared<xxsocket>();
  if (new_sock->popen(peer.af(), SOCK_DGRAM))
  {
    if (yasio__testbits(ctx->properties_, YCF_REUSEADDR))
      new_sock->reuse_address(true);
    if (yasio__testbits(ctx->properties_, YCF_EXCLUSIVEADDRUSE))
      new_sock->exclusive_address(false);
    if (new_sock->bind(YASIO_ADDR_ANY(peer.af()), ctx->remote_port_) == 0)
    {
      auto transport = static_cast<io_transport_udp*>(allocate_transport(ctx, std::move(new_sock)));
      // We always establish 4 tuple with clients
      transport->confgure_remote(peer);
      const bool user_route = !YASIO__UDP_KROUTE || yasio__testbits(ctx->properties_, YCPF_MCAST);
      if (user_route)
        active_transport(transport);
      else
        handle_connect_succeed(transport);

      this->transport_map_.emplace(peer, transport);
      return transport;
    }
  }

  // unhandled, get error from system.
  error = xxsocket::get_last_errno();
  return nullptr;
}
void io_service::handle_connect_succeed(transport_handle_t transport)
{
  auto ctx = transport->ctx_;
  ctx->clear_mutable_flags();
  ctx->error_      = 0; // clear errno, value may be EINPROGRESS
  auto& connection = transport->socket_;
  if (yasio__testbits(ctx->properties_, YCM_CLIENT))
  {
    // Reset client channel bytes transferred when a new connection established
    ctx->bytes_transferred_ = 0;
    ctx->state_             = io_base::state::OPENED;
    if (yasio__testbits(ctx->properties_, YCM_UDP))
      static_cast<io_transport_udp*>(transport)->confgure_remote(ctx->remote_eps_[0]);
  }
  else
    io_watcher_.mod_event(connection->native_handle(), socket_event::read, 0);
  if (yasio__testbits(ctx->properties_, YCM_TCP))
  {
    // apply tcp keepalive options
    if (options_.tcp_keepalive_.onoff)
      connection->set_keepalive(options_.tcp_keepalive_.onoff, options_.tcp_keepalive_.idle, options_.tcp_keepalive_.interval, options_.tcp_keepalive_.probs);
  }
#if !defined(_WIN32) // windows: UDP will ignore sndbuf, other: ensure sndbuf >= max_ip_mtu(65535)
  if (yasio__testbits(ctx->properties_, YCM_UDP))
  {
    constexpr int max_ip_mtu = static_cast<int>((std::numeric_limits<uint16_t>::max)());
    transport->socket_->set_optval(SOL_SOCKET, SO_SNDBUF, max_ip_mtu + 1);
  }
#endif

  active_transport(transport);
}
void io_service::active_transport(transport_handle_t t)
{
  auto ctx = t->ctx_;
  auto& s  = t->socket_;
  this->transports_.push_back(t);
  if (yasio__testbits(ctx->properties_, YCM_KCP))
  {
    ++this->nsched_;
#if defined(YASIO_ENABLE_KCP)
    auto interval = static_cast<io_transport_kcp*>(t)->interval();
    if (this->sched_freq_ > interval)
      this->sched_freq_ = interval;
#endif
  }
  if (!yasio__testbits(ctx->properties_, YCM_SSL))
  {
    YASIO__UNUSED_PARAM(s);
    YASIO_KLOGV("[index: %d] sndbuf=%d, rcvbuf=%d", ctx->index_, s->get_optval<int>(SOL_SOCKET, SO_SNDBUF), s->get_optval<int>(SOL_SOCKET, SO_RCVBUF));
    YASIO_KLOGD("[index: %d] the connection #%u <%s> --> <%s> is established.", ctx->index_, t->id_, t->local_endpoint().to_string().c_str(),
                t->remote_endpoint().to_string().c_str());
    this->fire_event(ctx->index_, YEK_ON_OPEN, 0, t);
  }
  else if (yasio__testbits(ctx->properties_, YCM_CLIENT))
    this->wakeup();
}
transport_handle_t io_service::allocate_transport(io_channel* ctx, xxsocket_ptr&& s)
{
  transport_handle_t transport;
  void* vp;
  if (!tpool_.empty())
  { // allocate from pool
    vp = tpool_.back();
    tpool_.pop_back();
  }
  else
    vp = ::operator new(yasio__max_tsize);
  do
  {
    if (yasio__testbits(ctx->properties_, YCM_TCP))
    { // tcp like transport
#if defined(YASIO_SSL_BACKEND)
      if (yasio__testbits(ctx->properties_, YCM_SSL))
      {
        transport = new (vp) io_transport_ssl(ctx, std::forward<xxsocket_ptr>(s));
        break;
      }
#endif
      transport = new (vp) io_transport_tcp(ctx, std::forward<xxsocket_ptr>(s));
    }
    else // udp like transport
    {
#if defined(YASIO_ENABLE_KCP)
      if (yasio__testbits(ctx->properties_, YCM_KCP))
      {
        transport = new (vp) io_transport_kcp(ctx, std::forward<xxsocket_ptr>(s));
        break;
      }
#endif
      transport = new (vp) io_transport_udp(ctx, std::forward<xxsocket_ptr>(s));
    }
  } while (false);
  transport->set_primitives();
  return transport;
}
void io_service::deallocate_transport(transport_handle_t t)
{
  if (t->is_valid())
  {
    yasio::invoke_dtor(t);
    this->tpool_.push_back(t);
  }
}
void io_service::handle_connect_failed(io_channel* ctx, int error)
{
  cleanup_channel(ctx);
  YASIO_KLOGE("[index: %d] connect server %s failed, ec=%d, detail:%s", ctx->index_, ctx->format_destination().c_str(), error, io_service::strerror(error));
  fire_event(ctx->index_, YEK_ON_OPEN, error, ctx);
}
bool io_service::do_read(transport_handle_t transport)
{
  bool ret = false;
  do
  {
    if (!transport->socket_->is_open())
      break;
    int error  = 0;
    int revent = io_watcher_.is_ready(transport->socket_->native_handle(), socket_event::read | socket_event::error);
    int n      = transport->do_read(revent, error, this->wait_duration_);
    if (n >= 0)
    {
      if (!options_.forward_packet_)
      {
        YASIO_KLOGV("[index: %d] do_read status ok, bytes transferred: %d, buffer used: %d", transport->cindex(), n, n + transport->offset_);
        const int bytes_to_strip = transport->ctx_->uparams_.initial_bytes_to_strip;
        if (transport->expected_size_ == -1)
        { // decode length
          int length = transport->ctx_->decode_len_(transport->buffer_, transport->offset_ + n);
          if (length > 0)
          {
            if (length < bytes_to_strip)
            {
              transport->set_last_errno(yasio::errc::invalid_packet, yasio::io_base::error_stage::READ);
              break;
            }
            transport->expected_size_ = length;
            transport->expected_packet_.reserve((std::min)(length - bytes_to_strip,
                                                           YASIO_MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid memory reallocte.
            unpack(transport, transport->expected_size_, n, bytes_to_strip);
          }
          else if (length == 0) // header insufficient, wait readfd ready at next event frame.
            transport->offset_ += n;
          else
          {
            transport->set_last_errno(yasio::errc::invalid_packet, yasio::io_base::error_stage::READ);
            break;
          }
        }
        else // process incompleted pdu
          unpack(transport, transport->expected_size_ - static_cast<int>(transport->expected_packet_.size() + bytes_to_strip), n, 0);
      }
      else if (n > 0)
      { // forward packet, don't perform unpack, it's useful for implement streaming based protocol, like http, websocket and ...
        this->forward_packet(transport->cindex(), io_packet_view{transport->buffer_, n}, transport);
      }
    }
    else
    { // n < 0, regard as connection should close
      transport->set_last_errno(error, yasio::io_base::error_stage::READ);
      break;
    }
    ret = true;
  } while (false);
  return ret;
}
void io_service::unpack(transport_handle_t transport, int bytes_want /*want consume bytes from recv buffer per time*/, int bytes_transferred,
                        int bytes_to_strip)
{
  auto& offset         = transport->offset_;
  auto bytes_available = bytes_transferred + offset;
  auto& pkt            = transport->expected_packet_;
  pkt.insert(pkt.end(), transport->buffer_ + bytes_to_strip, transport->buffer_ + (std::min)(bytes_want, bytes_available));

  // set 'offset' to bytes of remain buffer
  offset = bytes_available - bytes_want;
  if (offset >= 0)
  { /* pdu received properly */
    if (offset > 0)
    { /* move remain data to head of buffer and hold 'offset'. */
      ::memmove(transport->buffer_, transport->buffer_ + bytes_want, offset);
      this->wait_duration_ = 0;
    }
    // move properly pdu to ready queue, the other thread who care about will retrieve it.
    YASIO_KLOGV("[index: %d] received a properly packet from peer, packet size:%d", transport->cindex(), transport->expected_size_);
    this->fire_event(transport->cindex(), transport->fetch_packet(), transport);
  }
  else /* all buffer consumed, set 'offset' to ZERO, pdu incomplete, continue recv remain data. */
    offset = 0;
}
highp_timer_ptr io_service::schedule(const std::chrono::microseconds& duration, timer_cb_t cb)
{
  auto timer = std::make_shared<highp_timer>(*this);
  timer->expires_from_now(duration);
  /*!important, hold on `timer` by lambda expression */
#if YASIO__HAS_CXX14
  timer->async_wait([timer, cb = std::move(cb)](io_service& service) { return cb(service); });
#else
  timer->async_wait([timer, cb](io_service& service) { return cb(service); });
#endif
  return timer;
}
void io_service::schedule_timer(highp_timer* timer_ctl, timer_cb_t&& timer_cb)
{
  if (timer_ctl == nullptr)
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  auto timer_it = this->find_timer(timer_ctl);
  if (timer_it == timer_queue_.end())
    this->timer_queue_.emplace_back(timer_ctl, std::move(timer_cb));
  else // always replace timer_cb
    timer_it->second = std::move(timer_cb);

  this->sort_timers();
  // If the timer is earliest, wakup
  if (timer_ctl == this->timer_queue_.back().first)
    this->wakeup();
}
void io_service::remove_timer(highp_timer* timer)
{
  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  auto iter = this->find_timer(timer);
  if (iter != timer_queue_.end())
  {
    timer_queue_.erase(iter);
    if (!timer_queue_.empty())
    {
      this->sort_timers();
      this->wakeup();
    }
  }
}
bool io_service::open_internal(io_channel* ctx)
{
  if (ctx->state_ == io_base::state::CONNECTING || ctx->state_ == io_base::state::RESOLVING)
  {
    YASIO_KLOGD("[index: %d] the channel open operation is in progress!", ctx->index_);
    return false;
  }

  yasio__clearbits(ctx->opmask_, YOPM_CLOSE);
  yasio__setbits(ctx->opmask_, YOPM_OPEN);

  ++ctx->connect_id_;

  this->channel_ops_mtx_.lock();
  if (yasio__find(this->channel_ops_, ctx) == this->channel_ops_.end())
    this->channel_ops_.push_back(ctx);
  this->channel_ops_mtx_.unlock();

  this->wakeup();
  return true;
}
bool io_service::close_internal(io_channel* ctx)
{
  yasio__clearbits(ctx->opmask_, YOPM_OPEN);
  if (ctx->socket_->is_open())
  {
    yasio__setbits(ctx->opmask_, YOPM_CLOSE);
    return true;
  }
  return false;
}
void io_service::process_timers()
{
  if (this->timer_queue_.empty())
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  unsigned int n = 0; // the count expired loop timers
  while (!this->timer_queue_.empty())
  {
    auto timer_ctl = timer_queue_.back().first;
    if (timer_ctl->expired())
    {
      // fetch timer
      auto timer_impl = std::move(timer_queue_.back());
      timer_queue_.pop_back();

      if (!timer_impl.second(*this))
      { // reschedule if the timer want wait again
        timer_ctl->expires_from_now();
        timer_queue_.push_back(std::move(timer_impl));
        ++n;
      }
    }
    else
      break;
  }
  if (n)
    this->sort_timers();
}
void io_service::process_deferred_events()
{
  if (!options_.no_dispatch_ && dispatch() > 0)
    this->wait_duration_ = 0;
}
highp_time_t io_service::get_timeout(highp_time_t usec)
{
  this->wait_duration_ = this->sched_freq_; // Reset next wait duration per frame

  if (this->timer_queue_.empty())
    return usec;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  if (!this->timer_queue_.empty())
  {
    // microseconds
    auto duration = timer_queue_.back().first->wait_duration();
    if (std::chrono::microseconds(usec) > duration)
      usec = duration.count();
  }
  return usec;
}
bool io_service::cleanup_channel(io_channel* ctx, bool clear_mask)
{
  ctx->clear_mutable_flags();
  bool bret = cleanup_io(ctx, clear_mask);
#if defined(YASIO_ENABLE_PASSIVE_EVENT)
  if (bret && yasio__testbits(ctx->properties_, YCM_SERVER))
    this->fire_event(ctx->index_, YEK_ON_CLOSE, 0, ctx, 1);
#endif
  return bret;
}
bool io_service::cleanup_io(io_base* obj, bool clear_mask)
{
  obj->error_ = 0;
  obj->state_ = io_base::state::CLOSED;
  if (clear_mask)
    obj->opmask_ = 0;
  if (obj->socket_->is_open())
  {
    io_watcher_.mod_event(obj->socket_->native_handle(), 0, socket_event::readwrite);
    obj->socket_->close();
    return true;
  }
  return false;
}

int io_service::do_resolve(io_channel* ctx)
{
  if (yasio__testbits(ctx->properties_, YCPF_HOST_DIRTY))
  {
    yasio__clearbits(ctx->properties_, YCPF_HOST_DIRTY | YCPF_NEEDS_QUERY);
    ctx->remote_eps_.clear();
    ip::endpoint ep;
#if defined(YASIO_ENABLE_UDS) && YASIO__HAS_UDS
    if (yasio__testbits(ctx->properties_, YCM_UDS))
    {
      ep.as_un(ctx->remote_host_.c_str());
      ctx->remote_eps_.push_back(ep);
      return 0;
    }
#endif
    if (ep.as_in(ctx->remote_host_.c_str(), ctx->remote_port_))
      ctx->remote_eps_.push_back(ep);
    else
      yasio__setbits(ctx->properties_, YCPF_NEEDS_QUERY);
  }
  if (yasio__testbits(ctx->properties_, YCPF_PORT_DIRTY))
  {
    yasio__clearbits(ctx->properties_, YCPF_PORT_DIRTY);
    for (auto& ep : ctx->remote_eps_)
      ep.port(ctx->remote_port_);
  }

  if (!ctx->remote_eps_.empty())
  {
    if (!yasio__testbits(ctx->properties_, YCPF_NEEDS_QUERY))
      return 0; // remote host is IP address, no needs to query

    update_dns_status(); // will reset our resolved address expire time

    if (ctx->error_ == EINPROGRESS)
      return 0; // queried address not consumed by this connect flow

    if (!address_expired(ctx))
      return 0; // queried address not exipred

    ctx->remote_eps_.clear();
  }

  if (!ctx->remote_host_.empty())
  {
    if (!ctx->error_)
      start_query(ctx);
  }
  else
    ctx->error_ = yasio::errc::no_available_address;
  return -1;
}
void io_service::start_query(io_channel* ctx)
{
  ctx->set_last_errno(EINPROGRESS);
  YASIO_KLOGD("[index: %d] start query %s...", ctx->index_, ctx->remote_host_.c_str());
#if defined(YASIO_ENABLE_ARES_PROFILER)
  ctx->query_start_time_ = highp_clock();
#endif
#if !defined(YASIO_USE_CARES)
  // init async name query thread state
  auto resolving_host                           = ctx->remote_host_;
  auto resolving_port                           = ctx->remote_port_;
  std::weak_ptr<cxx17::shared_mutex> weak_mutex = life_mutex_;
  std::weak_ptr<life_token> life_token          = life_token_;
  std::thread async_resolv_thread([this, life_token, weak_mutex, resolving_host, resolving_port, ctx] {
    // check life token
    if (life_token.use_count() < 1)
      return;

    // preform blocking resolving safe
    std::vector<ip::endpoint> remote_eps;
    int error = options_.resolv_(remote_eps, resolving_host.c_str(), resolving_port);

    // lock perform update dns state of the channel
    auto pmtx = weak_mutex.lock();
    if (!pmtx)
      return;
    cxx17::shared_lock<cxx17::shared_mutex> lck(*pmtx);

    // check life token again, when io_service cleanup done, life_token's use_count will be 0,
    // otherwise, we can safe to do follow assignments.
    if (life_token.use_count() < 1)
      return;
    if (error == 0)
    {
      ctx->remote_eps_         = std::move(remote_eps);
      ctx->query_success_time_ = highp_clock();
#  if defined(YASIO_ENABLE_ARES_PROFILER)
      YASIO_KLOGD("[index: %d] query %s succeed, cost: %g(ms)", ctx->index_, ctx->remote_host_.c_str(),
                  (ctx->query_success_time_ - ctx->query_start_time_) / 1000.0);
#  endif
    }
    else
    {
      ctx->set_last_errno(yasio::errc::resolve_host_failed);
      YASIO_KLOGE("[index: %d] query %s failed, ec=%d, detail:%s", ctx->index_, ctx->remote_host_.c_str(), error, xxsocket::gai_strerror(error));
    }
    this->wakeup();
  });
  async_resolv_thread.detach();
#else
  ares_addrinfo_hints hint;
  memset(&hint, 0x0, sizeof(hint));
  hint.ai_family             = local_address_family();
  char sport[sizeof "65535"] = {'\0'};
  const char* service        = nullptr;
  if (ctx->remote_port_ > 0)
  {
    snprintf(sport, sizeof(sport), "%u", ctx->remote_port_);
    service = sport;
  }
  ares_work_started();
  ::ares_getaddrinfo(this->ares_, ctx->remote_host_.c_str(), service, &hint, io_service::ares_getaddrinfo_cb, ctx);
#endif
}
void io_service::update_dns_status()
{
  if (this->options_.dns_dirty_)
  {
    this->options_.dns_dirty_ = false;
#if defined(YASIO_USE_CARES)
    recreate_ares_channel();
#endif
    for (auto channel : this->channels_)
      channel->query_success_time_ = 0;
  }
}
int io_service::resolve(std::vector<ip::endpoint>& endpoints, const char* hostname, unsigned short port)
{ // prob v4, v6, v4mapped
  if (xxsocket::resolve_v4(endpoints, hostname, port) == 0)
    return 0;
  if (xxsocket::resolve_v6(endpoints, hostname, port) == 0)
    return 0;
  return xxsocket::resolve_v4to6(endpoints, hostname, port);
}
void io_service::wakeup() { io_watcher_.wakeup(); }
const char* io_service::strerror(int error)
{
  switch (error)
  {
    case yasio::errc::resolve_host_failed:
      return "Resolve host failed!";
    case yasio::errc::no_available_address:
      return "No available address!";
    case yasio::errc::shutdown_by_localhost:
      return "An existing connection was shutdown by local host!";
    case yasio::errc::invalid_packet:
      return "Invalid packet!";
    case yasio::errc::ssl_handshake_failed:
      return "SSL handshake failed!";
    case yasio::errc::ssl_write_failed:
      return "SSL write failed!";
    case yasio::errc::ssl_read_failed:
      return "SSL read failed!";
    case yasio::errc::read_timeout:
      return "The remote host did not respond after a period of time.";
    case yasio::errc::eof:
      return "End of file.";
    case -1:
      return "Unknown error!";
    default:
      return xxsocket::strerror(error);
  }
}
void io_service::set_option(int opt, ...)
{
  va_list ap;
  va_start(ap, opt);
  set_option_internal(opt, ap);
  va_end(ap);
}
void io_service::set_option_internal(int opt, va_list ap) // lgtm [cpp/poorly-documented-function]
{
  switch (opt)
  {
    case YOPT_S_NO_DISPATCH:
      options_.no_dispatch_ = !!va_arg(ap, int);
      break;
    case YOPT_S_TCP_KEEPALIVE:
      options_.tcp_keepalive_.onoff    = 1;
      options_.tcp_keepalive_.idle     = va_arg(ap, int);
      options_.tcp_keepalive_.interval = va_arg(ap, int);
      options_.tcp_keepalive_.probs    = va_arg(ap, int);
      break;
    case YOPT_S_RESOLV_FN:
      options_.resolv_ = *va_arg(ap, resolv_fn_t*);
      break;
    case YOPT_S_PRINT_FN: {
      auto ncb = *va_arg(ap, print_fn_t*);
      if (ncb)
        options_.print_ = [=](int, const char* msg) { ncb(msg); };
      else
        options_.print_ = nullptr;
    }
    break;
    case YOPT_S_PRINT_FN2:
      options_.print_ = *va_arg(ap, print_fn2_t*);
      break;
    case YOPT_S_NO_NEW_THREAD:
      this->options_.no_new_thread_ = !!va_arg(ap, int);
      break;
#if defined(YASIO_SSL_BACKEND)
    case YOPT_S_SSL_CACERT:
      this->options_.cafile_ = va_arg(ap, const char*);
      break;
#endif
    case YOPT_S_CONNECT_TIMEOUT:
      options_.connect_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::micro::den;
      break;
    case YOPT_S_CONNECT_TIMEOUTMS:
      options_.connect_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::milli::den;
      break;
    case YOPT_S_DNS_CACHE_TIMEOUT:
      options_.dns_cache_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::micro::den;
      break;
    case YOPT_S_DNS_CACHE_TIMEOUTMS:
      options_.dns_cache_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::milli::den;
      break;
    case YOPT_S_DNS_QUERIES_TIMEOUT:
      options_.dns_queries_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::micro::den;
      break;
    case YOPT_S_DNS_QUERIES_TIMEOUTMS:
      options_.dns_queries_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * std::milli::den;
      break;
    case YOPT_S_DNS_QUERIES_TRIES:
      options_.dns_queries_tries_ = va_arg(ap, int);
      break;
    case YOPT_S_DNS_DIRTY:
      options_.dns_dirty_ = true;
      break;
#if defined(YASIO_USE_CARES)
    case YOPT_S_DNS_LIST:
      options_.name_servers_ = va_arg(ap, const char*);
      options_.dns_dirty_    = true;
      break;
#endif
    case YOPT_S_EVENT_CB:
      options_.on_event_ = *va_arg(ap, event_cb_t*);
      break;
    case YOPT_S_DEFER_EVENT_CB:
      options_.on_defer_event_ = *va_arg(ap, defer_event_cb_t*);
      break;
    case YOPT_S_FORWARD_PACKET:
      options_.forward_packet_ = !!va_arg(ap, int);
      break;
#if defined(_WIN32)
    case YOPT_S_HRES_TIMER:
      options_.hres_timer_ = !!va_arg(ap, int);
      break;
#endif
#if defined(YASIO_SSL_BACKEND)
    case YOPT_S_SSL_CERT:
      options_.crtfile_ = va_arg(ap, const char*);
      options_.keyfile_ = va_arg(ap, const char*);
      break;
#endif
    case YOPT_C_UNPACK_PARAMS: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->uparams_.max_frame_length    = va_arg(ap, int);
        channel->uparams_.length_field_offset = va_arg(ap, int);
        channel->uparams_.length_field_length = yasio::clamp(va_arg(ap, int), YASIO_SSIZEOF(int8_t), YASIO_SSIZEOF(int));
        channel->uparams_.length_adjustment   = va_arg(ap, int);
      }
      break;
    }
    case YOPT_C_UNPACK_STRIP: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->uparams_.initial_bytes_to_strip = yasio::clamp(va_arg(ap, int), 0, YASIO_UNPACK_MAX_STRIP);
      break;
    }
    case YOPT_C_UNPACK_NO_BSWAP: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->uparams_.no_bswap = va_arg(ap, int);
      break;
    }
    case YOPT_C_UNPACK_FN: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->decode_len_ = *va_arg(ap, decode_len_fn_t*);
      break;
    }
    case YOPT_C_LOCAL_HOST: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->local_host_ = va_arg(ap, const char*);
      break;
    }
    case YOPT_C_LOCAL_PORT: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->local_port_ = (u_short)va_arg(ap, int);
      break;
    }
    case YOPT_C_LOCAL_ENDPOINT: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->local_host_ = va_arg(ap, const char*);
        channel->local_port_ = ((u_short)va_arg(ap, int));
      }
      break;
    }
    case YOPT_C_REMOTE_HOST: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->set_host(va_arg(ap, const char*));
      break;
    }
    case YOPT_C_REMOTE_PORT: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->set_port((u_short)va_arg(ap, int));
      break;
    }
    case YOPT_C_REMOTE_ENDPOINT: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->set_host(va_arg(ap, const char*));
        channel->set_port((u_short)va_arg(ap, int));
      }
      break;
    }
    case YOPT_C_MCAST_IF: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        const char* ifaddr = va_arg(ap, const char*);
        if (ifaddr)
          channel->multiif_.as_in(ifaddr, (unsigned short)0);
      }
      break;
    }
    case YOPT_C_ENABLE_MCAST: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        const char* addr = va_arg(ap, const char*);
        int loopback     = va_arg(ap, int);
        channel->enable_multicast(addr, loopback);
        if (channel->socket_->is_open())
          channel->join_multicast_group();
      }
      break;
    }
    case YOPT_C_DISABLE_MCAST: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->disable_multicast();
      break;
    }
    case YOPT_C_MOD_FLAGS: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        yasio__setbits(channel->properties_, (uint32_t)va_arg(ap, int));
        yasio__clearbits(channel->properties_, (uint32_t)va_arg(ap, int));
      }
      break;
    }
#if defined(YASIO_ENABLE_KCP)
    case YOPT_C_KCP_CONV: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->kcp_options().kcp_conv_ = va_arg(ap, int);
      break;
    }
    case YOPT_C_KCP_NODELAY: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        // nodelay:int, interval:int, resend:int, nc:int.
        channel->kcp_options().kcp_nodelay_  = va_arg(ap, int);
        channel->kcp_options().kcp_interval_ = va_arg(ap, int);
        channel->kcp_options().kcp_resend_   = va_arg(ap, int);
        channel->kcp_options().kcp_ncwnd_    = va_arg(ap, int);
      }
      break;
    }
    case YOPT_C_KCP_WINDOW_SIZE: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->kcp_options().kcp_sndwnd_ = va_arg(ap, int);
        channel->kcp_options().kcp_rcvwnd_ = va_arg(ap, int);
      }
      break;
    }
    case YOPT_C_KCP_MTU: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->kcp_options().kcp_mtu_ = va_arg(ap, int);
      break;
    }
    case YOPT_C_KCP_RTO_MIN: {
      auto channel = channel_at(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->kcp_options().kcp_minrto_ = va_arg(ap, int);
      break;
    }
#endif
    case YOPT_T_CONNECT: {
      auto transport = va_arg(ap, transport_handle_t);
      if (transport && transport->is_open() && (transport->ctx_->properties_ & 0xff) == YCK_UDP_CLIENT)
        static_cast<io_transport_udp*>(transport)->connect();
      break;
    }
    case YOPT_T_DISCONNECT: {
      auto transport = va_arg(ap, transport_handle_t);
      if (transport && transport->is_open() && (transport->ctx_->properties_ & 0xff) == YCK_UDP_CLIENT)
        static_cast<io_transport_udp*>(transport)->disconnect();
      break;
    }
    case YOPT_B_SOCKOPT: {
      auto obj = va_arg(ap, io_base*);
      if (obj && obj->socket_ && obj->socket_->is_open())
      {
        auto optlevel = va_arg(ap, int);
        auto optname  = va_arg(ap, int);
        auto optval   = va_arg(ap, void*);
        auto optlen   = va_arg(ap, int);
        obj->socket_->set_optval(optlevel, optname, optval, optlen);
      }
      break;
    }
  }
}
} // namespace inet
} // namespace yasio

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif
