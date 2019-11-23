//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 HALX99

HAL: Hardware Abstraction Layer
X99: Intel X99

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
#ifndef YASIO__CORE_CPP
#define YASIO__CORE_CPP
#if !defined(YASIO_HEADER_ONLY)
#  include "yasio/yasio.hpp"
#endif
#include <limits>
#if defined(_WIN32)
#  include <io.h>
#  define YASIO_O_OPEN_FLAGS O_CREAT | O_RDWR | O_BINARY, S_IWRITE | S_IREAD
#  define ftruncate _chsize
#else
#  include <unistd.h>
#  define YASIO_O_OPEN_FLAGS O_CREAT | O_RDWR, S_IRWXU
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(YASIO_HAVE_KCP)
#  include "yasio/kcp/ikcp.h"
#endif

#define YASIO_SLOG_IMPL(options, format, ...)                                                      \
  do                                                                                               \
  {                                                                                                \
    if (options.print_)                                                                            \
      options.print_(::yasio::strfmt(127, "[yasio][%lld]" format "\n",                             \
                                     highp_clock<system_clock_t>(), ##__VA_ARGS__)                 \
                         .c_str());                                                                \
    else                                                                                           \
      YASIO_LOG("[%lld]" format, highp_clock<system_clock_t>(), ##__VA_ARGS__);                    \
  } while (false)

#define YASIO_SLOG(format, ...) YASIO_SLOG_IMPL(options_, format, ##__VA_ARGS__)
#if !defined(YASIO_VERBOS_LOG)
#  define YASIO_SLOGV(fmt, ...) (void)0
#else
#  define YASIO_SLOGV YASIO_SLOG
#endif

#define YASIO_ANY_ADDR(flags) (flags) & ipsv_ipv4 ? "0.0.0.0" : "::"

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 6320 6322 4996)
#endif

namespace yasio
{
namespace inet
{
namespace
{
// error code
enum
{
  YERR_OK                 = 0,    // NO ERROR.
  YERR_INVALID_PACKET     = -500, // Invalid packet.
  YERR_DPL_ILLEGAL_PDU    = -499, // Decode pdu length error.
  YERR_RESOLV_HOST_FAILED = -498, // Resolve host failed.
  YERR_NO_AVAIL_ADDR      = -497, // No available address to connect.
  YERR_LOCAL_SHUTDOWN     = -496, // Local shutdown the connection.
};

// event mask
enum
{
  YEM_POLLIN  = 1,
  YEM_POLLOUT = 2,
  YEM_POLLERR = 4,
};

// op mask
enum
{
  YOPM_OPEN_CHANNEL    = 1,
  YOPM_CLOSE_CHANNEL   = 1 << 1,
  YOPM_CLOSE_TRANSPORT = 1 << 2
};

// dns queries state
enum : u_short
{
  YDQS_READY = 1,
  YDQS_DIRTY,
  YDQS_INPRROGRESS,
  YDQS_FAILED,
  YDQSF_QUERIES_NEEDED = 1 << 8
};

// channel state
enum : short
{
  YCS_CLOSED,
  YCS_OPENING,
  YCS_OPENED,
};

#define YDQS_CHECK_STATE(what, value) ((what & 0x00ff) == value)
#define YDQS_SET_STATE(what, value) (what = (what & 0xff00) | value)
#define YDQS_GET_STATE(what) (what & 0x00ff)

#if defined(_WIN32)
const DWORD MS_VC_EXCEPTION = 0x406D1388;
#  pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
  DWORD dwType;     // Must be 0x1000.
  LPCSTR szName;    // Pointer to name (in user addr space).
  DWORD dwThreadID; // Thread ID (-1=caller thread).
  DWORD dwFlags;    // Reserved for future use, must be zero.
} THREADNAME_INFO;
#  pragma pack(pop)
static void _set_thread_name(const char* threadName)
{
  THREADNAME_INFO info;
  info.dwType     = 0x1000;
  info.szName     = threadName;
  info.dwThreadID = GetCurrentThreadId();
  info.dwFlags    = 0;
#  if !defined(__MINGW64__) && !defined(__MINGW32__)
  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {}
#  endif
}
#elif defined(ANDROID)
#  define _set_thread_name(name) pthread_setname_np(pthread_self(), name)
#elif defined(__APPLE__)
#  define _set_thread_name(name) pthread_setname_np(name)
#else
#  define _set_thread_name(name)
#endif
} // namespace

class a_pdu
{
public:
  a_pdu(std::vector<char>&& buffer, std::function<void()>&& handler)
      : offset_(0), buffer_(std::move(buffer)), handler_(std::move(handler))
  {}

  size_t offset_;            // offset
  std::vector<char> buffer_; // sending data buffer
  std::function<void()> handler_;
#if !defined(YASIO_DISABLE_OBJECT_POOL)
  DEFINE_CONCURRENT_OBJECT_POOL_ALLOCATION(a_pdu, 512)
#endif
};

/// deadline_timer
void deadline_timer::async_wait(timer_cb_t cb)
{
  this->service_.schedule_timer(this, std::move(cb));
}

void deadline_timer::cancel()
{
  if (!expired())
  {
    this->expire_time_ = highp_clock_t::now() - duration_;
    this->cancelled_   = true;
    this->service_.interrupt();
  }
}

void deadline_timer::unschedule() { this->service_.remove_timer(this); }

/// io_channel
io_channel::io_channel(io_service& service) : deadline_timer_(service)
{
  socket_.reset(new xxsocket());
  state_             = YCS_CLOSED;
  dns_queries_state_ = YDQS_FAILED;

  decode_len_ = [=](void* ptr, int len) { return this->__builtin_decode_len(ptr, len); };
}

int io_channel::join_multicast_group(std::shared_ptr<xxsocket>& sock, int loopback)
{
  if (sock && !this->remote_eps_.empty())
  {
    auto& ep = this->remote_eps_[0];
    // loopback
    sock->set_optval(ep.af() == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                     ep.af() == AF_INET ? IP_MULTICAST_LOOP : IPV6_MULTICAST_LOOP, loopback);
    // ttl
    sock->set_optval(ep.af() == AF_INET ? IPPROTO_IP : IPPROTO_IPV6,
                     ep.af() == AF_INET ? IP_MULTICAST_TTL : IPV6_MULTICAST_HOPS,
                     YASIO_DEFAULT_MULTICAST_TTL);

    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = 0;
    mreq.imr_multiaddr.s_addr = ep.in4_.sin_addr.s_addr;
    return sock->set_optval(IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, (int)sizeof(mreq));
  }
  return -1;
}

void io_channel::leave_multicast_group(std::shared_ptr<xxsocket>& sock)
{
  if (sock && !this->remote_eps_.empty())
  {
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = 0;
    mreq.imr_multiaddr.s_addr = this->remote_eps_[0].in4_.sin_addr.s_addr;
    sock->set_optval(IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, (int)sizeof(mreq));
  }
}

void io_channel::setup_remote_host(std::string host)
{
  if (this->remote_host_ != host)
  {
    this->remote_host_ = std::move(host);

    this->remote_eps_.clear();

    ip::endpoint ep;
    if (ep.assign(this->remote_host_.c_str(), this->remote_port_))
    {
      this->remote_eps_.push_back(ep);
      this->dns_queries_state_ = YDQS_READY;
    }
    else
      this->dns_queries_state_ = YDQSF_QUERIES_NEEDED | YDQS_DIRTY;
  }
}

void io_channel::setup_remote_port(u_short port)
{
  if (port == 0)
    return;
  if (this->remote_port_ != port)
  {
    this->remote_port_ = port;
    if (!this->remote_eps_.empty())
      for (auto& ep : this->remote_eps_)
        ep.port(port);
  }
}

int io_channel::__builtin_decode_len(void* ud, int n)
{
  if (lfb_.length_field_offset >= 0)
  {
    if (n >= (lfb_.length_field_offset + lfb_.length_field_length))
    {
      int32_t length = -1;
      switch (lfb_.length_field_length)
      {
        case 4:
          length =
              ntohl(*reinterpret_cast<int32_t*>((unsigned char*)ud + lfb_.length_field_offset)) +
              lfb_.length_adjustment;
          break;
        case 3:
          length = 0;
          memcpy(&length, (unsigned char*)ud + lfb_.length_field_offset, 3);
          length = (ntohl(length) >> 8) + lfb_.length_adjustment;
          break;
        case 2:
          length =
              ntohs(*reinterpret_cast<uint16_t*>((unsigned char*)ud + lfb_.length_field_offset)) +
              lfb_.length_adjustment;
          break;
        case 1:
          length = *((unsigned char*)ud + lfb_.length_field_offset) + lfb_.length_adjustment;
          break;
      }
      if (length > lfb_.max_frame_length)
        length = -1;
      return length;
    }
    return 0;
  }
  return n;
}

// -------------------- io_transport ---------------------
io_transport::io_transport(io_channel* ctx, std::shared_ptr<xxsocket> sock) : ctx_(ctx)
{
  static unsigned int s_object_id = 0;
  this->state_                    = YCS_OPENED;
  this->id_                       = ++s_object_id;
  this->socket_                   = sock;
  this->valid_                    = true;
  this->ud_.ptr                   = nullptr;
}

// -------------------- io_transport_posix ---------------------
io_transport_posix::io_transport_posix(io_channel* ctx, std::shared_ptr<xxsocket> sock)
    : io_transport(ctx, sock)
{
  this->set_primitives(!(ctx->flags_ & YCF_MCAST) || !(ctx->mask_ & YCM_CLIENT));
}
void io_transport_posix::write(std::vector<char>&& buffer, std::function<void()>&& handler)
{
  send_queue_.emplace(std::make_shared<a_pdu>(std::move(buffer), std::move(handler)));
}
int io_transport_posix::do_read(int& error)
{
  int n = recv_cb_(buffer_ + offset_, sizeof(buffer_) - offset_);
  error = n < 0 ? xxsocket::get_last_errno() : 0;
  return n;
}
bool io_transport_posix::do_write(long long& max_wait_duration)
{
  bool ret = false;
  do
  {
    if (!socket_->is_open())
      break;

    int error     = -1;
    a_pdu_ptr* pv = send_queue_.peek();
    if (pv != nullptr)
    {
      auto v                 = *pv;
      auto outstanding_bytes = static_cast<int>(v->buffer_.size() - v->offset_);
      int n                  = send_cb_(v->buffer_.data() + v->offset_, outstanding_bytes);
      if (n == outstanding_bytes)
      { // All pdu bytes sent.
        send_queue_.pop();
#if defined(YASIO_VERBOS_LOG)
        YASIO_SLOG_IMPL(get_service().options_,
                        "[index: %d] do_write ok, A packet sent "
                        "success, packet size:%d",
                        cindex(), static_cast<int>(v->buffer_.size()),
                        socket_->local_endpoint().to_string().c_str(),
                        socket_->peer_endpoint().to_string().c_str());
#endif
        if (v->handler_)
          v->handler_();
      }
      else if (n > 0)
      {
        // #performance: change offset only, remain data will be send next loop.
        v->offset_ += n;
        outstanding_bytes = static_cast<int>(v->buffer_.size() - v->offset_);
      }
      else
      { // n <= 0
        error = xxsocket::get_last_errno();
        if (SHOULD_CLOSE_1(n, error))
        {
          if (((ctx_->mask_ & YCM_UDP) == 0) || error != EPERM)
          { // Fix issue: #126, simply ignore EPERM for UDP
            set_last_errno(error);
            offset_ = n;
            break;
          }
        }
      }
    }

    // If still have work to do.
    if (!send_queue_.empty())
      max_wait_duration = error != EWOULDBLOCK ? 0 : YASIO_WOULDBLOCK_WAIT_DURATION;

    ret = true;
  } while (false);

  return ret;
}
void io_transport_posix::set_primitives(bool connected)
{
  if (connected)
  {
    this->send_cb_ = [=](const void* data, int len) { return socket_->send_i(data, len); };
    this->recv_cb_ = [=](void* data, int len) { return socket_->recv_i(data, len, 0); };
  }
  else
  {
    this->send_cb_ = [=](const void* data, int len) {
      return socket_->sendto_i(data, len, ctx_->remote_eps_[0]);
    };
    this->recv_cb_ = [=](void* data, int len) {
      ip::endpoint peer;
      int n = socket_->recvfrom_i(data, len, peer);

      // Now the 'peer' is a real host address
      // So  we can use connect to establish 4 tuple with 'peer' & leave the multicast group.
      if (n > 0 && 0 == socket_->connect_n(peer))
      {
        ctx_->leave_multicast_group(socket_);

        YASIO_SLOG_IMPL(
            get_service().options_,
            "[index: %d] the connection #%u [%s] --> [%s] is established through multicast: [%s].",
            ctx_->index_, this->id_, socket_->local_endpoint().to_string().c_str(),
            socket_->peer_endpoint().to_string().c_str(), ctx_->remote_host_.c_str());

        set_primitives(true);
      }
      return n;
    };
  }
}
#if defined(YASIO_HAVE_KCP)
// ----------------------- io_transport_kcp ------------------
io_transport_kcp::io_transport_kcp(io_channel* ctx, std::shared_ptr<xxsocket> sock)
    : io_transport(ctx, sock), kcp_(nullptr)
{
  this->kcp_ = ::ikcp_create(0, this);
  ::ikcp_nodelay(this->kcp_, 1, 16 /*MAX_WAIT_DURATION / 1000*/, 2, 1);
  ::ikcp_setoutput(this->kcp_, [](const char* buf, int len, ::ikcpcb* /*kcp*/, void* user) {
    auto t = (transport_handle_t)user;
    return t->socket_->send_i(buf, len);
  });
}
io_transport_kcp::~io_transport_kcp() { ::ikcp_release(this->kcp_); }

void io_transport_kcp::write(std::vector<char>&& buffer, std::function<void()>&& /*handler*/)
{
  std::lock_guard<std::recursive_mutex> lck(send_mtx_);
  ::ikcp_send(kcp_, buffer.data(), static_cast<int>(buffer.size()));
}
int io_transport_kcp::do_read(int& error)
{
  char sbuf[2048];
  int n = socket_->recv_i(sbuf, sizeof(sbuf));
  if (n > 0)
  { // ikcp in event always in service thread, so no need to lock, TODO: confirm.
    // 0: ok, -1: again, -3: error
    if (0 == ::ikcp_input(kcp_, sbuf, n))
      n = ::ikcp_recv(kcp_, buffer_ + offset_, sizeof(buffer_) - offset_);
    else
    { // current, simply regards -1,-3 as error and trigger connection lost event.
      n     = 0;
      error = YERR_INVALID_PACKET;
    }
  }
  else
    error = xxsocket::get_last_errno();
  return n;
}
bool io_transport_kcp::do_write(long long& max_wait_duration)
{
  std::lock_guard<std::recursive_mutex> lck(send_mtx_);

  auto current = static_cast<IUINT32>(highp_clock() / 1000);
  ::ikcp_update(kcp_, current);

  auto expire_time        = ::ikcp_check(kcp_, current);
  long long wait_duration = (long long)(expire_time - current) * 1000;
  if (wait_duration < 0)
    wait_duration = 0;

  if (max_wait_duration > wait_duration)
    max_wait_duration = wait_duration;

  return true;
}
#endif

// ------------------------ io_service ------------------------

io_service::io_service() : state_(io_service::state::IDLE), interrupter_()
{
  FD_ZERO(&fds_array_[read_op]);
  FD_ZERO(&fds_array_[write_op]);
  FD_ZERO(&fds_array_[except_op]);

  maxfdp_ = 0;

  options_.resolv_ = [=](std::vector<ip::endpoint>& eps, const char* host, unsigned short port) {
    return this->__builtin_resolv(eps, host, port);
  };
}

io_service::~io_service()
{
  stop_service();
  if (this->state_ == io_service::state::STOPPED)
    cleanup();

  for (auto o : tpool_)
    ::operator delete(o);
  tpool_.clear();

  options_.resolv_ = nullptr;
  options_.print_  = nullptr;
}

void io_service::start_service(const io_hostent* channel_eps, int channel_count, io_event_cb_t cb)
{
  if (state_ == io_service::state::IDLE)
  {
    this->init(channel_eps, channel_count, cb);

    this->state_ = io_service::state::RUNNING;
    if (!options_.no_new_thread_)
    {
      this->worker_    = std::thread(&io_service::run, this);
      this->worker_id_ = worker_.get_id();
    }
    else
    {
      this->worker_id_               = std::this_thread::get_id();
      this->options_.deferred_event_ = false;
      run();
      this->state_ = io_service::state::STOPPED;
      cleanup();
    }
  }
}

void io_service::stop_service()
{
  if (this->state_ == io_service::state::RUNNING)
  {
    this->state_ = io_service::state::STOPPING;

    this->interrupt();
    this->wait_service();
  }
  else if (this->state_ == io_service::state::STOPPING)
    this->wait_service();
}

void io_service::wait_service()
{
  if (this->worker_.joinable())
  {
    if (std::this_thread::get_id() != this->worker_id_)
    {
      this->worker_.join();
      this->state_ = io_service::state::STOPPED;
      cleanup();
    }
    else
      errno = EAGAIN;
  }
}

void io_service::init(const io_hostent* channel_eps, int channel_count, io_event_cb_t& cb)
{
  if (this->state_ != io_service::state::IDLE)
    return;
  if (channel_count <= 0)
    return;
  if (cb)
    options_.on_event_ = std::move(cb);

  register_descriptor(interrupter_.read_descriptor(), YEM_POLLIN);

  // Initialize channels
  for (auto i = 0; i < channel_count; ++i)
  {
    auto& channel_ep = channel_eps[i];
    (void)new_channel(channel_ep);
  }

  this->state_ = io_service::state::INITIALIZED;
}

void io_service::cleanup()
{
  if (this->state_ == io_service::state::STOPPED)
  {
    clear_transports();
    clear_channels();
    this->events_.clear();
    this->timer_queue_.clear();

    unregister_descriptor(interrupter_.read_descriptor(), YEM_POLLIN);

    options_.on_event_ = nullptr;

    this->state_ = io_service::state::IDLE;
  }
}

io_channel* io_service::new_channel(const io_hostent& ep)
{
  auto ctx = new io_channel(*this);
  ctx->init(ep.host_, ep.port_);
  ctx->index_ = static_cast<int>(this->channels_.size());
  this->channels_.push_back(ctx);
  return ctx;
}

void io_service::clear_channels()
{
  this->channel_ops_.clear();
  for (auto channel : channels_)
  {
    channel->deadline_timer_.cancel();
    cleanup_io(channel);
    delete channel;
  }
  channels_.clear();
}

void io_service::clear_transports()
{
  for (auto transport : transports_)
  {
    cleanup_io(transport);
    transport->~io_transport();
    this->tpool_.push_back(transport);
  }
  transports_.clear();
}

void io_service::dispatch(int count)
{
  if (options_.on_event_)
    this->events_.consume(count, options_.on_event_);
}

void io_service::run()
{
  _set_thread_name("yasio");

  // Call once at startup
  this->ipsv_ = static_cast<u_short>(xxsocket::getipsv());

  // event loop
  fd_set fds_array[max_ops];
  long long max_wait_duration = YASIO_MAX_WAIT_DURATION;
  for (; this->state_ == io_service::state::RUNNING;)
  {
    int nfds = do_evpoll(fds_array, max_wait_duration);
    if (this->state_ != io_service::state::RUNNING)
      break;

    max_wait_duration = YASIO_MAX_WAIT_DURATION;

    if (nfds == -1)
    {
      int ec = xxsocket::get_last_errno();
      YASIO_SLOG("do_evpoll failed, ec=%d, detail:%s\n", ec, io_service::strerror(ec));
      if (ec == EBADF)
        goto _L_end;
      continue; // just continue.
    }

    if (nfds == 0)
      YASIO_SLOGV("%s", "do_evpoll is timeout, do perform_timeout_timers()");

    // Reset the interrupter.
    else if (nfds > 0 && FD_ISSET(this->interrupter_.read_descriptor(), &(fds_array[read_op])))
    {
      interrupter_.reset();
      --nfds;
    }

    // perform active transports
    perform_transports(fds_array, max_wait_duration);

    // perform active channels
    perform_channels(fds_array);

    // perform timeout timers
    perform_timers();
  }

_L_end:
  (void)0; // ONLY for xcode compiler happy.
}

void io_service::perform_transports(fd_set* fds_array, long long& max_wait_duration)
{
  // preform transports
  for (auto iter = transports_.begin(); iter != transports_.end();)
  {
    auto transport = *iter;
    if (do_read(transport, fds_array, max_wait_duration) && do_write(transport, max_wait_duration))
      ++iter;
    else
    {
      handle_close(transport);
      iter = transports_.erase(iter);
    }
  }

  /*
    Because Bind() the client socket to the socket address of the listening socket.  On Linux this
    essentially passes the responsibility for receiving data for the client session from the
    well-known listening socket, to the newly allocated client socket.  It is important to note
    that this behavior is not the same on other platforms, like Windows (unfortunately), detail
    see:
    https://blog.grijjy.com/2018/08/29/creating-high-performance-udp-servers-on-windows-and-linux
    https://cloud.tencent.com/developer/article/1004555
    So we emulate thus by ourself.
  */
#if defined(_WIN32)
  for (auto iter = dgram_transports_.begin(); iter != dgram_transports_.end();)
  {
    auto transport = iter->second;
    if (do_write(transport, max_wait_duration))
      ++iter;
    else
    {
      handle_close(transport);
      iter = dgram_transports_.erase(iter);
    }
  }
#endif
}

void io_service::perform_channels(fd_set* fds_array)
{
  if (!this->channel_ops_.empty())
  {
    // perform active channels
    std::lock_guard<std::recursive_mutex> lck(this->channel_ops_mtx_);
    for (auto iter = this->channel_ops_.begin(); iter != this->channel_ops_.end();)
    {
      auto ctx    = *iter;
      bool finish = true;
      if (ctx->mask_ & YCM_CLIENT)
      { // resolving, opening
        if (ctx->opmask_ & YOPM_OPEN_CHANNEL)
        {
          switch (this->query_ares_state(ctx))
          {
            case YDQS_READY:
              do_nonblocking_connect(ctx);
              break;
            case YDQS_FAILED:
              YASIO_SLOG("[index: %d] getaddrinfo failed, ec=%d, detail:%s", ctx->index_,
                         ctx->error_, xxsocket::gai_strerror(ctx->error_));
              handle_connect_failed(ctx, YERR_RESOLV_HOST_FAILED);
              break;
            default:; // YDQS_INPRROGRESS
          }
        }
        else if (ctx->state_ == YCS_OPENING)
        {
          do_nonblocking_connect_completion(ctx, fds_array);
        }
        finish = ctx->error_ != EINPROGRESS;
      }
      else if (ctx->mask_ & YCM_SERVER)
      {
        auto opmask = ctx->opmask_;
        if (opmask & YOPM_CLOSE_CHANNEL)
          cleanup_io(ctx);

        if (opmask & YOPM_OPEN_CHANNEL)
          do_nonblocking_accept(ctx);

        finish = (ctx->state_ != YCS_OPENED);
        if (!finish)
          do_nonblocking_accept_completion(ctx, fds_array);
      }

      if (finish)
        iter = this->channel_ops_.erase(iter);
      else
        ++iter;
    }
  }
}

void io_service::close(size_t cindex)
{
  // Gets channel context
  if (cindex >= channels_.size())
    return;
  auto ctx = channels_[cindex];

  if (!(ctx->opmask_ & YOPM_CLOSE_CHANNEL))
  {
    if (close_internal(ctx))
      this->interrupt();
  }
}

void io_service::close(transport_handle_t transport)
{
  if (transport->is_open() && !(transport->opmask_ & YOPM_CLOSE_TRANSPORT))
  {
    transport->opmask_ |= YOPM_CLOSE_TRANSPORT;
    if (transport->ctx_->mask_ & YCM_TCP)
      transport->socket_->shutdown();
    this->interrupt();
  }
}

bool io_service::is_open(transport_handle_t transport) const
{
  return transport->state_ == YCS_OPENED;
}

bool io_service::is_open(size_t cindex) const
{
  auto ctx = cindex_to_handle(cindex);
  return ctx != nullptr && ctx->state_ == YCS_OPENED;
}

void io_service::reopen(transport_handle_t transport)
{
  auto ctx = transport->ctx_;
  if (ctx->mask_ & YCM_CLIENT) // Only client channel support reopen by transport
    open_internal(ctx);
}

void io_service::open(size_t cindex, int channel_mask)
{
  auto ctx = cindex_to_handle(cindex);
  if (ctx != nullptr)
  {
    ctx->mask_ = static_cast<u_short>(channel_mask);
    if (channel_mask & YCM_TCP)
      ctx->protocol_ = SOCK_STREAM;
    else if (channel_mask & YCM_UDP)
      ctx->protocol_ = SOCK_DGRAM;

    open_internal(ctx);
  }
}

io_channel* io_service::cindex_to_handle(size_t cindex) const
{
  if (cindex < channels_.size())
    return channels_[cindex];
  return nullptr;
}

void io_service::handle_close(transport_handle_t transport)
{
  auto ptr = transport;
  auto ctx = ptr->ctx_;
  auto ec  = ptr->error_;
  // @Because we can't retrive peer endpoint when connect reset by peer, so use id to trace.
  YASIO_SLOG("[index: %d] the connection #%u is lost, offset:%d, ec=%d, detail:%s", ctx->index_,
             ptr->id_, ptr->offset_, ec, io_service::strerror(ec));

  cleanup_io(ptr);

  // @Update state
  if (ctx->mask_ & YCM_CLIENT)
  {
    ctx->state_ = YCS_CLOSED;
    ctx->opmask_ &= ~YOPM_CLOSE_TRANSPORT;
    ctx->set_last_errno(0);
  } // server channel, do nothing.

  deallocate_transport(ptr);

  // @Notify connection lost
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECTION_LOST, ec, ptr)));

  // @Process tcp client reconnect
  if (ctx->mask_ == YCM_TCP_CLIENT)
  {
    if (options_.reconnect_timeout_ > 0 && ctx->state_ != YCS_OPENING)
    {
      ctx->state_ = YCS_OPENING;
      this->schedule(options_.reconnect_timeout_, [=](bool cancelled) {
        if (!cancelled)
          this->open_internal(ctx, true);
      });
    }
  }
}

