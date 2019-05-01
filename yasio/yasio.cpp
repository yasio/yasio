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

// UDP server: https://cloud.tencent.com/developer/article/1004555
#include "yasio.h"
#include <limits>
#include <stdarg.h>
#include <string>
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

#define _YASIO_VERBOS_LOG 0
#define YASIO_SOMAXCONN 19
#define MAX_WAIT_DURATION 5 * 60 * 1000 * 1000 // 5 minites
/* max pdu buffer length, avoid large memory allocation when application layer decode a huge length
 * field. */
#define MAX_PDU_BUFFER_SIZE static_cast<int>(SZ(1, M))

#if defined(_WIN32)
#  define YASIO_DEBUG_PRINT(msg) OutputDebugStringA(msg)
#  pragma warning(push)
#  pragma warning(disable : 6320 6322 4996)
#elif defined(ANDROID) || defined(__ANDROID__)
#  include <android/log.h>
#  include <jni.h>
#  define YASIO_DEBUG_PRINT(msg) __android_log_print(ANDROID_LOG_INFO, "yasio", "%s", msg)
#else
#  define YASIO_DEBUG_PRINT(msg) printf("%s", msg)
#endif

#define YASIO_LOG(format, ...)                                                                     \
  do                                                                                               \
  {                                                                                                \
    auto content =                                                                                 \
        _sfmt(("[yasio][%lld] " format "\r\n"), highp_clock<system_clock_t>(), ##__VA_ARGS__);     \
    YASIO_DEBUG_PRINT(content.c_str());                                                            \
    if (options_.outf_ != -1)                                                                      \
    {                                                                                              \
      if (::lseek(options_.outf_, 0, SEEK_CUR) > options_.outf_max_size_)                          \
        ::ftruncate(options_.outf_, 0), ::lseek(options_.outf_, 0, SEEK_SET);                      \
      ::write(options_.outf_, content.c_str(), static_cast<int>(content.size()));                  \
    }                                                                                              \
  } while (false)

namespace yasio
{
namespace inet
{
namespace
{
// error code
enum
{
  YERR_OK,                  // NO ERROR.
  YERR_SEND_TIMEOUT = -500, // Send timeout.
  YERR_DPL_ILLEGAL_PDU,     // Decode pdu length error.
  YERR_RESOLV_HOST_FAILED,  // Resolve host failed.
  YERR_NO_AVAIL_ADDR,       // No available address to connect.
  YERR_LOCAL_SHUTDOWN,      // Local shutdown the connection.
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

/*--- This is a C++ universal sprintf in the future.
 **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is
 *different
 **      VS2013 or Unix-Like System will return -1 when buffer not enough, but
 *VS2015/2017 will return the actural needed length for buffer at this station
 **      The _vsnprintf behavior is compatible API which always return -1 when
 *buffer isn't enough at VS2013/2015/2017
 **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later,
 *AND it's also standard-compliant, see reference:
 *http://www.cplusplus.com/reference/cstdio/vsnprintf/
 */
static std::string _sfmt(const char *format, ...)
{
#define YASIO_VSNPRINTF_BUFFER_LENGTH 256
  va_list args;
  std::string buffer(YASIO_VSNPRINTF_BUFFER_LENGTH, '\0');

  va_start(args, format);
  int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
  va_end(args);

  if (nret >= 0)
  {
    if ((unsigned int)nret < buffer.length())
    {
      buffer.resize(nret);
    }
    else if ((unsigned int)nret > buffer.length())
    { // VS2015/2017 or later Visual Studio Version
      buffer.resize(nret);

      va_start(args, format);
      nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
      va_end(args);
    }
    // else equals, do nothing.
  }
  else
  { // less or equal VS2013 and Unix System glibc implement.
    do
    {
      buffer.resize(buffer.length() * 3 / 2);

      va_start(args, format);
      nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
      va_end(args);

    } while (nret < 0);

    buffer.resize(nret);
  }

  return buffer;
}

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
static void _set_thread_name(const char *threadName)
{
  THREADNAME_INFO info;
  info.dwType     = 0x1000;
  info.szName     = threadName;
  info.dwThreadID = GetCurrentThreadId(); // dwThreadID;
  info.dwFlags    = 0;
#  if !defined(__MINGW64__) && !defined(__MINGW32__)
  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
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
  a_pdu(std::vector<char> &&right, const std::chrono::microseconds &duration)
  {
    data_        = std::move(right);
    offset_      = 0;
    expire_time_ = highp_clock_t::now() + duration;
  }
  bool expired() const { return (expire_time_ - highp_clock_t::now()).count() < 0; }
  std::vector<char> data_; // sending data
  size_t offset_;          // offset
  std::chrono::time_point<highp_clock_t> expire_time_;

#if _USING_OBJECT_POOL
  DEFINE_CONCURRENT_OBJECT_POOL_ALLOCATION(a_pdu, 512)
#endif
};

/// deadline_timer
void deadline_timer::async_wait(timer_cb_t cb) { this->service_.schedule_timer(this, cb); }

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
io_channel::io_channel(io_service &service) : deadline_timer_(service)
{
  socket_.reset(new xxsocket());
  state_             = YCS_CLOSED;
  dns_queries_state_ = YDQS_FAILED;
}

io_transport::io_transport(io_channel *ctx) : ctx_(ctx)
{
  static unsigned int s_object_id = 0;
  this->state_                    = YCS_OPENED;
  this->id_                       = ++s_object_id;
}

io_service::io_service() : state_(io_service::state::IDLE), interrupter_()
{
  FD_ZERO(&fds_array_[read_op]);
  FD_ZERO(&fds_array_[write_op]);
  FD_ZERO(&fds_array_[except_op]);

  maxfdp_           = 0;
  outstanding_work_ = 0;

  this->decode_len_ = [=](void *ptr, int len) { return this->__builtin_decode_len(ptr, len); };

  this->resolv_ = [=](std::vector<ip::endpoint> &eps, const char *host, unsigned short port) {
    return this->__builtin_resolv(eps, host, port);
  };
}

io_service::~io_service()
{
  stop_service();
  if (this->state_ == io_service::state::STOPPED)
    cleanup();

  for (auto t : transports_dypool_)
    ::operator delete(t);
  transports_dypool_.clear();

  if (options_.outf_ != -1)
    ::close(options_.outf_);
  this->decode_len_ = nullptr;
  this->resolv_     = nullptr;
}

void io_service::start_service(const io_hostent *channel_eps, int channel_count, io_event_cb_t cb)
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

void io_service::init(const io_hostent *channel_eps, int channel_count, io_event_cb_t &cb)
{
  if (this->state_ != io_service::state::IDLE)
    return;
  if (channel_count <= 0)
    return;
  if (cb)
    this->on_event_ = std::move(cb);

  register_descriptor(interrupter_.read_descriptor(), YEM_POLLIN);

  // Initialize channels
  for (auto i = 0; i < channel_count; ++i)
  {
    auto &channel_ep = channel_eps[i];
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
    this->event_queue_.clear();
    this->timer_queue_.clear();
    this->outstanding_work_ = 0;

    unregister_descriptor(interrupter_.read_descriptor(), YEM_POLLIN);

    this->on_event_ = nullptr;

    this->state_ = io_service::state::IDLE;
  }
}

io_channel *io_service::new_channel(const io_hostent &ep)
{
  auto ctx    = new io_channel(*this);
  ctx->host_  = ep.host_;
  ctx->port_  = ep.port_;
  ctx->index_ = static_cast<int>(this->channels_.size());
  update_dns_queries_state(ctx, true);
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
    this->transports_dypool_.push_back(transport);
  }
  transports_.clear();
}

void io_service::dispatch_events(int count)
{
  if (!this->on_event_ || this->event_queue_.empty())
    return;

  std::lock_guard<std::recursive_mutex> lck(this->event_queue_mtx_);
  do
  {
    auto event = std::move(this->event_queue_.front());
    this->event_queue_.pop_front();
    this->on_event_(std::move(event));
  } while (!this->event_queue_.empty() && --count > 0);
}

void io_service::run()
{
  _set_thread_name("yasio-evloop");

  // Call once at startup
  this->ipsv_ = xxsocket::getipsv();

  // event loop
  fd_set fds_array[max_ops];
  for (; this->state_ == io_service::state::RUNNING;)
  {
    int nfds = do_evpoll(fds_array);
    if (this->state_ != io_service::state::RUNNING)
      break;

    if (nfds == -1)
    {
      int ec = xxsocket::get_last_errno();
      YASIO_LOG("do_evpoll failed, ec:%d, detail:%s\n", ec, io_service::strerror(ec));
      if (ec == EBADF)
        goto _L_end;
      continue; // just continue.
    }

    if (nfds == 0)
    {
#if _YASIO_VERBOS_LOG
      YASIO_LOG("%s", "do_evpoll is timeout, do perform_timeout_timers()");
#endif
    }
    // Reset the interrupter.
    else if (nfds > 0 && FD_ISSET(this->interrupter_.read_descriptor(), &(fds_array[read_op])))
    {
#if _YASIO_VERBOS_LOG
      bool was_interrupt = interrupter_.reset();
      YASIO_LOG("do_evpoll waked up by interrupt, interrupter fd:%d, "
                "was_interrupt:%s",
                this->interrupter_.read_descriptor(), was_interrupt ? "true" : "false");
#else
      interrupter_.reset();
#endif
      --nfds;
    }

    // perform active transports
    perform_transports(fds_array);

    // perform active channels
    perform_channels(fds_array);

    // perform timeout timers
    perform_timers();
  }

_L_end:
  (void)0; // ONLY for xcode compiler happy.
}

void io_service::perform_transports(fd_set *fds_array)
{
  // preform transports
  for (auto iter = transports_.begin(); iter != transports_.end();)
  {
    auto transport = *iter;
    if (do_read(transport, fds_array) && do_write(transport))
      ++iter;
    else
    {
      handle_close(transport);
      iter = transports_.erase(iter);
    }
  }
}

void io_service::perform_channels(fd_set *fds_array)
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
          switch (this->update_dns_queries_state(ctx, false))
          {
            case YDQS_READY:
              do_nonblocking_connect(ctx);
              break;
            case YDQS_FAILED:
              YASIO_LOG("[index: %d] getaddrinfo failed, ec:%d, detail:%s", ctx->index_,
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
        if (ctx->opmask_ & YOPM_CLOSE_CHANNEL)
          cleanup_io(ctx);

        if (ctx->opmask_ & YOPM_OPEN_CHANNEL)
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

void io_service::close(size_t channel_index)
{
  // Gets channel context
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  if (!(ctx->opmask_ & YOPM_CLOSE_CHANNEL))
  {
    if (close_internal(ctx))
      this->interrupt();
  }
}

void io_service::close(transport_ptr transport)
{
  if (transport->is_open() && !(transport->opmask_ & YOPM_CLOSE_TRANSPORT))
  {
    transport->opmask_ |= YOPM_CLOSE_TRANSPORT;
    if (transport->ctx_->mask_ & YCM_TCP)
      transport->socket_->shutdown();
    this->interrupt();
  }
}

bool io_service::is_open(size_t channel_index) const
{
  // Gets channel
  if (channel_index >= channels_.size())
    return false;
  auto ctx = channels_[channel_index];
  return ctx->state_ == YCS_OPENED;
}

bool io_service::is_open(transport_ptr transport) const { return transport->state_ == YCS_OPENED; }

void io_service::reopen(transport_ptr transport)
{
  auto ctx = transport->ctx_;
  if (ctx->mask_ & YCM_CLIENT) // Only client channel support reopen by transport
    open_internal(ctx);
}

void io_service::open(size_t channel_index, int channel_mask)
{
#if defined(_WIN32)
  if (channel_mask == YCM_UDP_SERVER)
  {
    /*
    Because Bind() the client socket to the socket address of the listening socket.  On Linux this
    essentially passes the responsibility for receiving data for the client session from the
    well-known listening socket, to the newly allocated client socket.  It is important to note
    that this behavior is not the same on other platforms, like Windows (unfortunately), detail
    see:
    https://blog.grijjy.com/2018/08/29/creating-high-performance-udp-servers-on-windows-and-linux
  */
    YASIO_LOG(
        "[index: %d], YCM_UDP_SERVER does'n supported by Microsoft Winsock provider, you can use "
        "YCM_UDP_CLIENT to communicate with peer!",
        channel_index);
    return;
  }
#endif

  // Gets channel
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  ctx->mask_ = channel_mask;
  if (channel_mask & YCM_TCP)
    ctx->protocol_ = SOCK_STREAM;
  else if (channel_mask & YCM_UDP)
    ctx->protocol_ = SOCK_DGRAM;

  open_internal(ctx);
}

void io_service::handle_close(transport_ptr transport)
{
  auto ptr = transport;
  auto ctx = ptr->ctx_;
  auto ec  = ptr->error_;
  // @Because we can't retrive peer endpoint when connect reset by peer, so use id to trace.
  YASIO_LOG("[index: %d] the connection #%u is lost, offset:%d, ec:%d, detail:%s", ctx->index_,
            ptr->id_, ptr->offset_, ec, io_service::strerror(ec));

  cleanup_io(ptr);

  // @Update state
  if (ctx->mask_ & YCM_CLIENT)
  {
    ctx->state_ = YCS_CLOSED;
    ctx->opmask_ &= ~YOPM_CLOSE_TRANSPORT;
    ctx->error_ = 0;
  } // server channel, do nothing.

  ptr->~io_transport();
  transports_dypool_.push_back(ptr);

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
int io_service::write(transport_ptr transport, std::vector<char> data)
{
  if (transport && transport->is_open())
  {
    a_pdu_ptr pdu(new a_pdu(std::move(data), std::chrono::microseconds(options_.send_timeout_)));

    transport->send_queue_mtx_.lock();
    transport->send_queue_.push_back(pdu);
    transport->send_queue_mtx_.unlock();

    this->interrupt();

    return data.size();
  }
  else
  {
    YASIO_LOG("[transport: %p] send failed, the connection not ok!", transport);
    return -1;
  }
}
void io_service::handle_event(event_ptr event)
{
  if (options_.deferred_event_)
  {
    event_queue_mtx_.lock();
    event_queue_.push_back(std::move(event));
    event_queue_mtx_.unlock();
  }
  else
    this->on_event_(std::move(event));
}

void io_service::do_nonblocking_connect(io_channel *ctx)
{
  assert(YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_READY));
  if (this->ipsv_ == 0)
    this->ipsv_ = xxsocket::getipsv();
  if (ctx->socket_->is_open())
    cleanup_io(ctx);

  ctx->opmask_ &= ~YOPM_OPEN_CHANNEL;

  if (ctx->endpoints_.empty())
  {
    this->handle_connect_failed(ctx, YERR_NO_AVAIL_ADDR);
    return;
  }

  ctx->state_ = YCS_OPENING;
  auto &ep    = ctx->endpoints_[0];
  YASIO_LOG("[index: %d] connecting server %s:%u...", ctx->index_, ctx->host_.c_str(), ctx->port_);
  int ret = -1;
  if (ctx->socket_->open(ep.af(), ctx->protocol_))
  {
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);
    if (ctx->local_port_ != 0 || ctx->mask_ & YCM_UDP)
      ctx->socket_->bind("0.0.0.0", ctx->local_port_);
    ret = xxsocket::connect_n(ctx->socket_->native_handle(), ep);
    if (ret < 0)
    { // setup no blocking connect
      int error = xxsocket::get_last_errno();
      if (error != EINPROGRESS && error != EWOULDBLOCK)
      {
        this->handle_connect_failed(ctx, error);
      }
      else
      {
        ctx->update_error(EINPROGRESS);
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

void io_service::do_nonblocking_connect_completion(io_channel *ctx, fd_set *fds_array)
{
  assert(ctx->mask_ == YCM_TCP_CLIENT);
  assert(ctx->state_ == YCS_OPENING);

  int error = -1;
  if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[write_op]) ||
      FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
  {
    socklen_t len = sizeof(error);
    if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
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

void io_service::do_nonblocking_accept(io_channel *ctx)
{ // channel is server
  cleanup_io(ctx);

  ip::endpoint ep(ipsv_ & ipsv_ipv4 ? "0.0.0.0" : "::", ctx->port_);

  if (ctx->socket_->open(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6, ctx->protocol_))
  {
    int error = 0;
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);
    if (ctx->socket_->bind(ep) != 0)
    {
      error = xxsocket::get_last_errno();
      YASIO_LOG("[index: %d] bind failed, ec:%d, detail:%s", ctx->index_, error,
                io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = YCS_CLOSED;
      return;
    }

    if ((ctx->mask_ & YCM_UDP) || ctx->socket_->listen(YASIO_SOMAXCONN) == 0)
    {
      ctx->state_ = YCS_OPENED;
      ctx->socket_->set_nonblocking(true);
      register_descriptor(ctx->socket_->native_handle(), YEM_POLLIN);
      YASIO_LOG("[index: %d] listening at %s...", ctx->index_, ep.to_string().c_str());
    }
    else
    {
      error = xxsocket::get_last_errno();
      YASIO_LOG("[index: %d] listening failed, ec:%d, detail:%s", ctx->index_, error,
                io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = YCS_CLOSED;
    }
  }
}

void io_service::do_nonblocking_accept_completion(io_channel *ctx, fd_set *fds_array)
{
  if (ctx->state_ == YCS_OPENED)
  {
    int error = -1;
    if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
    {
      socklen_t len = sizeof(error);
      if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
              0 &&
          error == 0)
      {
        if (ctx->mask_ & YCM_TCP)
        {
          std::shared_ptr<xxsocket> client_sock(new xxsocket(ctx->socket_->accept()));
          if (client_sock->is_open())
          {
            handle_connect_succeed(ctx, std::move(client_sock));
          }
          else
            YASIO_LOG("%s", "tcp-server: accept client socket fd failed!");
        }
        else // YCM_UDP
        {
          ip::endpoint peer;

          char buffer[65535];
          int n = ctx->socket_->recvfrom_i(buffer, sizeof(buffer), peer);
          if (n > 0)
          {
            YASIO_LOG("udp-server: recvfrom peer: %s", peer.to_string().c_str());

            // make a transport local --> peer udp session, just like tcp accept
            std::shared_ptr<xxsocket> client_sock(new xxsocket());
            if (client_sock->open(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0))
            {
              client_sock->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);
              error = client_sock->bind("0.0.0.0", ctx->port_) == 0
                          ? xxsocket::connect(client_sock->native_handle(), peer)
                          : -1;
              if (error == 0)
              {
                auto transport = allocate_transport(ctx, std::move(client_sock));
                handle_connect_succeed(transport);
                this->handle_event(
                    event_ptr(new io_event(transport->channel_index(), YEK_PACKET,
                                           std::vector<char>(buffer, buffer + n), transport)));
              }
              else
                YASIO_LOG("%s", "udp-server: open socket fd failed!");
            }
          }
        }
      }
      else
      {
        YASIO_LOG("The channel:%d has socket ec:%d, will be closed!", ctx->index_, error);
        cleanup_io(ctx);
      }
    }
  }
}

void io_service::handle_connect_succeed(transport_ptr transport)
{
  auto ctx         = transport->ctx_;
  ctx->error_      = 0;
  auto &connection = transport->socket_;
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
      connection->set_keepalive(options_.tcp_keepalive_.idle, options_.tcp_keepalive_.interval,
                                options_.tcp_keepalive_.probs);
  }

  YASIO_LOG("[index: %d] the connection #%u [%s] --> [%s] is established.", ctx->index_,
            transport->id_, connection->local_endpoint().to_string().c_str(),
            connection->peer_endpoint().to_string().c_str());
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, 0, transport)));
}

transport_ptr io_service::allocate_transport(io_channel *ctx, std::shared_ptr<xxsocket> socket)
{
  transport_ptr transport;
  if (!transports_dypool_.empty())
  { // allocate from pool
    auto reuse_ptr = transports_dypool_.back();

    // construct it since we don't delete transport memory
    transport = new ((void *)reuse_ptr) io_transport(ctx);
    transports_dypool_.pop_back();
  }
  else
    transport = new io_transport(ctx);

  this->transports_.push_back(transport);
  transport->socket_ = socket;

  return transport;
}

void io_service::handle_connect_failed(io_channel *ctx, int error)
{
  cleanup_io(ctx);

  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, error, nullptr)));

  YASIO_LOG("[index: %d] connect server %s:%u failed, ec:%d, detail:%s", ctx->index_,
            ctx->host_.c_str(), ctx->port_, error, io_service::strerror(error));
}

bool io_service::do_write(transport_ptr transport)
{
  bool ret = false;
  do
  {
    if (!transport->socket_->is_open())
      break;

    if (!transport->send_queue_.empty())
    {
      std::lock_guard<std::recursive_mutex> lck(transport->send_queue_mtx_);

      auto v                 = transport->send_queue_.front();
      auto outstanding_bytes = static_cast<int>(v->data_.size() - v->offset_);
      int n = transport->socket_->send_i(v->data_.data() + v->offset_, outstanding_bytes);
      if (n == outstanding_bytes)
      { // All pdu bytes sent.
        transport->send_queue_.pop_front();
#if _YASIO_VERBOS_LOG
        auto packet_size = static_cast<int>(v->data_.size());
        YASIO_LOG("[index: %d] do_write ok, A packet sent "
                  "success, packet size:%d",
                  transport->channel_index(), packet_size,
                  transport->socket_->local_endpoint().to_string().c_str(),
                  transport->socket_->peer_endpoint().to_string().c_str());
#endif
        handle_send_finished(v, YERR_OK);

        if (!transport->send_queue_.empty())
          ++this->outstanding_work_;
      }
      else if (n > 0)
      { // TODO: add time
        if (!v->expired())
        { // #performance: change offset only, remain data will be send next time.
          v->offset_ += n;
          outstanding_bytes = static_cast<int>(v->data_.size() - v->offset_);
          YASIO_LOG("[index: %d] do_write pending, %dbytes still "
                    "outstanding, "
                    "%dbytes was sent!",
                    transport->channel_index(), outstanding_bytes, n);

          ++this->outstanding_work_;
        }
        else
        { // send timeout
          transport->send_queue_.pop_front();

          auto packet_size = static_cast<int>(v->data_.size());
          YASIO_LOG("[index: %d] do_write packet timeout, packet "
                    "size:%d",
                    transport->channel_index(), packet_size);
          handle_send_finished(v, YERR_SEND_TIMEOUT);
          if (!transport->send_queue_.empty())
            ++this->outstanding_work_;
        }
      }
      else
      { // n <= 0, TODO: add time
        int error = transport->update_error();
        if (SHOULD_CLOSE_1(n, error))
        {
          transport->offset_ = n;
          break;
        }
      }
    }

    ret = true;
  } while (false);

  return ret;
}

void io_service::handle_send_finished(a_pdu_ptr /*pdu*/, int /*error*/) {}

bool io_service::do_read(transport_ptr transport, fd_set *fds_array)
{
  bool ret = false;
  do
  {
    if (!transport->socket_->is_open())
      break;
    if ((transport->opmask_ | transport->ctx_->opmask_) & YOPM_CLOSE_TRANSPORT)
    {
      transport->update_error(YERR_LOCAL_SHUTDOWN);
      break;
    }

    int n = -1;
    if (FD_ISSET(transport->socket_->native_handle(), &(fds_array[read_op])))
    {
      n = transport->socket_->recv_i(transport->buffer_ + transport->offset_,
                                     sizeof(transport->buffer_) - transport->offset_);
      transport->update_error();
    }
    else
      transport->update_error(EWOULDBLOCK);

    if (n > 0 || !SHOULD_CLOSE_0(n, transport->error_))
    {
#if _YASIO_VERBOS_LOG
      YASIO_LOG("[index: %d] do_read status ok, ec:%d, detail:%s", transport->channel_index(),
                transport->error_, io_service::strerror(transport->error_));
#endif
      if (n == -1)
        n = 0;
#if _YASIO_VERBOS_LOG
      if (n > 0)
      {
        YASIO_LOG("[index: %d] do_read ok, received data len: %d, "
                  "buffer data "
                  "len: %d",
                  transport->channel_index(), n, n + transport->offset_);
      }
#endif
      if (transport->expected_packet_size_ == -1)
      { // decode length
        int length = this->decode_len_(transport->buffer_, transport->offset_ + n);
        if (length > 0)
        {
          transport->expected_packet_size_ = length;
          transport->expected_packet_.reserve(
              (std::min)(transport->expected_packet_size_,
                         MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid memory reallocte.
          do_unpack(transport, transport->expected_packet_size_, n);
        }
        else if (length == 0)
        {
          // header insufficient, wait readfd ready at
          // next event step.
          transport->offset_ += n;
        }
        else
        {
          transport->update_error(YERR_DPL_ILLEGAL_PDU);
          break;
        }
      }
      else
      { // process incompleted pdu
        do_unpack(transport,
                  transport->expected_packet_size_ -
                      static_cast<int>(transport->expected_packet_.size()),
                  n);
      }
    }
    else
    { // n == 0: The return value will be 0 when the peer has performed an orderly shutdown.
      transport->offset_ = n;
      break;
    }

    ret = true;

  } while (false);

  return ret;
}

void io_service::do_unpack(transport_ptr transport, int bytes_expected, int bytes_transferred)
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
      ++this->outstanding_work_;
    }
    // move properly pdu to ready queue, the other thread who care about will retrieve
    // it.
#if _YASIO_VERBOS_LOG
    YASIO_LOG("[index: %d] received a properly packet from peer, "
              "packet size:%d",
              transport->channel_index(), transport->expected_packet_size_);
#endif
    this->handle_event(event_ptr(
        new io_event(transport->channel_index(), YEK_PACKET, transport->take_packet(), transport)));
  }
  else
  { // all buffer consumed, set offset to ZERO, pdu
    // incomplete, continue recv remain data.
    transport->offset_ = 0;
  }
}

std::shared_ptr<deadline_timer> io_service::schedule(highp_time_t duration, timer_cb_t cb,
                                                     bool repeated)
{
  std::shared_ptr<deadline_timer> timer(new deadline_timer(*this));
  timer->expires_from_now(std::chrono::microseconds(duration), repeated);
  timer->async_wait(
      [timer /*!important, hold on by lambda expression */, cb](bool cancelled) { cb(cancelled); });
  return timer;
}

void io_service::schedule_timer(deadline_timer *timer_ctl, timer_cb_t &timer_cb)
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
  this->sort_timers_unlocked();
  if (timer_ctl == this->timer_queue_.begin()->first)
    this->interrupt();
}

