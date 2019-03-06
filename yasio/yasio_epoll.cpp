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
// EPOLLET, EPOLLLT, yasio use EPOLLET mode
#include "yasio_epoll.h"
#include <limits>
#include <stdarg.h>
#include <string>

#if !defined(MICROSECONDS_PER_SECOND)
#  define MICROSECONDS_PER_SECOND 1000000LL
#endif

#define _YASIO_VERBOS_LOG 0

#if defined(_WIN32)
#  define INET_LOG(format, ...)                                                                    \
    do                                                                                             \
    {                                                                                              \
      auto content = _sfmt(("[yasio][%lld] " format "\r\n"), _highp_clock(), ##__VA_ARGS__);       \
      OutputDebugStringA(content.c_str());                                                         \
      if (options_.outf_)                                                                          \
        fprintf(options_.outf_, "%s", content.c_str());                                            \
    } while (false)
#elif defined(ANDROID) || defined(__ANDROID__)
#  include <android/log.h>
#  include <jni.h>
#  define INET_LOG(format, ...)                                                                    \
    __android_log_print(ANDROID_LOG_INFO, "yasio", ("[%lld]" format), _highp_clock(),              \
                        ##__VA_ARGS__);                                                            \
    if (options_.outf_)                                                                            \
    fprintf(options_.outf_, ("[yasio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__)
#else
#  define INET_LOG(format, ...)                                                                    \
    fprintf(stdout, ("[yasio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__);                \
    if (options_.outf_)                                                                            \
    fprintf(options_.outf_, ("[yasio][%lld] " format "\n"), _highp_clock(), ##__VA_ARGS__)
#endif

#define YASIO_SOMAXCONN 19

#define MAX_WAIT_DURATION 5 * 60 * 1000 * 1000 // 5 minites

/* max pdu buffer length, avoid large memory allocation when application layer decode a huge length
 * field. */
#define MAX_PDU_BUFFER_SIZE static_cast<int>(SZ(1, M))

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

#if defined(_WIN32)
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
#  define CC_VSNPRINTF_BUFFER_LENGTH 512
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
#  if !defined(__MINGW64__) && !defined(__MINGW32__)
  __try
  {
    RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR *)&info);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {}
#  endif
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
  class_id_ = IO_CLASS_CHANNEL;
  socket_.reset(new xxsocket());
}

void io_channel::reset()
{
  state_ = channel_state::CLOSED;

  resolve_state_         = resolve_state::FAILED;
  dns_queries_timestamp_ = 0;
  dns_queries_needed_    = false;
  endpoints_.clear();
  deadline_timer_.cancel();
}

io_service::io_service() : state_(io_service::state::IDLE), interrupter_()
{
  options_.connect_timeout_   = 5LL * MICROSECONDS_PER_SECOND;
  options_.send_timeout_      = (std::numeric_limits<int>::max)();
  options_.reconnect_timeout_ = -1;
  // Default: 10 minutes.
  options_.dns_cache_timeout_ = 600LL * MICROSECONDS_PER_SECOND;

  outstanding_work_ = 0;

  ipsv_state_ = 0;

  epoll_fd_ = epoll_create(19);

  this->xdec_len_ = [](io_service *service, void *ptr, int len) {
    return service->builtin_decode_frame_length(ptr, len);
  };
}

io_service::~io_service()
{
  stop_service();
  if (this->state_ == io_service::state::STOPPED)
    cleanup();
  if (epoll_fd_ != -1)
    close(epoll_fd_);
}

void io_service::start_service(const io_hostent *channel_eps, int channel_count,
                               io_event_callback_t cb)
{
  if (state_ == io_service::state::IDLE)
  {
    init(channel_eps, channel_count, cb);

    this->state_ = io_service::state::RUNNING;
    if (!options_.no_new_thread_)
    {
      this->worker_thread_ = std::thread(&io_service::run, this);
      this->worker_id_     = worker_thread_.get_id();
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

    for (auto ctx : channels_)
    {
      ctx->deadline_timer_.cancel();
      if (ctx->socket_->is_open())
      {
        ctx->socket_->shutdown();
      }
    }

    this->interrupt();
    this->wait_service();
  }
  else if (this->state_ == io_service::state::STOPPING)
  {
    this->wait_service();
  }
}

void io_service::wait_service()
{
  if (this->worker_thread_.joinable())
  {
    if (std::this_thread::get_id() != this->worker_id_)
    {
      this->worker_thread_.join();
      this->state_ = io_service::state::STOPPED;
      cleanup();
    }
    else
      errno = EAGAIN;
  }
}

void io_service::init(const io_hostent *channel_eps, int channel_count, io_event_callback_t cb)
{
  if (this->state_ != io_service::state::IDLE)
    return;
  if (channel_count <= 0)
    return;
  if (cb)
    this->on_event_ = std::move(cb);

  // Add the interrupter's descriptor to epoll.
  epoll_event ev = {0, {0}};
  ev.events      = EPOLLIN | EPOLLERR | EPOLLET;
  ev.data.ptr    = &interrupter_;
  epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, interrupter_.read_descriptor(), &ev);

  /* !!!Important: Make the descriptor in a ready-to-read state and relying on edge-triggered
   * notifications to make it so that we only get woken up when the descriptor's epoll registration
   * is updated. */
  interrupter_.interrupt();

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
    this->transport_free_list_.clear();
    this->transports_.clear();

    clear_channels();

    epoll_event ev = {0, {0}};
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, interrupter_.read_descriptor(), &ev);

    this->on_event_ = nullptr;

    this->state_ = io_service::state::IDLE;
  }
}

void io_service::set_option(int option, ...)
{
  va_list ap;
  va_start(ap, option);

  switch (option)
  {
    case YASIO_OPT_CONNECT_TIMEOUT:
      options_.connect_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YASIO_OPT_SEND_TIMEOUT:
      options_.send_timeout_ = static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YASIO_OPT_RECONNECT_TIMEOUT:
    {
      int value = va_arg(ap, int);
      if (value > 0)
        options_.reconnect_timeout_ = static_cast<highp_time_t>(value) * MICROSECONDS_PER_SECOND;
      else
        options_.reconnect_timeout_ = -1; // means auto reconnect is disabled.
    }
    break;
    case YASIO_OPT_DNS_CACHE_TIMEOUT:
      options_.dns_cache_timeout_ =
          static_cast<highp_time_t>(va_arg(ap, int)) * MICROSECONDS_PER_SECOND;
      break;
    case YASIO_OPT_DEFER_EVENT:
      options_.deferred_event_ = !!va_arg(ap, int);
      break;
    case YASIO_OPT_TCP_KEEPALIVE:
      options_.tcp_keepalive_.onoff    = 1;
      options_.tcp_keepalive_.idle     = va_arg(ap, int);
      options_.tcp_keepalive_.interval = va_arg(ap, int);
      options_.tcp_keepalive_.probs    = va_arg(ap, int);
      break;
    case YASIO_OPT_RESOLV_FUNCTION:
      this->xresolv_ = std::move(*va_arg(ap, resolv_fn_t *));
      break;
    case YASIO_OPT_LOG_FILE:
      if (options_.outf_)
        fclose(options_.outf_);
      options_.outf_ = fopen(va_arg(ap, const char *), "wb");
      break;
    case YASIO_OPT_LFBFD_PARAMS:
      options_.lfb_.max_frame_length    = va_arg(ap, int);
      options_.lfb_.length_field_offset = va_arg(ap, int);
      options_.lfb_.length_field_length = va_arg(ap, int);
      options_.lfb_.length_adjustment   = va_arg(ap, int);
      break;
    case YASIO_OPT_IO_EVENT_CALLBACK:
      this->on_event_ = std::move(*va_arg(ap, io_event_callback_t *));
      break;
    case YASIO_OPT_DECODE_FRAME_LENGTH_FUNCTION:
      this->xdec_len_ = std::move(*va_arg(ap, decode_frame_length_fn_t *));
      break;
    case YASIO_OPT_CHANNEL_LOCAL_PORT:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        this->channels_[index]->local_port_ = (u_short)va_arg(ap, int);
      }
    }
    break;
    case YASIO_OPT_CHANNEL_REMOTE_HOST:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        this->channels_[index]->host_ = va_arg(ap, const char *);
      }
    }
    break;
    case YASIO_OPT_CHANNEL_REMOTE_PORT:
    {
      auto index = static_cast<size_t>(va_arg(ap, int));
      if (index < this->channels_.size())
      {
        this->channels_[index]->port_ = (u_short)va_arg(ap, int);
      }
    }
    break;
    case YASIO_OPT_NO_NEW_THREAD:
      this->options_.no_new_thread_ = !!va_arg(ap, int);
      break;
  }

  va_end(ap);
}

