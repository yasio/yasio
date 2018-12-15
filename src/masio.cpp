//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app version: 3.9.3
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2018 HALX99

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
#include "masio.h"
#include <limits>
#include <stdarg.h>
#include <string>

#if _USING_ARES_LIB
#  if !defined(CARES_STATICLIB)
#    define CARES_STATICLIB 1
#  endif
extern "C" {
#  include "c-ares/ares.h"
}
#  if defined(_WIN32)
#    if defined(_DEBUG)
#      pragma comment(lib, "libcaresd.lib")
#    else
#      pragma comment(lib, "libcares.lib")
#    endif
#  endif
#endif

#if !defined(MICROSECONDS_PER_SECOND)
#  define MICROSECONDS_PER_SECOND 1000000LL
#endif

#define _MASIO_VERBOS_LOG 0

#if defined(_WIN32)
#  define INET_LOG(format, ...)                                                                    \
    do                                                                                             \
    {                                                                                              \
      auto content = _sfmt(("[mini-asio][%lld] " format "\r\n"), _highp_clock(), ##__VA_ARGS__);   \
      OutputDebugStringA(content.c_str());                                                         \
      if (options_.outf)                                                                           \
        fprintf(options_.outf, "%s", content.c_str());                                             \
    } while (false)
#elif defined(ANDROID) || defined(__ANDROID__)
#  include <android/log.h>
#  include <jni.h>
#  define INET_LOG(format, ...)                                                                    \
    __android_log_print(ANDROID_LOG_INFO, "mini-asio", ("[%lld]" format), _highp_clock(),          \
                        ##__VA_ARGS__);                                                            \
    if (options_.outf)                                                                             \
    fprintf(options_.outf, ("[mini-asio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__)
#else
#  define INET_LOG(format, ...)                                                                    \
    fprintf(stdout, ("[mini-asio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__);            \
    if (options_.outf)                                                                             \
    fprintf(options_.outf, ("[mini-asio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__)
#endif

#define ASYNC_RESOLVE_TIMEOUT 45 // 45 seconds

#define MAX_WAIT_DURATION 5 * 60 * 1000 * 1000 // 5 minites
#define MAX_PDU_BUFFER_SIZE                                                                        \
  static_cast<int>(SZ(1, M)) // max pdu buffer length, avoid large memory allocation when \
        // application layer decode a huge length field.

namespace purelib
{
namespace inet
{

namespace
{
// The high precision micro seconds timestamp
static long long _highp_clock()
{
  auto duration = highp_clock_t::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

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
#define CC_VSNPRINTF_BUFFER_LENGTH 512
  va_list args;
  std::string buffer(CC_VSNPRINTF_BUFFER_LENGTH, '\0');

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

#if _USING_ARES_LIB
static void ares_getaddrinfo_callback(void *arg, int status, addrinfo *answerlist)
{
  auto ctx = (channel *)arg;
  if (status == ARES_SUCCESS)
  {
    if (answerlist != nullptr)
    {
      ip::endpoint ep(answerlist);
      ep.port(ctx->port_);
      std::string ip = ep.to_string();
      ctx->endpoints_.push_back(ep);
      ctx->dns_queries_timestamp_ = _highp_clock();
      INET_LOG("[index: %d] ares_getaddrinfo_callback: resolve %s succeed, ip:%s", ctx->index_,
               ctx->address_.c_str(), ip.c_str());
    }
  }
  else
  {
    INET_LOG("[index: %d] ares_getaddrinfo_callback: resolve %s failed, status:%d", ctx->index_,
             ctx->address_.c_str(), status);
  }

  ctx->deadline_timer_.cancel();
  ctx->resolve_state_ = !ctx->endpoints_.empty() ? resolve_state::READY : resolve_state::FAILED;
  ctx->deadline_timer_.service_.handle_ares_work_finish(ctx);
}
#endif

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
#  pragma warning(push)
#  pragma warning(disable : 6320 6322)
  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {}
#  pragma warning(pop)
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
void deadline_timer::async_wait(const std::function<void(bool cancelled)> &callback)
{
  this->callback_ = callback;
  this->service_.schedule_timer(this);
}

void deadline_timer::cancel()
{
  if (!expired())
  {
    this->service_.cancel_timer(this);
    this->expire();
  }
}

/// io_channel
io_channel::io_channel(io_service &service) : deadline_timer_(service)
{
  socket_.reset(new xxsocket());
}

void io_channel::reset()
{
  state_ = channel_state::INACTIVE;

  resolve_state_         = resolve_state::FAILED;
  dns_queries_timestamp_ = 0;
  dns_queries_needed_    = false;
  endpoints_.clear();
  deadline_timer_.cancel();
}

io_service::io_service() : stopping_(false), thread_started_(false), interrupter_()
{
  options_.connect_timeout_   = 5LL * MICROSECONDS_PER_SECOND;
  options_.send_timeout_      = (std::numeric_limits<int>::max)();
  options_.reconnect_timeout_ = -1;
  // Default: 10 minutes.
  options_.dns_cache_timeout_ = 600LL * MICROSECONDS_PER_SECOND;
  FD_ZERO(&fds_array_[read_op]);
  FD_ZERO(&fds_array_[write_op]);
  FD_ZERO(&fds_array_[except_op]);

  maxfdp_           = 0;
  outstanding_work_ = 0;
#if _USING_ARES_LIB
  ares_                  = nullptr;
  ares_outstanding_work_ = 0;
#endif
  ipsv_state_ = 0;
}

io_service::~io_service() { stop_service(); }

void io_service::stop_service()
{
  if (thread_started_)
  {
    stopping_ = true;

    for (auto ctx : channels_)
    {
      ctx->deadline_timer_.cancel();
      if (ctx->socket_->is_open())
      {
        ctx->socket_->shutdown();
      }
    }

    interrupter_.interrupt();
    if (this->worker_thread_.joinable())
      this->worker_thread_.join();

    clear_channels();

    unregister_descriptor(interrupter_.read_descriptor(), socket_event_read);
    thread_started_ = false;
#if _USING_ARES_LIB
    if (this->ares_ != nullptr)
    {
      ::ares_destroy((ares_channel)this->ares_);
      this->ares_ = nullptr;
    }
#endif
  }
}

void io_service::set_option(int option, ...)
{
  va_list ap;
  va_start(ap, option);

  switch (option)
  {
    case MASIO_OPT_CONNECT_TIMEOUT:
      options_.connect_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case MASIO_OPT_SEND_TIMEOUT:
      options_.send_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case MASIO_OPT_RECONNECT_TIMEOUT:
    {
      int value = va_arg(ap, int);
      if (value > 0)
        options_.reconnect_timeout_ = static_cast<highp_time_t>(value) * MICROSECONDS_PER_SECOND;
      else
        options_.reconnect_timeout_ = -1; // means auto reconnect is disabled.
    }
    break;
    case MASIO_OPT_DNS_CACHE_TIMEOUT:
      options_.dns_cache_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case MASIO_OPT_DEFER_EVENT:
      options_.deferred_event_ = !!va_arg(ap, int);
      break;
    case MASIO_OPT_TCP_KEEPALIVE:
      options_.tcp_keepalive.onoff    = 1;
      options_.tcp_keepalive.idle     = va_arg(ap, int);
      options_.tcp_keepalive.interval = va_arg(ap, int);
      options_.tcp_keepalive.probs    = va_arg(ap, int);
      break;
    case MASIO_OPT_RESOLV_FUNCTION:
      this->xresolv_ = std::move(*va_arg(ap, resolv_fn_t *));
      break;
    case MASIO_OPT_LOG_FILE:
      if (options_.outf)
        fclose(options_.outf);
      options_.outf = fopen(va_arg(ap, const char *), "wb");
      break;
    case MASIO_OPT_LFIB_PARAMS:
      options_.lfib.max_frame_length    = va_arg(ap, int);
      options_.lfib.length_field_offset = va_arg(ap, int);
      options_.lfib.length_field_length = va_arg(ap, int);
      options_.lfib.length_adjustment   = va_arg(ap, int);
      break;
    case MASIO_OPT_IO_EVENT_CALLBACK:
      this->on_event_ = std::move(*va_arg(ap, io_event_callback_t *));
      break;
  }

  va_end(ap);
}

io_channel *io_service::new_channel(const io_hostent &ep)
{
  auto ctx = new io_channel(*this);
  ctx->reset();
  ctx->address_ = ep.address_;
  ctx->port_    = ep.port_;
  ctx->index_   = static_cast<int>(this->channels_.size());
  update_resolve_state(ctx);
  this->channels_.push_back(ctx);
  return ctx;
}

void io_service::clear_channels()
{
  for (auto iter = channels_.begin(); iter != channels_.end();)
  {
    (*iter)->socket_->close();
    delete *(iter);
    iter = channels_.erase(iter);
  }
}

size_t io_service::get_event_count(void) const { return this->event_queue_.size(); }

void io_service::dispatch_events(int count)
{
  assert(this->on_event_ != nullptr);

  if (this->event_queue_.empty())
    return;

  std::lock_guard<std::mutex> lck(this->event_queue_mtx_);
  do
  {
    auto event = this->event_queue_.front();
    this->event_queue_.pop_front();
    this->on_event_(std::move(event));
  } while (!this->event_queue_.empty() && --count > 0);
}

void io_service::start_service(const io_hostent *channel_eps, int channel_count,
                               io_event_callback_t cb)
{
  if (!thread_started_)
  {
    if (channel_count <= 0)
      return;
    if (cb)
      this->on_event_ = std::move(cb);

    stopping_       = false;
    thread_started_ = true;
#if _USING_ARES_LIB
    /* Initialize the library */
    ::ares_library_init(ARES_LIB_INIT_ALL);

    int ret = ::ares_init((ares_channel *)&ares_);

    if (ret == ARES_SUCCESS)
    {
      // set dns servers, optional, comment follow code, ares also work well.
      // ::ares_set_servers_csv((ares_channel )ares_,
      // "114.114.114.114,8.8.8.8");
    }
    else
      INET_LOG("Initialize ares failed: %d!", ret);
#endif

    register_descriptor(interrupter_.read_descriptor(), socket_event_read);

    // Initialize channels
    for (auto i = 0; i < channel_count; ++i)
    {
      auto &channel_ep = channel_eps[i];
      (void)new_channel(channel_ep);
    }

    worker_thread_ = std::thread(&io_service::service, this);
  }
}

void io_service::set_endpoint(size_t channel_index, const char *address, u_short port)
{
  // Gets channel context
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  ctx->address_ = address;
  ctx->port_    = port;
  update_resolve_state(ctx);
}

void io_service::set_endpoint(size_t channel_index, const ip::endpoint &ep)
{
  // Gets channel context
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  ctx->endpoints_.clear();
  ctx->endpoints_.push_back(ep);
  ctx->resolve_state_ = resolve_state::READY;
}

void io_service::service()
{ // The async event-loop
  // Set Thread Name: mini async socket io
  _set_thread_name("mini-asio");

  // Call once at startup
  this->ipsv_state_ = xxsocket::getipsv();

  // event loop
  fd_set fds_array[3];
  timeval timeout;

  for (; !stopping_;)
  {
    int nfds = do_select(fds_array, timeout);

    if (stopping_)
      break;

    if (nfds == -1)
    {
      int ec = xxsocket::get_last_errno();
      INET_LOG("socket.select failed, ec:%d, detail:%s\n", ec, xxsocket::strerror(ec));
      if (ec == EBADF)
      {
        goto _L_end;
      }
      continue; // try select again.
    }

    if (nfds == 0)
    {
#if _MASIO_VERBOS_LOG
      INET_LOG("socket.select is timeout, do perform_timeout_timers()");
#endif
    }
    // Reset the interrupter.
    else if (nfds > 0 && FD_ISSET(this->interrupter_.read_descriptor(), &(fds_array[read_op])))
    {
#if _MASIO_VERBOS_LOG
      bool was_interrupt = interrupter_.reset();
      INET_LOG("socket.select waked up by interrupt, interrupter fd:%d, "
               "was_interrupt:%s",
               this->interrupter_.read_descriptor(), was_interrupt ? "true" : "false");
#else
      interrupter_.reset();
#endif
      --nfds;
    }
#if _USING_ARES_LIB
    /// perform possible domain resolve requests.
    if (this->ares_outstanding_work_ > 0)
    {
      ares_socket_t socks[ARES_GETSOCK_MAXNUM] = {0};
      int bitmask = ares_getsock((ares_channel)this->ares_, socks, _ARRAYSIZE(socks));

      for (int i = 0; i < ARES_GETSOCK_MAXNUM; ++i)
      {
        if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i))
        {
          auto fd = socks[i];
          ::ares_process_fd((ares_channel)this->ares_,
                            FD_ISSET(fd, &(fds_array[read_op])) ? fd : ARES_SOCKET_BAD,
                            FD_ISSET(fd, &(fds_array[write_op])) ? fd : ARES_SOCKET_BAD);
        }
        else
          break;
      }
    }
#endif

    // preform transports
    for (auto iter = transports_.begin(); iter != transports_.end();)
    {
      auto &transport = *iter;
      if (transport->offset_ > 0 ||
          FD_ISSET(transport->socket_->native_handle(), &(fds_array[read_op])))
      {
#if _MASIO_VERBOS_LOG
        INET_LOG("[index: %d] perform non-blocking read operation...", ctx->index_);
#endif
        if (!do_read(transport))
        {
          handle_close(transport);
          iter = transports_.erase(iter);
          continue;
        }
      }

      // perform write operations
      if (!transport->send_queue_.empty())
      {
        transport->send_queue_mtx_.lock();
#if _MASIO_VERBOS_LOG
        INET_LOG("[index: %d] perform non-blocking write operation...", ctx->index_);
#endif
        if (!do_write(transport))
        { // TODO: check would block? for client, may
          // be unnecessary.
          transport->send_queue_mtx_.unlock();
          handle_close(transport);
          iter = transports_.erase(iter);
          continue;
        }

        if (!transport->send_queue_.empty())
          ++this->outstanding_work_;

        transport->send_queue_mtx_.unlock();
      }

      ++iter;
    }

    // perform active channels
    perform_active_channels(fds_array);

    // perform timeout timers
    perform_timeout_timers();
  }

_L_end:
  (void)0; // ONLY for xcode compiler happy.
}

void io_service::perform_active_channels(fd_set *fds_array)
{
  if (!active_channels_.empty())
  {
    // perform active channels
    active_channels_mtx_.lock();
    for (auto iter = active_channels_.begin(); iter != active_channels_.end();)
    {
      auto ctx    = *iter;
      bool finish = false;
      switch (ctx->type_)
      {
        case CHANNEL_TCP_CLIENT:
          switch (ctx->state_)
          {
            case channel_state::REQUEST_CONNECT:
              finish = do_nonblocking_connect(ctx);
              break;
            case channel_state::CONNECTING:
              finish = do_nonblocking_connect_completion(fds_array, ctx);
              break;
            default:; // do nothing
          }
          break;
        case CHANNEL_TCP_SERVER:

          switch (ctx->state_)
          {
            case channel_state::REQUEST_CONNECT:
              do_nonblocking_accept(ctx);
              break;
            case channel_state::CONNECTING:
              do_nonblocking_accept_completion(fds_array, ctx);
              break;
            case channel_state::INACTIVE:
              finish = true;
              break;
            default:; // do nothing
          }
          break;
      }

      if (finish)
        iter = active_channels_.erase(iter);
      else
        ++iter;
    }
    active_channels_mtx_.unlock();
  }
}

void io_service::close(size_t channel_index)
{
  // Gets channel context
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  assert(ctx->type_ == CHANNEL_TCP_SERVER);
  if (ctx->type_ != CHANNEL_TCP_SERVER)
    return;
  if (ctx->state_ != channel_state::INACTIVE)
  {
    ctx->state_ = channel_state::INACTIVE;
    unregister_descriptor(ctx->socket_->native_handle(), socket_event_read);
    ctx->socket_->close();
    interrupt();
  }
}

void io_service::close(transport_ptr &transport)
{
  if (transport->is_open())
  {
    INET_LOG("close the transport: %s --> %s",
             transport->socket_->local_endpoint().to_string().c_str(),
             transport->socket_->peer_endpoint().to_string().c_str());
    transport->offset_ = 1; // !IMPORTANT, trigger the close immidlately.
    transport->socket_->shutdown();
    transport.reset();
    interrupter_.interrupt();
  }
}

bool io_service::is_connected(size_t channel_index) const
{
  // Gets channel
  if (channel_index >= channels_.size())
    return false;
  auto ctx = channels_[channel_index];
  return ctx->state_ == channel_state::CONNECTED;
}

void io_service::reopen(transport_ptr transport)
{
  if (transport->is_open())
  {
    transport->offset_ = 1; // !IMPORTANT, trigger the close immidlately.
  }
  open_internal(transport->ctx_);
}

void io_service::open(size_t channel_index, int channel_type)
{
  // Gets channel
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  ctx->type_ = channel_type;

  open_internal(ctx);
}

void io_service::handle_close(transport_ptr transport)
{
  INET_LOG("the connection %s --> %s is lost, error:%d, detail:%s",
           transport->local_endpoint().to_string().c_str(),
           transport->peer_endpoint().to_string().c_str(), transport->error_,
           xxsocket::strerror(transport->error_));

  close_internal(transport.get());

  auto ctx = transport->ctx_;

  // @Notify connection lost
  this->handle_event(event_ptr(
      new io_event(ctx->index_, MASIO_EVENT_CONNECTION_LOST, transport->error_, transport)));

  if (ctx->type_ == CHANNEL_TCP_CLIENT)
  {
    if (channel_state::REQUEST_CONNECT != ctx->state_)
      ctx->state_ = channel_state::INACTIVE;
    if (options_.reconnect_timeout_ > 0)
    {
      std::shared_ptr<deadline_timer> timer(new deadline_timer(*this));
      timer->expires_from_now(std::chrono::microseconds(options_.reconnect_timeout_));
      timer->async_wait(
          [this, ctx, timer /*!important, hold on by lambda expression */](bool cancelled) {
            if (!cancelled)
              this->open_internal(ctx);
          });
    }
  }
}

void io_service::register_descriptor(const socket_native_type fd, int flags)
{
  if ((flags & socket_event_read) != 0)
  {
    FD_SET(fd, &(fds_array_[read_op]));
  }

  if ((flags & socket_event_write) != 0)
  {
    FD_SET(fd, &(fds_array_[write_op]));
  }

  if ((flags & socket_event_except) != 0)
  {
    FD_SET(fd, &(fds_array_[except_op]));
  }

  if (maxfdp_ < static_cast<int>(fd) + 1)
    maxfdp_ = static_cast<int>(fd) + 1;
}

void io_service::unregister_descriptor(const socket_native_type fd, int flags)
{
  if ((flags & socket_event_read) != 0)
  {
    FD_CLR(fd, &(fds_array_[read_op]));
  }

  if ((flags & socket_event_write) != 0)
  {
    FD_CLR(fd, &(fds_array_[write_op]));
  }

  if ((flags & socket_event_except) != 0)
  {
    FD_CLR(fd, &(fds_array_[except_op]));
  }
}

void io_service::write(transport_ptr transport, std::vector<char> data)
{
  if (transport && transport->socket_->is_open())
  {
    auto pdu =
        a_pdu_ptr(new a_pdu(std::move(data), std::chrono::microseconds(options_.send_timeout_)));

    transport->send_queue_mtx_.lock();
    transport->send_queue_.push_back(pdu);
    transport->send_queue_mtx_.unlock();

    interrupter_.interrupt();
  }
  else
  {
    INET_LOG("[transport: %p] send failed, the connection not ok!", transport.get());
  }
}

void io_service::handle_packet(transport_ptr transport)
{
#if _MASIO_VERBOS_LOG
  INET_LOG("[index: %d] received a properly packet from peer, "
           "packet size:%d",
           ctx->index_, ctx->receiving_pdu_elen_);
#endif
  this->handle_event(event_ptr(new io_event(transport->channel_index(), MASIO_EVENT_RECV_PACKET,
                                            std::move(transport->receiving_pdu_))));
  transport->receiving_pdu_elen_ = -1;
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
  {
    this->on_event_(std::move(event));
  }
}

bool io_service::do_nonblocking_connect(io_channel *ctx)
{
  assert(ctx->state_ == channel_state::REQUEST_CONNECT);
  if (ctx->state_ != channel_state::REQUEST_CONNECT)
    return true;

  if (this->ipsv_state_ == 0)
    this->ipsv_state_ = xxsocket::getipsv();

  auto diff = (_highp_clock() - ctx->dns_queries_timestamp_);
  if (ctx->dns_queries_needed_ && ctx->resolve_state_ == resolve_state::READY &&
      diff >= options_.dns_cache_timeout_)
    ctx->resolve_state_ = resolve_state::DIRTY;

  if (ctx->resolve_state_ == resolve_state::READY)
  {
    if (!ctx->socket_->is_open())
    { // cleanup descriptor if possible
      INET_LOG("[index: %d] connecting server %s:%u...", ctx->index_, ctx->address_.c_str(),
               ctx->port_);
    }
    else
    {
      close_internal(ctx);
      INET_LOG("[index: %d] reconnecting server %s:%u...", ctx->index_, ctx->address_.c_str(),
               ctx->port_);
    }

    ctx->state_ = channel_state::CONNECTING;

    int ret  = -1;
    auto &ep = ctx->endpoints_[0];
    if (ctx->socket_->reopen(ep.af()))
    {
      ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
      ret = xxsocket::connect_n(ctx->socket_->native_handle(), ep);
    }

    if (ret < 0)
    { // setup no blocking connect
      int error = xxsocket::get_last_errno();
      if (error != EINPROGRESS && error != EWOULDBLOCK)
      {
        this->handle_connect_failed(ctx, error);
        return true;
      }
      else
      {
        register_descriptor(ctx->socket_->native_handle(), socket_event_read | socket_event_write);

        ctx->deadline_timer_.expires_from_now(std::chrono::microseconds(options_.connect_timeout_));
        ctx->deadline_timer_.async_wait([this, ctx](bool cancelled) {
          if (!cancelled && ctx->state_ != channel_state::CONNECTED)
          {
            handle_connect_failed(ctx, ERR_CONNECT_TIMEOUT);
          }
        });

        return false;
      }
    }
    else if (ret == 0)
    { // connect server succed immidiately.
      handle_connect_succeed(ctx, ctx->socket_);
      return true;
    }
    // NEVER GO HERE
  }
  else if (ctx->resolve_state_ == resolve_state::FAILED)
  {
    handle_connect_failed(ctx, ERR_RESOLVE_HOST_FAILED);
    return true;
  } // DIRTY,Try resolve address nonblocking
  else if (ctx->resolve_state_ == resolve_state::DIRTY)
  {
    return start_resolve(ctx);
  }

  return !(ctx->resolve_state_ == resolve_state::INPRROGRESS);
}

bool io_service::do_nonblocking_connect_completion(fd_set *fds_array, io_channel *ctx)
{
  if (ctx->state_ == channel_state::CONNECTING)
  {
    int error = -1;
    if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[write_op]) ||
        FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
    {
      socklen_t len = sizeof(error);
      if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
              0 &&
          error == 0)
      {
        handle_connect_succeed(ctx, ctx->socket_);
        ctx->deadline_timer_.cancel();
      }
      else
      {
        handle_connect_failed(ctx, ERR_CONNECT_FAILED);
        ctx->deadline_timer_.cancel();
      }

      return true;
    }
    // else  ; // Check whether connect is timeout.
  }

  return false;
}

void io_service::do_nonblocking_accept(io_channel *ctx)
{ // channel is server
  close_internal(ctx);

  if (ctx->socket_->reopen(ipsv_state_ & ipsv_ipv4 ? AF_INET : AF_INET6))
  {
    ctx->state_ = channel_state::CONNECTING;

    ip::endpoint ep(ipsv_state_ & ipsv_ipv4 ? "0.0.0.0" : "::", ctx->port_);
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
    ctx->socket_->set_nonblocking(true);
    int error = 0;
    if (ctx->socket_->bind(ep) != 0)
    {
      error = xxsocket::get_last_errno();
      INET_LOG("[index: %d] bind failed, ec:%d, detail:%s", ctx->index_, error,
               xxsocket::strerror(error));
      ctx->socket_->close();
      ctx->state_ = channel_state::INACTIVE;
      return;
    }

    if (ctx->socket_->listen(1) != 0)
    {
      error = xxsocket::get_last_errno();
      INET_LOG("[index: %d] listening failed, ec:%d, detail:%s", ctx->index_, error,
               xxsocket::strerror(error));
      ctx->socket_->close();
      ctx->state_ = channel_state::INACTIVE;
      return;
    }

    INET_LOG("[index: %d] listening at %s...", ctx->index_, ep.to_string().c_str());
    register_descriptor(ctx->socket_->native_handle(), socket_event_read);
  }
}

void io_service::do_nonblocking_accept_completion(fd_set *fds_array, io_channel *ctx)
{
  if (ctx->state_ == channel_state::CONNECTING)
  {
    int error = -1;
    if (FD_ISSET(ctx->socket_->native_handle(), &fds_array[read_op]))
    {
      socklen_t len = sizeof(error);
      if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
              0 &&
          error == 0)
      {
        xxsocket client_sock = ctx->socket_->accept();
        if (client_sock.is_open())
        {
          register_descriptor(client_sock.native_handle(), socket_event_read);

          handle_connect_succeed(ctx,
                                 std::shared_ptr<xxsocket>(new xxsocket(std::move(client_sock))));
          ctx->state_ = channel_state::CONNECTING;
        }
      }
      else
      {
        close_internal(ctx);
      }
    }
  }
}

void io_service::handle_connect_succeed(io_channel *ctx, std::shared_ptr<xxsocket> socket)
{
  transport_ptr transport(new io_transport(ctx));

  if (ctx->type_ == CHANNEL_TCP_CLIENT)
  { // The client channl
    unregister_descriptor(socket->native_handle(),
                          socket_event_write); // remove write event avoid
    // high-CPU occupation
    ctx->state_ = channel_state::CONNECTED;
  }

  if (options_.tcp_keepalive.onoff)
  {
    socket->set_keepalive(options_.tcp_keepalive.idle, options_.tcp_keepalive.interval,
                          options_.tcp_keepalive.probs);
  }

  transport->socket_ = socket;
  this->transports_.push_back(transport);

  auto connection = transport->socket_;
  INET_LOG("[index: %d] the connection [%s] ---> %s is established.", ctx->index_,
           connection->local_endpoint().to_string().c_str(),
           connection->peer_endpoint().to_string().c_str());

  this->handle_event(
      event_ptr(new io_event(ctx->index_, MASIO_EVENT_CONNECT_RESPONSE, 0, transport)));
}

void io_service::handle_connect_failed(io_channel *ctx, int error)
{
  close_internal(ctx);

  ctx->state_ = channel_state::INACTIVE;

  this->handle_event(
      event_ptr(new io_event(ctx->index_, MASIO_EVENT_CONNECT_RESPONSE, error, nullptr)));

  INET_LOG("[index: %d] connect server %s:%u failed, ec:%d, detail:%s", ctx->index_,
           ctx->address_.c_str(), ctx->port_, error, xxsocket::strerror(error));
}

bool io_service::do_write(transport_ptr transport)
{
  bool bRet = false;
  auto ctx  = transport->ctx_;
  do
  {
    int n;

    if (!transport->socket_->is_open())
      break;

    if (!transport->send_queue_.empty())
    {
      auto v                 = transport->send_queue_.front();
      auto outstanding_bytes = static_cast<int>(v->data_.size() - v->offset_);
      n = transport->socket_->send_i(v->data_.data() + v->offset_, outstanding_bytes);
      if (n == outstanding_bytes)
      { // All pdu bytes sent.
        transport->send_queue_.pop_front();
#if _MASIO_VERBOS_LOG
        auto packet_size = static_cast<int>(v->data_.size());
        INET_LOG("[index: %d] do_write ok, A packet sent "
                 "success, packet size:%d",
                 ctx->index_, packet_size);
#endif
        handle_send_finished(v, error_number::ERR_OK);
      }
      else if (n > 0)
      { // TODO: add time
        if (!v->expired())
        { // change offset, remain data will
          // send next time.
          // v->data_.erase(v->data_.begin(), v->data_.begin() +
          // n);
          v->offset_ += n;
          outstanding_bytes = static_cast<int>(v->data_.size() - v->offset_);
          INET_LOG("[index: %d] do_write pending, %dbytes still "
                   "outstanding, "
                   "%dbytes was sent!",
                   ctx->index_, outstanding_bytes, n);
        }
        else
        { // send timeout
          transport->send_queue_.pop_front();

          auto packet_size = static_cast<int>(v->data_.size());
          INET_LOG("[index: %d] do_write packet timeout, packet "
                   "size:%d",
                   ctx->index_, packet_size);
          handle_send_finished(v, error_number::ERR_SEND_TIMEOUT);
        }
      }
      else
      { // n <= 0, TODO: add time
        int error = transport->get_socket_error();
        if (SHOULD_CLOSE_1(n, error))
        {
          INET_LOG("[index: %d] do_write error, the connection "
                   "should be "
                   "closed, retval=%d, ec:%d, detail:%s",
                   ctx->index_, n, error, xxsocket::strerror(error));
          break;
        }
      }
    }

    bRet = true;
  } while (false);

  return bRet;
}

void io_service::handle_send_finished(a_pdu_ptr /*pdu*/, error_number /*error*/) {}

bool io_service::do_read(transport_ptr transport)
{
  bool bRet = false;
  auto ctx  = transport->ctx_;
  do
  {
    if (!transport->socket_->is_open())
      break;

    int n = transport->socket_->recv_i(transport->buffer_ + transport->offset_,
                                       socket_recv_buffer_size - transport->offset_);

    if (n > 0 || !SHOULD_CLOSE_0(n, transport->get_socket_error()))
    {
#if _MASIO_VERBOS_LOG
      INET_LOG("[index: %d] do_read status ok, ec:%d, detail:%s", ctx->index_, error_,
               xxsocket::strerror(error_));
#endif
      if (n == -1)
        n = 0;
#if _MASIO_VERBOS_LOG
      if (n > 0)
      {
        INET_LOG("[index: %d] do_read ok, received data len: %d, "
                 "buffer data "
                 "len: %d",
                 ctx->index_, n, n + ctx->offset_);
      }
#endif
      if (transport->receiving_pdu_elen_ == -1)
      { // decode length
        int length = decode_frame_length(transport->buffer_, transport->offset_ + n);
        if (length > 0)
        {
          transport->receiving_pdu_elen_ = length;
          transport->receiving_pdu_.reserve(
              (std::min)(transport->receiving_pdu_elen_,
                         MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid // memory reallocte.
          do_unpack(transport, transport->receiving_pdu_elen_, n);
        }
        else if (length == 0)
        {
          // header insufficient, wait readfd ready at
          // next event step.
          transport->offset_ += n;
        }
        else
        {
          // set_errorno(ctx, error_number::ERR_DPL_ILLEGAL_PDU);
          INET_LOG("[index: %d] do_read error, decode length of "
                   "pdu failed, "
                   "the connection should be closed!",
                   ctx->index_);
          break;
        }
      }
      else
      { // process incompleted pdu
        do_unpack(
            transport,
            transport->receiving_pdu_elen_ - static_cast<int>(transport->receiving_pdu_.size()), n);
      }
    }
    else
    {
      int error            = transport->error_;
      const char *errormsg = xxsocket::strerror(error);
      if (n == 0)
      {
        INET_LOG("[index: %d] do_read error, the server close the "
                 "connection, retval=%d, ec:%d, detail:%s",
                 ctx->index_, n, error, errormsg);
      }
      else
      {
        INET_LOG("[index: %d] do_read error, the connection should be "
                 "closed, retval=%d, ec:%d, detail:%s",
                 ctx->index_, n, error, errormsg);
      }
      break;
    }

    bRet = true;

  } while (false);

  return bRet;
}

void io_service::do_unpack(transport_ptr ctx, int bytes_expected, int bytes_transferred)
{
  auto bytes_available = bytes_transferred + ctx->offset_;
  ctx->receiving_pdu_.insert(ctx->receiving_pdu_.end(), ctx->buffer_,
                             ctx->buffer_ + (std::min)(bytes_expected, bytes_available));

  ctx->offset_ = bytes_available - bytes_expected; // set offset to bytes of remain buffer
  if (ctx->offset_ >= 0)
  {                       // pdu received properly
    if (ctx->offset_ > 0) // move remain data to head of buffer and hold offset.
    {
      ::memmove(ctx->buffer_, ctx->buffer_ + bytes_expected, ctx->offset_);
      // not all data consumed, so add events for this context
      ++this->outstanding_work_;
    }
    // move properly pdu to ready queue, GL thread will retrieve
    // it.
    handle_packet(ctx);
    ctx->receiving_pdu_elen_ = -1;
  }
  else
  { // all buffer consumed, set offset to ZERO, pdu
    // incomplete, continue recv remain data.
    ctx->offset_ = 0;
  }
}

void io_service::schedule_timer(deadline_timer *timer)
{
  // pitfall: this service only hold the weak pointer of the timer
  // object, so before dispose the timer object need call
  // cancel_timer to cancel it.
  if (timer == nullptr)
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  if (std::find(timer_queue_.begin(), timer_queue_.end(), timer) != timer_queue_.end())
    return;

  this->timer_queue_.push_back(timer);

  std::sort(this->timer_queue_.begin(), this->timer_queue_.end(),
            [](deadline_timer *lhs, deadline_timer *rhs) {
              return lhs->wait_duration() > rhs->wait_duration();
            });

  if (timer == *this->timer_queue_.begin())
    interrupter_.interrupt();
}

void io_service::cancel_timer(deadline_timer *timer)
{
  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  auto iter = std::find(timer_queue_.begin(), timer_queue_.end(), timer);
  if (iter != timer_queue_.end())
  {
    auto callback = timer->callback_;
    callback(true);
    timer_queue_.erase(iter);
  }
}

void io_service::open_internal(io_channel *ctx)
{
  if (ctx->state_ == channel_state::REQUEST_CONNECT || ctx->state_ == channel_state::CONNECTING)
  { // in-progress, do nothing
    INET_LOG("[index: %d] the connect request is already in progress!", ctx->index_);
    return;
  }

  if (ctx->resolve_state_ != resolve_state::READY)
    update_resolve_state(ctx);

  ctx->state_ = channel_state::REQUEST_CONNECT;
  if (ctx->socket_->is_open())
  {
    ctx->socket_->shutdown();
  }

  active_channels_mtx_.lock();
  this->active_channels_.push_back(ctx);
  active_channels_mtx_.unlock();

  interrupter_.interrupt();
}

void io_service::perform_timeout_timers()
{
  if (this->timer_queue_.empty())
    return;

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);

  std::vector<deadline_timer *> loop_timers;
  while (!this->timer_queue_.empty())
  {
    auto earliest = timer_queue_.back();
    if (earliest->expired())
    {
      timer_queue_.pop_back();
      auto callback = earliest->callback_;
      callback(false);
      if (earliest->repeated_)
      {
        earliest->expires_from_now();
        loop_timers.push_back(earliest);
      }
    }
    else
    {
      break;
    }
  }

  if (!loop_timers.empty())
  {
    this->timer_queue_.insert(this->timer_queue_.end(), loop_timers.begin(), loop_timers.end());
    std::sort(this->timer_queue_.begin(), this->timer_queue_.end(),
              [](deadline_timer *lhs, deadline_timer *rhs) {
                return lhs->wait_duration() > rhs->wait_duration();
              });
  }
}

int io_service::do_select(fd_set *fds_array, timeval &maxtv)
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
      maxtv.tv_sec  = static_cast<long>(wait_duration / 1000000);
      maxtv.tv_usec = static_cast<long>(wait_duration % 1000000);
#if _MASIO_VERBOS_LOG
      INET_LOG("socket.select maxfdp:%d waiting... %ld milliseconds", maxfdp_,
               maxtv.tv_sec * 1000 + maxtv.tv_usec / 1000);
#endif
#if !_USING_ARES_LIB
      nfds =
          ::select(this->maxfdp_, &(fds_array[read_op]), &(fds_array[write_op]), nullptr, &maxtv);
#else
      timeval *pmaxtv = &maxtv;
      if (this->ares_outstanding_work_ > 0 &&
          ::ares_fds((ares_channel)this->ares_, &fds_array[read_op], &fds_array[write_op]) > 0)
      {
        struct timeval tv = {0};
        pmaxtv            = ::ares_timeout((ares_channel)this->ares_, &maxtv, &tv);
      }
      nfds =
          ::select(this->maxfdp_, &(fds_array[read_op]), &(fds_array[write_op]), nullptr, pmaxtv);
#endif

#if _MASIO_VERBOS_LOG
      INET_LOG("socket.select waked up, retval=%d", nfds);
#endif
    }
    else
    {
      nfds = static_cast<int>(channels_.size()) << 1;
    }
  }

  return nfds;
}

long long io_service::get_wait_duration(long long usec)
{
  if (this->timer_queue_.empty())
  {
    return usec;
  }

  std::lock_guard<std::recursive_mutex> lck(this->timer_queue_mtx_);
  deadline_timer *earliest = timer_queue_.back();

  // microseconds
  auto duration = earliest->wait_duration();
  if (std::chrono::microseconds(usec) > duration)
    return duration.count();
  else
    return usec;
}

bool io_service::close_internal(io_base *ctx)
{
  if (ctx->socket_->is_open())
  {
    unregister_descriptor(ctx->socket_->native_handle(), socket_event_read | socket_event_write);
    ctx->socket_->close();
    return true;
  }
  return false;
}

void io_service::update_resolve_state(io_channel *ctx)
{
  if (ctx->port_ > 0)
  {
    ip::endpoint ep;
    ctx->endpoints_.clear();
    ctx->dns_queries_needed_ = !ep.assign(ctx->address_.c_str(), ctx->port_);
    if (!ctx->dns_queries_needed_)
    {
      ctx->endpoints_.push_back(ep);
      ctx->resolve_state_ = resolve_state::READY;
    }
    else
      ctx->resolve_state_ = resolve_state::DIRTY;
  }
  else
    ctx->resolve_state_ = resolve_state::FAILED;
}

bool io_service::start_resolve(io_channel *ctx)
{ // Only call at event-loop thread, so
  // no need to consider thread safe.
  if (ctx->resolve_state_ != resolve_state::DIRTY)
    return false;
  ctx->resolve_state_ = resolve_state::INPRROGRESS;
  ctx->endpoints_.clear();

  INET_LOG("[index: %d] start async resolving for %s", ctx->index_, ctx->address_.c_str());
#if !_USING_ARES_LIB // 6.563ms
  std::thread resolve_thread([=] {
    addrinfo hint;
    memset(&hint, 0x0, sizeof(hint));

    bool succeed = xresolv_ ? xresolv_(ctx->endpoints_, ctx->address_.c_str(), ctx->port_)
                            : resolve(ctx->endpoints_, ctx->address_.c_str(), ctx->port_);

    if (succeed && !ctx->endpoints_.empty())
    {
      ctx->resolve_state_         = resolve_state::READY;
      ctx->dns_queries_timestamp_ = _highp_clock();
      auto &ep                    = ctx->endpoints_[0];
      INET_LOG("[index: %d] getaddrinfo: resolve %s succeed, ip:%s", ctx->index_,
               ctx->address_.c_str(), ep.to_string().c_str());
    }
    else
    {
      ctx->resolve_state_ = resolve_state::FAILED;
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
  resolve_thread.detach();
#else
  addrinfo hint; // 333.921ms
  memset(&hint, 0x0, sizeof(hint));

  if (this->ipsv_state_ & ipsv_ipv4)
  {
    hint.ai_family = AF_INET;
  }
  else
  {
    hint.ai_family = AF_INET6;
  }
  ::ares_getaddrinfo((ares_channel)this->ares_, ctx->address_.c_str(), nullptr, &hint,
                     ares_getaddrinfo_callback, ctx);

  ctx->deadline_timer_.expires_from_now(std::chrono::seconds(ASYNC_RESOLVE_TIMEOUT));
  ctx->deadline_timer_.async_wait([=](bool cancelled) {
    if (!cancelled)
    {
      ::ares_cancel((ares_channel)this->ares_); // It's seems not trigger socket close,
      // because ares_getaddrinfo has bug yet.
      handle_connect_failed(ctx, ERR_RESOLVE_HOST_TIMEOUT);
    }
  });

  ++this->ares_outstanding_work_;
#endif

  return false; // waiting async resolve complete.
}

bool io_service::resolve(std::vector<ip::endpoint> &endpoints, const char *hostname,
                         unsigned short port)
{
  if (this->ipsv_state_ & ipsv_ipv4)
  {
    return xxsocket::resolve_v4(endpoints, hostname, port);
  }
  else if (this->ipsv_state_ & ipsv_ipv6)
  { // localhost is IPV6 ONLY network
    return xxsocket::resolve_v6(endpoints, hostname, port) ||
           xxsocket::resolve_v4to6(endpoints, hostname, port);
  }
  return false;
}

#if _USING_ARES_LIB
void io_service::handle_ares_work_finish(channel *)
{ // Only call at event-loop thread, so no
  // need to consider thread safe.
  --this->ares_outstanding_work_;
  ++this->outstanding_work_;
}
#endif

int io_service::decode_frame_length(void *ud, int n)
{
  if (options_.lfib.length_field_offset >= 0)
  {
    if (n >= (options_.lfib.length_field_offset + options_.lfib.length_field_length))
    {
      int32_t length = -1;
      switch (options_.lfib.length_field_length)
      {
        case 4:
          length = ntohl(*reinterpret_cast<int32_t *>((unsigned char *)ud +
                                                      options_.lfib.length_field_offset)) +
                   options_.lfib.length_adjustment;
          break;
        case 3:
          length = 0;
          memcpy(&length, (unsigned char *)ud + options_.lfib.length_field_offset, 3);
          length = (ntohl(length) >> 8) + options_.lfib.length_adjustment;
          break;
        case 2:
          length = ntohs(*reinterpret_cast<uint16_t *>((unsigned char *)ud +
                                                       options_.lfib.length_field_offset)) +
                   options_.lfib.length_adjustment;
          break;
        case 1:
          length = *((unsigned char *)ud + options_.lfib.length_field_offset) +
                   options_.lfib.length_adjustment;
          break;
      }
      if (length > options_.lfib.max_frame_length)
        length = -1;
      return length;
    }
    return 0;
  }
  return n;
}

void io_service::interrupt() { interrupter_.interrupt(); }

} // namespace inet
} // namespace purelib