void io_service::register_descriptor(const socket_native_type fd, int flags)
{
  if ((flags & YEM_POLLIN) != 0)
    FD_SET(fd, &(fds_array_[read_op]));

  if ((flags & YEM_POLLOUT) != 0)
    FD_SET(fd, &(fds_array_[write_op]));

  if ((flags & YEM_POLLERR) != 0)
    FD_SET(fd, &(fds_array_[except_op]));

  if (maxfdp_ < static_cast<int>(fd) + 1)
    maxfdp_ = static_cast<int>(fd) + 1;
}
void io_service::unregister_descriptor(const socket_native_type fd, int flags)
{
  if ((flags & YEM_POLLIN) != 0)
    FD_CLR(fd, &(fds_array_[read_op]));

  if ((flags & YEM_POLLOUT) != 0)
    FD_CLR(fd, &(fds_array_[write_op]));

  if ((flags & YEM_POLLERR) != 0)
    FD_CLR(fd, &(fds_array_[except_op]));
}
int io_service::write(transport_handle_t transport, std::vector<char> buffer,
                      std::function<void()> handler)
{
  if (transport && transport->is_open())
  {
    if (!buffer.empty())
    {
      transport->write(std::move(buffer), std::move(handler));
      this->interrupt();
      return static_cast<int>(buffer.size());
    }
    return 0;
  }
  else
  {
    YASIO_SLOG("[transport: %p] send failed, the connection not ok!", (void*)transport);
    return -1;
  }
}
void io_service::handle_event(event_ptr event)
{
  if (options_.deferred_event_)
    events_.emplace(std::move(event));
  else
    options_.on_event_(std::move(event));
}