io_channel *io_service::new_channel(const io_hostent &ep)
{
  auto ctx = new io_channel(*this);
  ctx->reset();
  ctx->host_  = ep.host_;
  ctx->port_  = ep.port_;
  ctx->index_ = static_cast<int>(this->channels_.size());
  update_resolve_state(ctx);
  this->channels_.push_back(ctx);
  return ctx;
}

void io_service::clear_channels()
{
  this->active_channels_.clear();
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

  std::lock_guard<std::recursive_mutex> lck(this->event_queue_mtx_);
  do
  {
    auto event = std::move(this->event_queue_.front());
    this->event_queue_.pop_front();
    this->on_event_(std::move(event));
  } while (!this->event_queue_.empty() && --count > 0);
}

void io_service::run()
{ // The async event-loop
  // Set Thread Name: yasio async socket io
  _set_thread_name("yasio-evloop");

  // Call once at startup
  this->ipsv_state_ = xxsocket::getipsv();

  // event loop
  for (; this->state_ == io_service::state::RUNNING;)
  {
    epoll_event events[128] = {0};
    int nfds                = do_evpoll(events, _ARRAYSIZE(events));

    if (this->state_ != io_service::state::RUNNING)
      break;

    if (nfds == -1)
    {
      int ec = xxsocket::get_last_errno();
      if (ec == EBADF)
      {
        INET_LOG("do_evpoll failed, ec:%d, detail:%s, the service will exit!\n", ec,
                 io_service::strerror(ec));
        goto _L_end;
      }
      INET_LOG("do_evpoll waked up improperly, ec:%d, detail:%s\n", ec, io_service::strerror(ec));
      continue; // just continue.
    }

    if (nfds == 0)
    {
#if !_YASIO_VERBOS_LOG
      (void)0;
#else
      INET_LOG("%s", "do_evpoll is timeout!");

#endif
    }
    // Reset the interrupter.
    else if (nfds > 0)
    {
      for (int i = 0; i < nfds; ++i)
      {
        auto &event = events[i];
        if (event.data.ptr == &this->interrupter_ || event.data.ptr == nullptr)
        {
          // No need to reset the interrupter since we're leaving the descriptor
          // in a ready-to-read state and relying on edge-triggered notifications
          // to make it so that we only get woken up when the descriptor's epoll
          // registration is updated.
          continue;
        }

        io_base *ud = (io_base *)event.data.ptr;
        if (ud->class_id_ == IO_CLASS_TRANSPORT)
        {
          perform_io(this->transports_[static_cast<io_transport *>(ud)->index_], event);
        }
        else if (ud->class_id_ == IO_CLASS_CHANNEL)
        {
          perform_channel_completion(static_cast<io_channel *>(ud), event);
        }
        // else, unknown object
      }
    }

    // update active channels
    if (!active_channels_.empty())
    {
      std::lock_guard<std::recursive_mutex> lck(active_channels_mtx_);
      for (auto iter = active_channels_.begin(); iter != active_channels_.end();)
      {
        auto ctx = *iter;
        if (ctx->state_ == channel_state::OPENING)
          perform_channel(ctx);

        if (ctx->resolve_state_ != resolve_state::INPRROGRESS)
          iter = active_channels_.erase(iter);
        else
          ++iter;
      }
    }

    // update timers
    perform_timers();
  }

_L_end:
  (void)0; // ONLY for xcode compiler happy.
}