void io_service::remove_timer(deadline_timer *timer)
{
  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  auto iter = this->find_timer(timer);
  if (iter != timer_queue_.end())
    timer_queue_.erase(iter);
}

void io_service::open_internal(io_channel *ctx, bool ignore_state)
{
  if (ctx->state_ == YCS_OPENING && !ignore_state)
  { // in-opening, do nothing
    YASIO_LOG("[index: %d] the channel is in opening!", ctx->index_);
    return;
  }

  close_internal(ctx);

  ctx->opmask_ |= YOPM_OPEN_CHANNEL;

  this->channel_ops_mtx_.lock();
  this->channel_ops_.push_back(ctx);
  this->channel_ops_mtx_.unlock();

  this->interrupt();
}

bool io_service::close_internal(io_channel *ctx)
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

  std::vector<timer_impl_t> loop_timers;
  while (!this->timer_queue_.empty())
  {
    if (timer_queue_.back().first->expired())
    {
      auto earliest  = std::move(timer_queue_.back());
      auto timer_ctl = earliest.first;
      auto &timer_cb = earliest.second;
      timer_queue_.pop_back(); // pop the expired timer from timer queue

      timer_cb(timer_ctl->cancelled_);
      if (timer_ctl->repeated_)
      {
        timer_ctl->expires_from_now();
        loop_timers.push_back(earliest);
      }
    }
    else
      break;
  }

  if (!loop_timers.empty())
  {
    this->timer_queue_.insert(this->timer_queue_.end(), loop_timers.begin(), loop_timers.end());
    this->sort_timers_unlocked();
  }
}