void io_service::do_nonblocking_connect(io_channel* ctx)
{
  assert(YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_READY));
  if (this->ipsv_ == 0)
    this->ipsv_ = static_cast<u_short>(xxsocket::getipsv());
  if (ctx->socket_->is_open())
    cleanup_io(ctx);

  ctx->opmask_ &= ~YOPM_OPEN_CHANNEL;

  if (ctx->remote_eps_.empty())
  {
    this->handle_connect_failed(ctx, YERR_NO_AVAIL_ADDR);
    return;
  }

  ctx->state_ = YCS_OPENING;
  auto& ep    = ctx->remote_eps_[0];
  YASIO_SLOG("[index: %d] connecting server %s:%u...", ctx->index_, ctx->remote_host_.c_str(),
             ctx->remote_port_);
  int ret = -1;
  if (ctx->socket_->open(ep.af(), ctx->protocol_))
  {
    if (ctx->flags_ & YCF_REUSEPORT)
      ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);
    if (ctx->local_host_.empty())
      ctx->local_host_ = YASIO_ANY_ADDR(this->ipsv_);

    if ((ctx->local_port_ != 0 || ctx->mask_ & YCM_UDP))
      ctx->socket_->bind(ctx->local_host_.c_str(), ctx->local_port_);

    if (!(ctx->flags_ & YCF_MCAST))
      ret = xxsocket::connect_n(ctx->socket_->native_handle(), ep);
    else
      ret = ctx->join_multicast_group(ctx->socket_, (ctx->flags_ & YCF_MCAST_LOOPBACK) != 0);
    if (ret < 0)
    { // setup no blocking connect
      int error = xxsocket::get_last_errno();
      if (error != EINPROGRESS && error != EWOULDBLOCK)
      {
        this->handle_connect_failed(ctx, error);
      }
      else
      {
        ctx->set_last_errno(EINPROGRESS);
        register_descriptor(ctx->socket_->native_handle(), YEM_POLLIN | YEM_POLLOUT);
        ctx->deadline_timer_.expires_from_now(std::chrono::microseconds(options_.connect_timeout_));
        ctx->deadline_timer_.async_wait([this, ctx](bool cancelled) {
          if (!cancelled && ctx->state_ != YCS_OPENED)
            handle_connect_failed(ctx, ETIMEDOUT);
        });
      }
    }
    else if (ret == 0)
    { // connect server succed immidiately.
      register_descriptor(ctx->socket_->native_handle(), YEM_POLLIN);
      handle_connect_succeed(ctx, ctx->socket_);
    } // !!!NEVER GO HERE
  }
  else
    this->handle_connect_failed(ctx, xxsocket::get_last_errno());
}