void io_service::perform_channel(io_channel *ctx)
{
  assert(ctx->state_ == channel_state::OPENING);
  if (ctx->type_ & CHANNEL_CLIENT)
  {
    do_nonblocking_connect(ctx);
  }
  else if (ctx->type_ & CHANNEL_SERVER)
  {
    do_nonblocking_accept(ctx);
  }
}

void io_service::perform_channel_completion(io_channel *ctx, const epoll_event &event)
{ // client: OPENING, server: OPENED
  if (ctx->type_ & CHANNEL_CLIENT)
  {
    do_nonblocking_connect_completion(ctx, event);
  }
  else if (ctx->type_ & CHANNEL_SERVER)
  {
    do_nonblocking_accept_completion(ctx, event);
  }
}

void io_service::perform_io(transport_ptr transport, const epoll_event &event)
{
  int n          = -1;
  epoll_event ev = {0, {0}};
  if (transport->offset_ > 0 || (event.events & (YASIO_EPOLLIN)))
  {
#if _YASIO_VERBOS_LOG
    INET_LOG("[index: %d] perform non-blocking read operation...", transport->channel_index());
#endif
    n = do_read(transport);
    if (n < 0)
    {
      handle_close(transport);
      return;
    }
    else if (n > 0)
    {
      ev.events = EPOLLIN;
    }
  }

  // perform write operations
  if (!transport->send_queue_.empty())
  {
    transport->send_queue_mtx_.lock();
#if _YASIO_VERBOS_LOG
    INET_LOG("[index: %d] perform non-blocking write operation...", transport->channel_index());
#endif

    if (!do_write(transport))
    { // TODO: check would block? for client, may
      // be unnecessary.
      transport->send_queue_mtx_.unlock();
      handle_close(transport);
      return;
    }

    if (!transport->send_queue_.empty())
    {
      ev.events |= EPOLLOUT;
    }

    transport->send_queue_mtx_.unlock();
  }

  if (ev.events != 0)
  {
    ev.data.ptr = transport.get();
    outstanding_events_.push_back(ev);
    ++this->outstanding_work_;
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
  if (ctx->state_ != channel_state::CLOSED)
  {
    ctx->state_ = channel_state::CLOSED;
    unregister_descriptor(ctx->socket_->native_handle(), YASIO_EPOLLIN, ctx);
    ctx->socket_->close();
    this->interrupt();
  }
}

void io_service::close(transport_ptr transport)
{
  if (transport->is_open())
  {
    INET_LOG("close the transport: %s --> %s",
             transport->socket_->local_endpoint().to_string().c_str(),
             transport->socket_->peer_endpoint().to_string().c_str());
    transport->offset_ = 1; // !IMPORTANT, trigger the close immidlately.
    transport->socket_->shutdown();
    transport.reset();
    this->interrupt();
  }
}

bool io_service::is_open(size_t channel_index) const
{
  // Gets channel
  if (channel_index >= channels_.size())
    return false;
  auto ctx = channels_[channel_index];
  return ctx->state_ == channel_state::OPENED;
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
  if (channel_type & CHANNEL_TCP)
    ctx->protocol_ = SOCK_STREAM;
  else if (channel_type & CHANNEL_UDP)
    ctx->protocol_ = SOCK_DGRAM;

  open_internal(ctx);
}

void io_service::handle_close(transport_ptr transport)
{
  INET_LOG("the connection %s --> %s is lost, error:%d, detail:%s",
           transport->local_endpoint().to_string().c_str(),
           transport->peer_endpoint().to_string().c_str(), transport->error_,
           io_service::strerror(transport->error_));

  close_internal(transport.get());

  auto ctx = transport->ctx_;

  // @notify connection lost
  this->handle_event(event_ptr(
      new io_event(ctx->index_, YASIO_EVENT_CONNECTION_LOST, transport->error_, transport)));

  if (ctx->type_ == CHANNEL_TCP_CLIENT)
  {
    if (ctx->state_ != channel_state::OPENING)
      ctx->state_ = channel_state::CLOSED;
    if (options_.reconnect_timeout_ > 0)
    { // !Auto reconnect support.
      std::shared_ptr<deadline_timer> timer(new deadline_timer(*this));
      timer->expires_from_now(std::chrono::microseconds(options_.reconnect_timeout_));
      timer->async_wait(
          [this, ctx, timer /*!important, hold on by lambda expression */](bool cancelled) {
            if (!cancelled)
              this->open_internal(ctx);
          });
    }
  }

  // @we don't real delete it, just push to free list, make sure all objects' index in transports_
  // never change.
  this->transport_free_list_.push_back(transport.get());
}

void io_service::register_descriptor(const socket_native_type fd, unsigned int events, io_base *ud)
{
  epoll_event ev = {ud->registered_events_, {0}};
  ev.data.ptr    = ud;
  ev.events |= events;

  if (ud->registered_events_ == 0)
  {
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
  }
  else
  {
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
  }
  ud->registered_events_ = ev.events;
}

void io_service::unregister_descriptor(const socket_native_type fd, unsigned int events,
                                       io_base *ud)
{
  epoll_event ev = {ud->registered_events_, {0}};
  ev.data.ptr    = ud;

  ev.events &= ~(events);

  if (ev.events != 0)
  {
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
  }
  else
  {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &ev);
  }

  ud->registered_events_ = ev.events;
}