int io_service::do_evpoll(fd_set *fds_array)
{
  /*
@Optimize, swap nfds, make sure do_read & do_write event chould
be perform when no need to call socket.select However, the
connection exception will detected through do_read or do_write,
but it's ok.
*/
  int nfds = 0;
  std::swap(nfds, this->outstanding_work_);

  ::memcpy(fds_array, this->fds_array_, sizeof(this->fds_array_));
  if (nfds <= 0)
  {
    auto wait_duration = get_wait_duration(MAX_WAIT_DURATION);
    if (wait_duration > 0)
    {
      timeval timeout = {(decltype(timeval::tv_sec))(wait_duration / 1000000),
                         (decltype(timeval::tv_usec))(wait_duration % 1000000)};
#if _YASIO_VERBOS_LOG
      YASIO_LOG("socket.select maxfdp:%d waiting... %ld milliseconds", maxfdp_,
                timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#endif

      nfds =
          ::select(this->maxfdp_, &(fds_array[read_op]), &(fds_array[write_op]), nullptr, &timeout);

#if _YASIO_VERBOS_LOG
      YASIO_LOG("socket.select waked up, retval=%d", nfds);
#endif
    }
    else
      nfds = static_cast<int>(channels_.size()) << 1;
  }

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

bool io_service::cleanup_io(io_base *ctx)
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

u_short io_service::update_dns_queries_state(io_channel *ctx, bool update_name)
{
  if (!update_name)
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
  }
  else
  {
    ctx->endpoints_.clear();
    if (ctx->port_ > 0)
    {
      ip::endpoint ep;
      if (ep.assign(ctx->host_.c_str(), ctx->port_))
      {
        ctx->endpoints_.push_back(ep);
        ctx->dns_queries_state_ = YDQS_READY;
      }
      else
        ctx->dns_queries_state_ = YDQSF_QUERIES_NEEDED | YDQS_DIRTY;
    }
    else
      ctx->dns_queries_state_ = YDQS_FAILED;
  }

  return YDQS_GET_STATE(ctx->dns_queries_state_);
}

void io_service::start_resolve(io_channel *ctx)
{ // Only call at event-loop thread, so
  // no need to consider thread safe.
  assert(YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_DIRTY));
  ctx->error_ = EINPROGRESS;
  YDQS_SET_STATE(ctx->dns_queries_state_, YDQS_INPRROGRESS);

  YASIO_LOG("[index: %d] resolving domain name: %s", ctx->index_, ctx->host_.c_str());

  ctx->endpoints_.clear();
  std::thread resolv_thread([=] { // 6.563ms
    addrinfo hint;
    memset(&hint, 0x0, sizeof(hint));

    int error = this->resolv_(ctx->endpoints_, ctx->host_.c_str(), ctx->port_);
    if (error == 0)
    {
      ctx->dns_queries_state_     = YDQS_READY;
      ctx->dns_queries_timestamp_ = highp_clock();
    }
    else
    {
      ctx->error_ = error;
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

int io_service::__builtin_resolv(std::vector<ip::endpoint> &endpoints, const char *hostname,
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

int io_service::__builtin_decode_len(void *ud, int n)
{
  if (options_.lfb_.length_field_offset >= 0)
  {
    if (n >= (options_.lfb_.length_field_offset + options_.lfb_.length_field_length))
    {
      int32_t length = -1;
      switch (options_.lfb_.length_field_length)
      {
        case 4:
          length = ntohl(*reinterpret_cast<int32_t *>((unsigned char *)ud +
                                                      options_.lfb_.length_field_offset)) +
                   options_.lfb_.length_adjustment;
          break;
        case 3:
          length = 0;
          memcpy(&length, (unsigned char *)ud + options_.lfb_.length_field_offset, 3);
          length = (ntohl(length) >> 8) + options_.lfb_.length_adjustment;
          break;
        case 2:
          length = ntohs(*reinterpret_cast<uint16_t *>((unsigned char *)ud +
                                                       options_.lfb_.length_field_offset)) +
                   options_.lfb_.length_adjustment;
          break;
        case 1:
          length = *((unsigned char *)ud + options_.lfb_.length_field_offset) +
                   options_.lfb_.length_adjustment;
          break;
      }
      if (length > options_.lfb_.max_frame_length)
        length = -1;
      return length;
    }
    return 0;
  }
  return n;
}

void io_service::interrupt() { interrupter_.interrupt(); }

const char *io_service::strerror(int error)
{
  switch (error)
  {
    case -1:
      return "Unknown error!";
    case YERR_SEND_TIMEOUT:
      return "Send timeout!";
    case YERR_DPL_ILLEGAL_PDU:
      return "Decode frame length failed!";
    case YERR_RESOLV_HOST_FAILED:
      return "Resolve host failed!";
    case YERR_NO_AVAIL_ADDR:
      return "No available address!";
    case YERR_LOCAL_SHUTDOWN:
      return "An existing connection was shutdown by local host!";
    default:
      return xxsocket::strerror(error);
  }
}

void io_service::set_option(int option, ...)
{
  va_list ap;
  va_start(ap, option);

  switch (option)
  {
    case YOPT_CONNECT_TIMEOUT:
      options_.connect_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YOPT_SEND_TIMEOUT:
      options_.send_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YOPT_RECONNECT_TIMEOUT:
    {
      int value = va_arg(ap, int);
      if (value > 0)
        options_.reconnect_timeout_ = static_cast<highp_time_t>(value) * MICROSECONDS_PER_SECOND;
      else
        options_.reconnect_timeout_ = -1; // means auto reconnect is disabled.
    }
    break;
    case YOPT_DNS_CACHE_TIMEOUT:
      options_.dns_cache_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YOPT_DEFER_EVENT:
      options_.deferred_event_ = !!va_arg(ap, int);
      break;
    case YOPT_TCP_KEEPALIVE:
      options_.tcp_keepalive_.onoff    = 1;
      options_.tcp_keepalive_.idle     = va_arg(ap, int);
      options_.tcp_keepalive_.interval = va_arg(ap, int);
      options_.tcp_keepalive_.probs    = va_arg(ap, int);
      break;
    case YOPT_RESOLV_FUNCTION:
      this->resolv_ = std::move(*va_arg(ap, resolv_fn_t *));
      break;
    case YOPT_LOG_FILE:
      if (options_.outf_ != -1)
        ::close(options_.outf_);
      options_.outf_ = ::open(va_arg(ap, const char *), YASIO_O_OPEN_FLAGS);
      if (options_.outf_ != -1)
        ::lseek(options_.outf_, 0, SEEK_END);
      break;
    case YOPT_LFBFD_PARAMS:
      options_.lfb_.max_frame_length    = va_arg(ap, int);
      options_.lfb_.length_field_offset = va_arg(ap, int);
      options_.lfb_.length_field_length = va_arg(ap, int);
      options_.lfb_.length_adjustment   = va_arg(ap, int);
      break;
    case YOPT_IO_EVENT_CALLBACK:
      this->on_event_ = std::move(*va_arg(ap, io_event_cb_t *));
      break;
    case YOPT_DECODE_FRAME_LENGTH_FUNCTION:
      this->decode_len_ = std::move(*va_arg(ap, decode_len_fn_t *));
      break;
    case YOPT_CHANNEL_LOCAL_PORT:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
        this->channels_[index]->local_port_ = (u_short)va_arg(ap, int);
    }
    break;
    case YOPT_CHANNEL_REMOTE_HOST:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        this->channels_[index]->host_ = va_arg(ap, const char *);
        update_dns_queries_state(this->channels_[index], true);
      }
    }
    break;
    case YOPT_CHANNEL_REMOTE_PORT:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        this->channels_[index]->port_ = (u_short)va_arg(ap, int);
        update_dns_queries_state(this->channels_[index], true);
      }
    }
    break;
    case YOPT_CHANNEL_REMOTE_ENDPOINT:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        auto channel   = this->channels_[index];
        channel->host_ = va_arg(ap, const char *);
        channel->port_ = (u_short)va_arg(ap, int);
        update_dns_queries_state(this->channels_[index], true);
      }
    }
    break;
    case YOPT_NO_NEW_THREAD:
      this->options_.no_new_thread_ = !!va_arg(ap, int);
      break;
  }

  va_end(ap);
}
} // namespace inet
} // namespace yasio

#if defined(_WIN32)
#  pragma warning(pop)
#endif