void io_service::do_nonblocking_connect_completion(io_channel* ctx, fd_set* fds_array)
{
  assert(ctx->mask_ == YCM_TCP_CLIENT);
  assert(ctx->state_ == YCS_OPENING);

  int error = -1;
  if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[write_op]) ||
      FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
  {
    socklen_t len = sizeof(error);
    if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &len) >=
            0 &&
        error == 0)
    {
      // remove write event avoid high-CPU occupation
      unregister_descriptor(ctx->socket_->native_handle(), YEM_POLLOUT);
      handle_connect_succeed(ctx, ctx->socket_);
    }
    else
    {
      handle_connect_failed(ctx, error);
    }

    ctx->deadline_timer_.cancel();
  }
}

void io_service::do_nonblocking_accept(io_channel* ctx)
{ // channel is server
  cleanup_io(ctx);

  // init ep properly once for bind
  // for server, local_port_ can't be zero
  if (ctx->local_host_.empty())
  {
    if (!(ctx->flags_ & YCF_MCAST))
      ctx->local_host_ = ctx->remote_host_;
    else
      ctx->local_host_ = YASIO_ANY_ADDR(ipsv_);
  }
  if (ctx->local_port_ == 0)
    ctx->local_port_ = ctx->remote_port_;

  ip::endpoint ep(ctx->local_host_.c_str(), ctx->local_port_);

  if (ctx->socket_->open(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6, ctx->protocol_))
  {
    int error = 0;
    if (ctx->flags_ & YCF_REUSEPORT)
      ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);

    if (ctx->socket_->bind(ep) != 0)
    {
      error = xxsocket::get_last_errno();
      YASIO_SLOG("[index: %d] bind failed, ec=%d, detail:%s", ctx->index_, error,
                 io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = YCS_CLOSED;
      return;
    }

    if ((ctx->mask_ & YCM_UDP) || ctx->socket_->listen(YASIO_SOMAXCONN) == 0)
    {
      ctx->state_ = YCS_OPENED;
      ctx->socket_->set_nonblocking(true);

      if (ctx->mask_ & YCM_UDP)
      {
        if (ctx->flags_ & YCF_MCAST)
          ctx->join_multicast_group(ctx->socket_, (ctx->flags_ & YCF_MCAST_LOOPBACK) != 0);
        ctx->buffer_.resize(YASIO_INET_BUFFER_SIZE);
      }
      register_descriptor(ctx->socket_->native_handle(), YEM_POLLIN);
      YASIO_SLOG("[index: %d] socket.fd=%d listening at %s...", ctx->index_,
                 (int)ctx->socket_->native_handle(), ep.to_string().c_str());
    }
    else
    {
      error = xxsocket::get_last_errno();
      YASIO_SLOG("[index: %d] socket.fd=%d listening failed, ec=%d, detail:%s", ctx->index_,
                 (int)ctx->socket_->native_handle(), error, io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = YCS_CLOSED;
    }
  }
}

void io_service::do_nonblocking_accept_completion(io_channel* ctx, fd_set* fds_array)
{
  if (ctx->state_ == YCS_OPENED)
  {
    int error = -1;
    if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
    {
      socklen_t len = sizeof(error);
      if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &len) >=
              0 &&
          error == 0)
      {
        if (ctx->mask_ & YCM_TCP)
        {
          socket_native_type sockfd;
          error = ctx->socket_->accept_n(sockfd);
          if (error == 0)
          {
            handle_connect_succeed(ctx, std::make_shared<xxsocket>(sockfd));
          }
          else // The non blocking tcp accept failed can be ignored.
            YASIO_SLOGV("[index: %d] socket.fd=%d, accept failed, ec=%u", ctx->index(),
                        (int)ctx->socket_->native_handle(), error);
        }
        else // YCM_UDP
        {
          ip::endpoint peer;
          int n = ctx->socket_->recvfrom_i(&ctx->buffer_.front(),
                                           static_cast<int>(ctx->buffer_.size()), peer);
          if (n > 0)
          {
            YASIO_SLOGV("recvfrom peer: %s succeed.", peer.to_string().c_str());

            /* make a transport local --> peer udp session, just like tcp accept */
#if !defined(_WIN32)
            auto transport = make_dgram_transport(ctx, peer);
#else
            // for win32, we manage dgram clients by ourself, and perfrom write operation only in
            // dgram_transports, the read operation still dispatch by channel.
            auto it = this->dgram_transports_.find(peer);
            auto transport =
                it != this->dgram_transports_.end() ? it->second : make_dgram_transport(ctx, peer);
#endif
            if (transport)
            {
              this->handle_event(event_ptr(new io_event(
                  transport->cindex(), YEK_PACKET,
                  std::vector<char>(&ctx->buffer_.front(), &ctx->buffer_.front() + n), transport)));
            }
          }
          else
          {
            error = xxsocket::get_last_errno();
            if (SHOULD_CLOSE_0(n, error))
            {
              YASIO_SLOG("[index: %d] recvfrom failed, ec=%d", ctx->index_, error);
              close(ctx->index_);
            }
          }
        }
      }
    }
  }
}