void io_service::write(transport_ptr transport, std::vector<char> data)
{
  this->write(transport.get(), std::move(data));
}
void io_service::write(io_transport *transport, std::vector<char> data)
{
  if (transport && transport->socket_->is_open())
  {
    auto pdu =
        a_pdu_ptr(new a_pdu(std::move(data), std::chrono::microseconds(options_.send_timeout_)));

    transport->send_queue_mtx_.lock();
    transport->send_queue_.push_back(pdu);
    transport->send_queue_mtx_.unlock();

    register_descriptor(transport->socket_->native_handle(), YASIO_EPOLLOUT, transport);
  }
  else
  {
    INET_LOG("[transport: %p] send failed, the connection not ok!", transport);
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
  {
    this->on_event_(std::move(event));
  }
}

void io_service::do_nonblocking_connect(io_channel *ctx)
{
  assert(ctx->state_ == channel_state::OPENING);

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
      INET_LOG("[index: %d] connecting server %s:%u...", ctx->index_, ctx->host_.c_str(),
               ctx->port_);
    }
    else
    {
      close_internal(ctx);
      INET_LOG("[index: %d] reconnecting server %s:%u...", ctx->index_, ctx->host_.c_str(),
               ctx->port_);
    }

    auto &ep = ctx->endpoints_[0];
    if (ctx->type_ & CHANNEL_TCP)
    {
      int ret = -1;
      if (ctx->socket_->open(ep.af()))
      {
        ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
        if (ctx->local_port_ != 0)
          ctx->socket_->bind("0.0.0.0", ctx->local_port_);
        ret = xxsocket::connect_n(ctx->socket_->native_handle(), ep);
      }

      if (ret < 0)
      { // setup no blocking connect
        auto last_error = ctx->update_error();
        if (last_error != EINPROGRESS && last_error != EWOULDBLOCK)
        {
          this->handle_connect_failed(ctx);
        }
        else
        {
          // register connect socket with connect channel.
          register_descriptor(ctx->socket_->native_handle(), YASIO_EPOLLIN | YASIO_EPOLLOUT, ctx);

          ctx->deadline_timer_.expires_from_now(
              std::chrono::microseconds(options_.connect_timeout_));
          ctx->deadline_timer_.async_wait([this, ctx](bool cancelled) {
            if (!cancelled && ctx->state_ != channel_state::OPENED)
            {
              ctx->update_error(ETIMEDOUT);
              handle_connect_failed(ctx);
            }
          });
        }
      }
      else if (ret == 0)
      { // connect server succed immidiately.
        handle_connect_succeed(ctx, ctx->socket_);
      } // NEVER GO HERE
    }
    else // CHANNEL_UDP
    {
      int ret = -1;
      if (ctx->socket_->open(ipsv_state_ & ipsv_ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0))
      {
        ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

        ctx->socket_->bind("0.0.0.0", ctx->local_port_);
        ret = xxsocket::connect(ctx->socket_->native_handle(), ep);
        if (ret == 0)
        {
          ctx->socket_->set_nonblocking(true);
          handle_connect_succeed(ctx, ctx->socket_);
        }
        else
        {
          ctx->update_error();
          this->handle_connect_failed(ctx);
        }
      }
    }
  }
  else if (ctx->resolve_state_ == resolve_state::FAILED)
  {
    ctx->update_error(ERR_RESOLVE_HOST_FAILED);
    handle_connect_failed(ctx);
  } // DIRTY,Try resolve address nonblocking
  else if (ctx->resolve_state_ == resolve_state::DIRTY)
  {
    start_resolve(ctx);
  }
}