transport_handle_t io_service::make_dgram_transport(io_channel* ctx, ip::endpoint& peer)
{
  auto client_sock = std::make_shared<xxsocket>();
  if (client_sock->open(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0))
  {
    if (ctx->flags_ & YCF_REUSEPORT)
      client_sock->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);
    int error = client_sock->bind("0.0.0.0", ctx->local_port_) == 0
                    ? xxsocket::connect(client_sock->native_handle(), peer)
                    : -1;
    if (error == 0)
    {
      auto transport = allocate_transport(ctx, std::move(client_sock));
#if !defined(_WIN32)
      handle_connect_succeed(transport);
#else
      notify_connect_succeed(transport);
      this->dgram_transports_.emplace(peer, transport);
#endif
      return transport;
    }
    else
      YASIO_SLOG("%s", "udp-server: open socket fd failed!");
  }

  return nullptr;
}

void io_service::handle_connect_succeed(transport_handle_t transport)
{
  this->transports_.push_back(transport);
  auto ctx = transport->ctx_;
  ctx->set_last_errno(0); // clear errno, value may be EINPROGRESS
  auto& connection = transport->socket_;
  if (ctx->mask_ & YCM_CLIENT)
    ctx->state_ = YCS_OPENED;
  else
  { // tcp/udp server, accept a new client session
    connection->set_nonblocking(true);
    register_descriptor(connection->native_handle(), YEM_POLLIN);
  }
  if (ctx->mask_ & YCM_TCP)
  {
    // apply tcp keepalive options
    if (options_.tcp_keepalive_.onoff)
      connection->set_keepalive(options_.tcp_keepalive_.onoff, options_.tcp_keepalive_.idle,
                                options_.tcp_keepalive_.interval, options_.tcp_keepalive_.probs);
  }

  notify_connect_succeed(transport);
}

void io_service::notify_connect_succeed(transport_handle_t transport)
{
  auto ctx         = transport->ctx_;
  auto& connection = transport->socket_;

  YASIO_SLOG("[index: %d] the connection #%u [%s] --> [%s] is established.", ctx->index_,
             transport->id_, connection->local_endpoint().to_string().c_str(),
             connection->peer_endpoint().to_string().c_str());
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, 0, transport)));
}

transport_handle_t io_service::allocate_transport(io_channel* ctx, std::shared_ptr<xxsocket> socket)
{
  transport_handle_t transport;
  void* vp;
  if (!tpool_.empty())
  { // allocate from pool
    vp = tpool_.back();
    tpool_.pop_back();
  }
  else
    vp = operator new(sizeof(io_transport_posix));
#if defined(YASIO_HAVE_KCP)
  if (!(ctx->flags_ & YCF_KCP))
    transport = new (vp) io_transport_posix(ctx, socket);
  else
    transport = new (vp) io_transport_kcp(ctx, socket);
#else
  transport = new (vp) io_transport_posix(ctx, socket);
#endif

  return transport;
}

void io_service::deallocate_transport(transport_handle_t t)
{
  if (t && t->valid_)
  {
    t->invalid();
    t->~io_transport();
    this->tpool_.push_back(t);
  }
}

void io_service::handle_connect_failed(io_channel* ctx, int error)
{
  cleanup_io(ctx);

  YASIO_SLOG("[index: %d] connect server %s:%u failed, ec=%d, detail:%s", ctx->index_,
             ctx->remote_host_.c_str(), ctx->remote_port_, error, io_service::strerror(error));
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, error, nullptr)));
}