void io_service::do_nonblocking_connect_completion(io_channel *ctx, const epoll_event &event)
{ // only tcp client will call this function
  assert(ctx->type_ == CHANNEL_TCP_CLIENT);
  assert(ctx->state_ == channel_state::OPENING);
  int error = -1;
  if (event.events & (YASIO_EPOLLIN | YASIO_EPOLLOUT))
  {
    socklen_t len = sizeof(error);
    if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
            0 &&
        error == 0)
    {
      unregister_descriptor(ctx->socket_->native_handle(), YASIO_EPOLLOUT, ctx);
      handle_connect_succeed(ctx, ctx->socket_);
    }
    else
    {
      ctx->update_error(ERR_CONNECT_FAILED);
      handle_connect_failed(ctx);
    }

    ctx->deadline_timer_.cancel();
  }
}

void io_service::do_nonblocking_accept(io_channel *ctx)
{ // channel is server
  close_internal(ctx);

  ip::endpoint ep(ipsv_state_ & ipsv_ipv4 ? "0.0.0.0" : "::", ctx->port_);

  if (ctx->socket_->open(ipsv_state_ & ipsv_ipv4 ? AF_INET : AF_INET6, ctx->protocol_))
  {
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);

    int error = 0;
    if (ctx->socket_->bind(ep) != 0)
    {
      error = xxsocket::get_last_errno();
      INET_LOG("[index: %d] bind failed, ec:%d, detail:%s", ctx->index_, error,
               io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = channel_state::CLOSED;
      return;
    }

    if ((ctx->type_ & CHANNEL_UDP) || ctx->socket_->listen(YASIO_SOMAXCONN) == 0)
    {
      ctx->state_ = channel_state::OPENED;
      ctx->socket_->set_nonblocking(true);
      register_descriptor(ctx->socket_->native_handle(), YASIO_EPOLLIN, ctx);
      INET_LOG("[index: %d] listening at %s...", ctx->index_, ep.to_string().c_str());
    }
    else
    {
      error = xxsocket::get_last_errno();
      INET_LOG("[index: %d] listening failed, ec:%d, detail:%s", ctx->index_, error,
               io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = channel_state::CLOSED;
    }
  }
}

void io_service::do_nonblocking_accept_completion(io_channel *ctx, const epoll_event &event)
{
  if (ctx->state_ == channel_state::OPENED)
  {
    int error = -1;
    if (event.events & YASIO_EPOLLIN)
    {
      socklen_t len = sizeof(error);
      if (::getsockopt(ctx->socket_->native_handle(), SOL_SOCKET, SO_ERROR, (char *)&error, &len) >=
              0 &&
          error == 0)
      {
        if (ctx->type_ & CHANNEL_TCP)
        {
          std::shared_ptr<xxsocket> client_sock(new xxsocket(ctx->socket_->accept()));
          if (client_sock->is_open())
          {
            handle_connect_succeed(ctx, std::move(client_sock));
          }
          else
            INET_LOG("%s", "tcp-server: accept client socket fd failed!");
        }
        else // CHANNEL_UDP
        {
          ip::endpoint peer;

          char buffer[65535];
          int n = ctx->socket_->recvfrom_i(buffer, sizeof(buffer), peer);
          if (n > 0)
          {
            INET_LOG("udp-server: recvfrom peer: %s", peer.to_string().c_str());

            // make a transport local --> peer udp session, just like tcp accept
            std::shared_ptr<xxsocket> client_sock(new xxsocket());
            if (client_sock->open(ipsv_state_ & ipsv_ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0))
            {
              client_sock->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
              client_sock->set_optval(SOL_SOCKET, SO_REUSEPORT, 1);

              error = client_sock->bind("0.0.0.0", ctx->port_) == 0
                          ? xxsocket::connect(client_sock->native_handle(), peer)
                          : -1;
              if (error == 0)
              {
                auto transport = allocate_transport(ctx, std::move(client_sock));
                handle_connect_succeed(transport);
                this->handle_event(
                    event_ptr(new io_event(transport->channel_index(), YASIO_EVENT_RECV_PACKET,
                                           std::vector<char>(buffer, buffer + n), transport)));
              }
              else
              {
                INET_LOG("%s", "udp-server: open socket fd failed!");
              }
            }
          }
        }
      }
      else
      {
        INET_LOG("The channel:%d has error, will be closed!", ctx->index_);
        close_internal(ctx);
      }
    }
  }
}

void io_service::handle_connect_succeed(io_channel *ctx, std::shared_ptr<xxsocket> socket)
{
  handle_connect_succeed(allocate_transport(ctx, std::move(socket)));
}

void io_service::handle_connect_succeed(transport_ptr transport)
{
  auto ctx = transport->ctx_;

  auto &connection = transport->socket_;

  if (ctx->type_ & CHANNEL_CLIENT)
  {
    ctx->state_ = channel_state::OPENED;
  }
  else
  { // new client income
    connection->set_nonblocking(true);
    register_descriptor(connection->native_handle(), YASIO_EPOLLIN, transport.get());
  }

  if ((ctx->type_ & CHANNEL_TCP) && options_.tcp_keepalive_.onoff)
  { // apply tcp keepalive options
    connection->set_keepalive(options_.tcp_keepalive_.idle, options_.tcp_keepalive_.interval,
                              options_.tcp_keepalive_.probs);
  }

  INET_LOG("[index: %d] the connection [%s] ---> %s is established.", ctx->index_,
           connection->local_endpoint().to_string().c_str(),
           connection->peer_endpoint().to_string().c_str());

  this->handle_event(
      event_ptr(new io_event(ctx->index_, YASIO_EVENT_CONNECT_RESPONSE, 0, transport)));
}

void io_service::handle_connect_failed(io_channel *ctx)
{
  close_internal(ctx);

  ctx->state_ = channel_state::CLOSED;

  int error = ctx->error_;
  this->handle_event(
      event_ptr(new io_event(ctx->index_, YASIO_EVENT_CONNECT_RESPONSE, error, nullptr)));

  INET_LOG("[index: %d] connect server %s:%u failed, ec:%d, detail:%s", ctx->index_,
           ctx->host_.c_str(), ctx->port_, error, io_service::strerror(error));
}

transport_ptr io_service::allocate_transport(io_channel *ctx, std::shared_ptr<xxsocket> socket)
{
  transport_ptr transport;
  if (!transport_free_list_.empty())
  { // allocate from free list, and do not need push to transports_ again.
    auto index      = transport_free_list_.back()->index_;
    transport       = transports_[index];
    transport->ctx_ = ctx; // !important, update context.
    transport_free_list_.pop_back();
    INET_LOG("allocate a transport from free list, index=%d, channel_index=%d", index, ctx->index_);
  }
  else
  {
    transport.reset(new io_transport(ctx));
    transport->index_ = static_cast<int>(this->transports_.size());
    this->transports_.push_back(transport);
  }

  transport->socket_ = socket;

  return transport;
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
#if _YASIO_VERBOS_LOG
        auto packet_size = static_cast<int>(v->data_.size());
        INET_LOG("[index: %d] do_write ok, A packet sent "
                 "success, packet size:%d",
                 ctx->index_, packet_size, transport->socket_->local_endpoint().to_string().c_str(),
                 transport->socket_->peer_endpoint().to_string().c_str());
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
        int error = transport->update_error();
        if (SHOULD_CLOSE_1(n, error))
        {
          INET_LOG("[index: %d] do_write error, the connection "
                   "should be "
                   "closed, retval=%d, ec:%d, detail:%s",
                   ctx->index_, n, error, io_service::strerror(error));
          break;
        }
      }
    }

    bRet = true;
  } while (false);

  return bRet;
}