bool io_service::do_read(transport_handle_t transport, fd_set* fds_array,
                         long long& max_wait_duration)
{
  bool ret = false;
  do
  {
    if (!transport->socket_->is_open())
      break;
    if ((transport->opmask_ | transport->ctx_->opmask_) & YOPM_CLOSE_TRANSPORT)
    {
      transport->set_last_errno(YERR_LOCAL_SHUTDOWN);
      break;
    }

    int n = -1, error = EWOULDBLOCK;
    if (FD_ISSET(transport->socket_->native_handle(), &(fds_array[read_op])))
    {
      n = transport->do_read(error);
    }
    if (n > 0 || !SHOULD_CLOSE_0(n, error))
    {
      YASIO_SLOGV("[index: %d] do_read status ok, ec=%d, detail:%s", transport->cindex(), error,
                  io_service::strerror(error));
      if (n == -1)
        n = 0;
#if defined(YASIO_VERBOS_LOG)
      if (n > 0)
      {
        YASIO_SLOG("[index: %d] do_read ok, received data len: %d, "
                   "buffer data "
                   "len: %d",
                   transport->cindex(), n, n + transport->offset_);
      }
#endif
      if (transport->expected_size_ == -1)
      { // decode length
        int length = transport->ctx_->decode_len_(transport->buffer_, transport->offset_ + n);
        if (length > 0)
        {
          transport->expected_size_ = length;
          transport->expected_packet_.reserve(
              (std::min)(transport->expected_size_,
                         YASIO_MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid memory reallocte.
          unpack(transport, transport->expected_size_, n, max_wait_duration);
        }
        else if (length == 0)
        {
          // header insufficient, wait readfd ready at
          // next event step.
          transport->offset_ += n;
        }
        else
        {
          transport->set_last_errno(YERR_DPL_ILLEGAL_PDU);
          break;
        }
      }
      else
      { // process incompleted pdu
        unpack(transport,
               transport->expected_size_ - static_cast<int>(transport->expected_packet_.size()), n,
               max_wait_duration);
      }
    }
    else
    { // n == 0: The return value will be 0 when the peer has performed an orderly shutdown.
      transport->set_last_errno(error);
      transport->offset_ = n;
      break;
    }

    ret = true;

  } while (false);

  return ret;
}

void io_service::unpack(transport_handle_t transport, int bytes_expected, int bytes_transferred,
                        long long& max_wait_duration)
{
  auto bytes_available = bytes_transferred + transport->offset_;
  transport->expected_packet_.insert(transport->expected_packet_.end(), transport->buffer_,
                                     transport->buffer_ +
                                         (std::min)(bytes_expected, bytes_available));

  transport->offset_ = bytes_available - bytes_expected; // set offset to bytes of remain buffer
  if (transport->offset_ >= 0)
  {                             // pdu received properly
    if (transport->offset_ > 0) // move remain data to head of buffer and hold offset.
    {
      ::memmove(transport->buffer_, transport->buffer_ + bytes_expected, transport->offset_);
      // not all data consumed, so add events for this context
      max_wait_duration = 0;
    }
    // move properly pdu to ready queue, the other thread who care about will retrieve
    // it.
    YASIO_SLOGV("[index: %d] received a properly packet from peer, "
                "packet size:%d",
                transport->cindex(), transport->expected_size_);
    this->handle_event(event_ptr(
        new io_event(transport->cindex(), YEK_PACKET, transport->fetch_packet(), transport)));
  }
  else
  { // all buffer consumed, set offset to ZERO, pdu
    // incomplete, continue recv remain data.
    transport->offset_ = 0;
  }
}

deadline_timer_ptr io_service::schedule(const std::chrono::microseconds& duration, timer_cb_t cb)
{
  auto timer = std::make_shared<deadline_timer>(*this);
  timer->expires_from_now(duration);
  timer->async_wait(
      [timer /*!important, hold on by lambda expression */, cb](bool cancelled) { cb(cancelled); });
  return timer;
}

void io_service::schedule_timer(deadline_timer* timer_ctl, timer_cb_t&& timer_cb)
{
  // pitfall: this service only hold the weak pointer of the timer
  // object, so before dispose the timer object need call
  // cancel_timer to cancel it.
  if (timer_ctl == nullptr)
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  if (this->find_timer(timer_ctl) != timer_queue_.end())
    return;

  this->timer_queue_.emplace_back(timer_ctl, std::move(timer_cb));
  this->sort_timers();
  if (timer_ctl == this->timer_queue_.begin()->first)
    this->interrupt();
}

void io_service::remove_timer(deadline_timer* timer)
{
  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  auto iter = this->find_timer(timer);
  if (iter != timer_queue_.end())
    timer_queue_.erase(iter);
}

void io_service::open_internal(io_channel* ctx, bool ignore_state)
{
  if (ctx->state_ == YCS_OPENING && !ignore_state)
  { // in-opening, do nothing
    YASIO_SLOG("[index: %d] the channel is in opening!", ctx->index_);
    return;
  }

  close_internal(ctx);

  ctx->opmask_ |= YOPM_OPEN_CHANNEL;

  this->channel_ops_mtx_.lock();
  if (std::find(this->channel_ops_.begin(), this->channel_ops_.end(), ctx) ==
      this->channel_ops_.end())
    this->channel_ops_.push_back(ctx);
  this->channel_ops_mtx_.unlock();

  this->interrupt();
}

bool io_service::close_internal(io_channel* ctx)
{
  if (ctx->socket_->is_open())
  {
    if (ctx->mask_ & YCM_CLIENT)
    {
      ctx->opmask_ |= YOPM_CLOSE_TRANSPORT;
      if (ctx->mask_ & YCM_TCP)
        ctx->socket_->shutdown();
    }
    else
      ctx->opmask_ |= YOPM_CLOSE_CHANNEL;
    return true;
  }
  return false;
}

void io_service::perform_timers()
{
  if (this->timer_queue_.empty())
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  while (!this->timer_queue_.empty())
  {
    if (timer_queue_.back().first->expired())
    {
      auto earliest  = std::move(timer_queue_.back());
      auto timer_ctl = earliest.first;
      auto& timer_cb = earliest.second;
      timer_queue_.pop_back(); // pop the expired timer from timer queue
      timer_cb(timer_ctl->cancelled_);
    }
    else
      break;
  }
}

int io_service::do_evpoll(fd_set* fdsa, long long max_wait_duration)
{
  /*
@Optimize, swap nfds, make sure do_read & do_write event chould
be perform when no need to call socket.select However, the
connection exception will detected through do_read or do_write,
but it's ok.
*/
  int nfds = 1;

  ::memcpy(fdsa, this->fds_array_, sizeof(this->fds_array_));

  auto wait_duration = get_wait_duration(max_wait_duration);
  if (wait_duration > 0)
  {
    timeval timeout = {(decltype(timeval::tv_sec))(wait_duration / 1000000),
                       (decltype(timeval::tv_usec))(wait_duration % 1000000)};
    YASIO_SLOGV("socket.select maxfdp:%d waiting... %ld milliseconds", maxfdp_,
                timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
    nfds = ::select(this->maxfdp_, &(fdsa[read_op]), &(fdsa[write_op]), nullptr, &timeout);
    YASIO_SLOGV("socket.select waked up, retval=%d", nfds);
  }
  else
    nfds = static_cast<int>(channels_.size()) << 1;

  return nfds;
}

long long io_service::get_wait_duration(long long usec)
{
  if (this->timer_queue_.empty())
    return usec;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  auto earliest = timer_queue_.back().first;

  // microseconds
  auto duration = earliest->wait_duration();
  if (std::chrono::microseconds(usec) > duration)
    return duration.count();
  else
    return usec;
}

bool io_service::cleanup_io(io_base* ctx)
{
  ctx->opmask_ = 0;
  ctx->state_  = YCS_CLOSED;
  ctx->error_  = 0;
  if (ctx->socket_->is_open())
  {
    unregister_descriptor(ctx->socket_->native_handle(), YEM_POLLIN | YEM_POLLOUT);
    ctx->socket_->close();
    return true;
  }
  return false;
}

u_short io_service::query_ares_state(io_channel* ctx)
{
  if ((ctx->dns_queries_state_ & YDQSF_QUERIES_NEEDED) &&
      !YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_INPRROGRESS))
  {
    auto diff = (highp_clock() - ctx->dns_queries_timestamp_);
    if (YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_READY) &&
        diff >= options_.dns_cache_timeout_)
      YDQS_SET_STATE(ctx->dns_queries_state_, YDQS_DIRTY);

    if (YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_DIRTY))
      start_resolve(ctx);
  }

  return YDQS_GET_STATE(ctx->dns_queries_state_);
}

void io_service::start_resolve(io_channel* ctx)
{ // Only call at event-loop thread, so
  // no need to consider thread safe.
  assert(YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_DIRTY));
  ctx->set_last_errno(EINPROGRESS);
  YDQS_SET_STATE(ctx->dns_queries_state_, YDQS_INPRROGRESS);

  YASIO_SLOG("[index: %d] resolving domain name: %s", ctx->index_, ctx->remote_host_.c_str());

  ctx->remote_eps_.clear();
  std::thread resolv_thread([=] { // 6.563ms
    addrinfo hint;
    memset(&hint, 0x0, sizeof(hint));

    int error = options_.resolv_(ctx->remote_eps_, ctx->remote_host_.c_str(), ctx->remote_port_);
    if (error == 0)
    {
      ctx->dns_queries_state_     = YDQS_READY;
      ctx->dns_queries_timestamp_ = highp_clock();
    }
    else
    {
      ctx->set_last_errno(error);
      YDQS_SET_STATE(ctx->dns_queries_state_, YDQS_FAILED);
    }
    /*
    The getaddrinfo behavior at win32 is strange:
    If the channel 0 is in non-blocking connect, and waiting at select, than
    channel 1 request connect(need dns queries), it's wake up the select call,
    do resolve with getaddrinfo. After resolved, the channel 0 call FD_ISSET
    without select call, FD_ISSET will always return true, even through the
    TCP connection handshake is not complete.

    Try write data to a incomplete TCP will trigger error: 10057
    Another result at this situation is: Try get local endpoint by getsockname
    will return 0.0.0.0
    */
    this->interrupt();
  });
  resolv_thread.detach();
}

int io_service::__builtin_resolv(std::vector<ip::endpoint>& endpoints, const char* hostname,
                                 unsigned short port)
{
  if (this->ipsv_ & ipsv_ipv4)
    return xxsocket::resolve_v4(endpoints, hostname, port);
  else if (this->ipsv_ & ipsv_ipv6) // localhost is IPV6 ONLY network
    return xxsocket::resolve_v6(endpoints, hostname, port) != 0
               ? xxsocket::resolve_v4to6(endpoints, hostname, port)
               : 0;
  return -1;
}

void io_service::interrupt() { interrupter_.interrupt(); }

const char* io_service::strerror(int error)
{
  switch (error)
  {
    case 0:
      return "No error.";
    case YERR_DPL_ILLEGAL_PDU:
      return "Decode frame length failed!";
    case YERR_RESOLV_HOST_FAILED:
      return "Resolve host failed!";
    case YERR_NO_AVAIL_ADDR:
      return "No available address!";
    case YERR_LOCAL_SHUTDOWN:
      return "An existing connection was shutdown by local host!";
    case YERR_INVALID_PACKET:
      return "Invalid packet!";
    case -1:
      return "Unknown error!";
    default:
      return xxsocket::strerror(error);
  }
}

void io_service::set_option(int option, ...) // lgtm [cpp/poorly-documented-function]
{
  va_list ap;
  va_start(ap, option);

  switch (option)
  {
    case YOPT_S_TIMEOUTS: {
      options_.dns_cache_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      options_.connect_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      int value = va_arg(ap, int);
      if (value > 0)
        options_.reconnect_timeout_ = static_cast<highp_time_t>(value) * MICROSECONDS_PER_SECOND;
      else
        options_.reconnect_timeout_ = -1; // means auto reconnect is disabled.
      break;
    }
    case YOPT_S_DEFERRED_EVENT:
      options_.deferred_event_ = !!va_arg(ap, int);
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
    case YOPT_S_PRINT_FN:
      this->options_.print_ = *va_arg(ap, print_fn_t*);
      break;
    case YOPT_C_LFBFD_PARAMS: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->lfb_.max_frame_length    = va_arg(ap, int);
        channel->lfb_.length_field_offset = va_arg(ap, int);
        channel->lfb_.length_field_length = va_arg(ap, int);
        channel->lfb_.length_adjustment   = va_arg(ap, int);
      }
      break;
    }
    case YOPT_S_EVENT_CB:
      options_.on_event_ = *va_arg(ap, io_event_cb_t*);
      break;
    case YOPT_C_LFBFD_FN: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->decode_len_ = *va_arg(ap, decode_len_fn_t*);
    }
    break;
    case YOPT_C_LOCAL_HOST: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->local_host_ = va_arg(ap, const char*);
      break;
    }
    case YOPT_C_LOCAL_PORT: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->local_port_ = (u_short)va_arg(ap, int);
      break;
    }
    case YOPT_C_REMOTE_HOST: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->setup_remote_host(va_arg(ap, const char*));
    }
    break;
    case YOPT_C_REMOTE_PORT: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
        channel->setup_remote_port((u_short)va_arg(ap, int));
    }
    break;
    case YOPT_C_LOCAL_ENDPOINT: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel != nullptr)
      {
        channel->local_host_ = (va_arg(ap, const char*));
        channel->local_port_ = ((u_short)va_arg(ap, int));
      }
    }
    break;
    case YOPT_C_REMOTE_ENDPOINT: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->setup_remote_host(va_arg(ap, const char*));
        channel->setup_remote_port((u_short)va_arg(ap, int));
      }
    }
    case YOPT_C_MOD_FLAGS: {
      auto channel = cindex_to_handle(static_cast<size_t>(va_arg(ap, int)));
      if (channel)
      {
        channel->flags_ |= (u_short)va_arg(ap, int);
        channel->flags_ &= ~(u_short)va_arg(ap, int);
      }
      break;
    }
    case YOPT_S_NO_NEW_THREAD:
      this->options_.no_new_thread_ = !!va_arg(ap, int);
      break;
    case YOPT_I_SOCKOPT: {
      auto obj = va_arg(ap, io_base*);
      if (obj && obj->socket_)
      {
        auto optlevel = va_arg(ap, int);
        auto optname  = va_arg(ap, int);
        auto optval   = va_arg(ap, void*);
        auto optlen   = va_arg(ap, int);
        obj->socket_->set_optval(optlevel, optname, optval, optlen);
      }
    }
    break;
  }

  va_end(ap);
}
} // namespace inet
} // namespace yasio

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif

#endif