void io_service::handle_send_finished(a_pdu_ptr /*pdu*/, error_number /*error*/) {}

int io_service::do_read(transport_ptr transport)
{
  // return value: Indicate whether still have avail data in kernel or user layer's recv buffer. <
  // 0, the connection should be closed.
  auto ctx = transport->ctx_;

  if (!transport->socket_->is_open())
    return -1;

  int n = transport->socket_->recv_i(transport->buffer_ + transport->offset_,
                                     socket_recv_buffer_size - transport->offset_);

  if (n > 0 || !SHOULD_CLOSE_0(n, transport->update_error()))
  {
#if _YASIO_VERBOS_LOG
    INET_LOG("[index: %d] do_read status ok, ec:%d, detail:%s", transport->channel_index(),
             transport->error_, io_service::strerror(transport->error_));
#endif
    if (n == -1)
      n = 0;
#if _YASIO_VERBOS_LOG
    if (n > 0)
    {
      INET_LOG("[index: %d] do_read ok, received data len: %d, "
               "buffer data "
               "len: %d",
               transport->channel_index(), n, n + transport->offset_);
    }
#endif
    if (transport->expected_packet_size_ == -1)
    { // decode length
      int length = this->xdec_len_(this, transport->buffer_, transport->offset_ + n);
      if (length > 0)
      {
        transport->expected_packet_size_ = length;
        transport->expected_packet_.reserve(
            (std::min)(transport->expected_packet_size_,
                       MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid // memory reallocte.
        return do_unpack(transport, transport->expected_packet_size_, n);
      }
      else if (length == 0)
      {
        // header insufficient, wait readfd ready at
        // next event step.
        transport->offset_ += n;
        return n;
      }
      else
      {
        // set_errorno(ctx, error_number::ERR_DPL_ILLEGAL_PDU);
        INET_LOG("[index: %d] do_read error, decode length of "
                 "pdu failed, "
                 "the connection should be closed!",
                 ctx->index_);
        return -1;
      }
    }
    else
    { // process incompleted pdu
      return do_unpack(transport,
                       transport->expected_packet_size_ -
                           static_cast<int>(transport->expected_packet_.size()),
                       n);
    }
  }
  else
  {
    int error            = transport->error_;
    const char *errormsg = io_service::strerror(error);
    if (n == 0)
    {
      INET_LOG("[index: %d] do_read error, the remote host close the "
               "connection, retval=%d, ec:%d, detail:%s",
               ctx->index_, n, error, errormsg);
    }
    else
    {
      INET_LOG("[index: %d] do_read error, the connection should be "
               "closed, retval=%d, ec:%d, detail:%s",
               ctx->index_, n, error, errormsg);
    }

    return -1;
  }
}

int io_service::do_unpack(transport_ptr transport, int bytes_expected, int bytes_transferred)
{
  auto &offset         = transport->offset_;
  auto bytes_available = bytes_transferred + offset;
  transport->expected_packet_.insert(transport->expected_packet_.end(), transport->buffer_,
                                     transport->buffer_ +
                                         (std::min)(bytes_expected, bytes_available));

  offset = bytes_available - bytes_expected; // set offset to bytes of remain buffer
  if (offset >= 0)
  {                 // pdu received properly
    if (offset > 0) // move remain data to head of buffer and hold offset.
    {
      ::memmove(transport->buffer_, transport->buffer_ + bytes_expected, offset);
      // not all data consumed, so add events for this context
    }
    // move properly pdu to ready queue, the other thread who care about will retrieve
    // it.
#if _YASIO_VERBOS_LOG
    INET_LOG("[index: %d] received a properly packet from peer, "
             "packet size:%d",
             transport->channel_index(), transport->expected_packet_size_);
#endif
    this->handle_event(event_ptr(new io_event(transport->channel_index(), YASIO_EVENT_RECV_PACKET,
                                              transport->take_packet(), transport)));
  }
  else
  { // all buffer consumed, set offset to ZERO, pdu
    // incomplete, continue recv remain data.
    offset = 0;
  }

  return offset;
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
    this->interrupt();
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
  if (ctx->state_ != channel_state::CLOSED)
  {
    INET_LOG("[index: %d] can't open the channel again!", ctx->index_);
    return;
  }

  if (ctx->resolve_state_ != resolve_state::READY)
    update_resolve_state(ctx);

  ctx->state_ = channel_state::OPENING;
  if (ctx->socket_->is_open())
  {
    ctx->socket_->shutdown();
  }

  active_channels_mtx_.lock();
  this->active_channels_.push_back(ctx);
  active_channels_mtx_.unlock();

  this->interrupt();
}

void io_service::perform_timers()
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

int io_service::do_evpoll(epoll_event *events, int n)
{
  /*
@Optimize, swap nfds, make sure do_read & do_write event chould
be perform when no need to call epoll_wait However, the
connection exception will detected through do_read or do_write,
but it's ok.
*/
  int nfds = 0;
  std::swap(nfds, this->outstanding_work_);

  if (nfds <= 0)
  {
    auto wait_duration = get_wait_duration(MAX_WAIT_DURATION) / 1000;
    if (wait_duration > 0)
    {
      nfds = ::epoll_wait(epoll_fd_, events, n, static_cast<long>(wait_duration));

#if _YASIO_VERBOS_LOG
      INET_LOG("epoll_wait waked up, retval=%d", nfds);
#endif
    }
    else
    {
      nfds = static_cast<int>(channels_.size()) << 1;
    }
  }
  else
  {
    if (!outstanding_events_.empty())
    {
      nfds       = (std::min)(static_cast<int>(outstanding_events_.size()), n);
      auto endit = std::next(outstanding_events_.begin(), nfds);
      std::copy(outstanding_events_.begin(), endit, events);
      outstanding_events_.erase(outstanding_events_.begin(), endit);
    }
    else
      nfds = 0;
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
    unregister_descriptor(ctx->socket_->native_handle(), YASIO_EPOLLIN | YASIO_EPOLLOUT, ctx);
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
    ctx->dns_queries_needed_ = !ep.assign(ctx->host_.c_str(), ctx->port_);
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

void io_service::start_resolve(io_channel *ctx)
{ // Only call at event-loop thread, so
  // no need to consider thread safe.
  if (ctx->resolve_state_ != resolve_state::DIRTY)
    return;
  ctx->resolve_state_ = resolve_state::INPRROGRESS;
  ctx->endpoints_.clear();

  INET_LOG("[index: %d] start async resolving for %s", ctx->index_, ctx->host_.c_str());
  // 6.563ms
  std::thread resolve_thread([=] {
    addrinfo hint;
    memset(&hint, 0x0, sizeof(hint));

    bool succeed = xresolv_ ? xresolv_(ctx->endpoints_, ctx->host_.c_str(), ctx->port_)
                            : resolve(ctx->endpoints_, ctx->host_.c_str(), ctx->port_);

    if (succeed && !ctx->endpoints_.empty())
    {
      ctx->resolve_state_         = resolve_state::READY;
      ctx->dns_queries_timestamp_ = _highp_clock();
      auto &ep                    = ctx->endpoints_[0];
      INET_LOG("[index: %d] getaddrinfo: resolve %s succeed, ip:%s", ctx->index_,
               ctx->host_.c_str(), ep.to_string().c_str());
    }
    else
    {
      ctx->resolve_state_ = resolve_state::FAILED;
    }

    this->interrupt();
  });
  resolve_thread.detach();
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

int io_service::builtin_decode_frame_length(void *ud, int n)
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

void io_service::interrupt()
{
  epoll_event ev = {0, {0}};
  ev.events      = EPOLLIN | EPOLLERR | EPOLLET;
  ev.data.ptr    = &interrupter_;
  epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, interrupter_.read_descriptor(), &ev);
}

const char *io_service::strerror(int error)
{
  switch (error)
  {
    case ERR_CONNECT_FAILED:
      return "connect failed!";
    case ERR_CONNECT_TIMEOUT:
      return "connect timeout!";
    case ERR_SEND_FAILED:
      return "send failed!";
    case ERR_SEND_TIMEOUT:
      return "send timeout!";
    case ERR_RECV_FAILED:
      return "recv failed!";
    case ERR_NETWORK_UNREACHABLE:
      return "the network unreachable!";
    case ERR_CONNECTION_LOST:
      return "the connection lost!";
    case ERR_DPL_ILLEGAL_PDU:
      return "decode frame length failed!";
    case ERR_RESOLVE_HOST_FAILED:
      return "resolve host failed!";
    case ERR_RESOLVE_HOST_TIMEOUT:
      return "resolve host timeout!";
    case ERR_RESOLVE_HOST_IPV6_REQUIRED:
      return "resolve host ipv6 required!";
    default:
      return xxsocket::strerror(error);
  }
}

} // namespace inet
} // namespace purelib
