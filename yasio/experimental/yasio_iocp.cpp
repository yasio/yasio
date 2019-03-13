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
#include "yasio_iocp.h"
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
    auto content = _sfmt(("[yasio][%lld] " format "\r\n"), _highp_clock(), ##__VA_ARGS__);         \
    YASIO_DEBUG_PRINT(content.c_str());                                                            \
    if (options_.outf_ != -1)                                                                      \
    {                                                                                              \
      if (::lseek(options_.outf_, 0, SEEK_CUR) > options_.outf_max_size_)                          \
        ::ftruncate(options_.outf_, 0), ::lseek(options_.outf_, 0, SEEK_SET);                      \
      ::write(options_.outf_, content.c_str(), content.size());                                    \
    }                                                                                              \
  } while (false)

namespace purelib
{
namespace inet
{
namespace
{
// channel state
enum : u_short
{
  YCS_CLOSED,
  YCS_OPENING,
  YCS_OPENED,
};

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

#define YDQS_CHECK_STATE(what, value) ((what & 0x00ff) == value)
#define YDQS_SET_STATE(what, value) (what = (what & 0xff00) | value)
#define YDQS_GET_STATE(what) (what & 0x00ff)

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

enum
{
  // Timeout to use with GetQueuedCompletionStatus on older versions of
  // Windows. Some versions of windows have a "bug" where a call to
  // GetQueuedCompletionStatus can appear stuck even though there are events
  // waiting on the queue. Using a timeout helps to work around the issue.
  default_gqcs_timeout = 500,

  // Maximum waitable timer timeout, in milliseconds.
  max_timeout_msec = 5 * 60 * 1000,

  // Maximum waitable timer timeout, in microseconds.
  max_timeout_usec = max_timeout_msec * 1000,

  // Wake for dispatch timer or unpack
  wake_for_dispatch = 1001,

  // Wake for open channel
  wake_for_open,

  // Wake for connect
  wake_for_write,
};
static DWORD get_gqcs_timeout()
{
  OSVERSIONINFOEX osvi;
  ZeroMemory(&osvi, sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  osvi.dwMajorVersion      = 6ul;

  const uint64_t condition_mask = ::VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);

  if (!!::VerifyVersionInfo(&osvi, VER_MAJORVERSION, condition_mask))
    return INFINITE;

  return default_gqcs_timeout;
}
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
    this->service_.interrupt(wake_for_dispatch);
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

io_service::io_service() : state_(io_service::state::IDLE)
{
  iocp_.handle = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, DWORD(~0));

  gqcs_timeout_ = get_gqcs_timeout();

  this->decode_len_ = [=](void *ptr, int len) { return this->__builtin_decode_len(ptr, len); };
  this->resolv_     = [=](std::vector<ip::endpoint> &eps, const char *host, unsigned short port) {
    return this->__builtin_resolv(eps, host, port);
  };
}

io_service::~io_service()
{
  stop_service();
  if (this->state_ == io_service::state::STOPPED)
    cleanup();
}

void io_service::start_service(const io_hostent *channel_eps, int channel_count, io_event_cb_t cb)
{
  if (state_ == io_service::state::IDLE)
  {
    init(channel_eps, channel_count, cb);

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

    for (auto ctx : channels_)
    {
      ctx->deadline_timer_.cancel();
      if (ctx->socket_->is_open())
      {
        ctx->socket_->shutdown();
      }
    }

    this->interrupt(wake_for_dispatch);
    this->wait_service();
  }
  else if (this->state_ == io_service::state::STOPPING)
  {
    this->wait_service();
  }
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
    this->transports_pool_.clear();
    this->transports_.clear();

    clear_channels();

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
  for (auto iter = channels_.begin(); iter != channels_.end();)
  {
    (*iter)->socket_->close();
    delete *(iter);
    iter = channels_.erase(iter);
  }
}

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
  this->ipsv_ = xxsocket::getipsv();

  // event loop
  for (; this->state_ == io_service::state::RUNNING;)
  {
    // Get the next operation from the queue.
    DWORD bytes_transferred   = 0;
    DWORD_PTR completion_key  = 0;
    LPOVERLAPPED lpOverlapped = nullptr;
    ::SetLastError(0);
    auto msec = get_wait_duration(MAX_WAIT_DURATION) / 1000;
    BOOL ok   = ::GetQueuedCompletionStatus(
        iocp_.handle, &bytes_transferred, &completion_key, &lpOverlapped,
        static_cast<DWORD>(msec < gqcs_timeout_ ? msec : gqcs_timeout_));

    if (this->state_ != io_service::state::RUNNING)
      break;

    if (lpOverlapped)
    { // process completeion events.
      auto ctx = (io_base *)lpOverlapped;
      ctx->update_error();

      if (completion_key == 0)
      {
        if (ctx->state_ == YCS_OPENED)
        { // process
          ctx->OffsetHigh = bytes_transferred;
          do_recv_complete(static_cast<io_transport *>(ctx));
        }
        else if (ctx->state_ == YCS_OPENING)
        {
          do_open_complete(ctx);
        }
      }
      else if (completion_key == wake_for_open)
      {
        do_open(ctx);
      }
      else if (completion_key == wake_for_write)
      {
        do_write(static_cast<io_transport*>(ctx));
      }
    }

    // perform timeout timers
    perform_timers();
  }
}

void io_service::do_open(io_base *ptr)
{
  auto ctx = (io_channel *)ptr;
  assert(ctx->state_ == YCS_OPENING);
  if (ctx->mask_ & YCM_CLIENT)
  {
    switch (this->update_dns_queries_state(ctx, false))
    {
      case YDQS_READY:
        do_connect(ctx);
        break;
      case YDQS_FAILED:
        YASIO_LOG("[index: %d] getaddrinfo failed, ec:%d, detail:%s", ctx->index_, ctx->error_,
                  xxsocket::gai_strerror(ctx->error_));
        handle_connect_failed(ctx, YERR_RESOLV_HOST_FAILED);
        break;
      default:; // YDQS_INPRROGRESS
    }
  }
  else if (ctx->mask_ & YCM_SERVER)
  {
    do_accept(ctx);
  }
}

void io_service::do_open_complete(io_base *overlap)
{ // client: OPENING, server: OPENED

  auto impl = (io_transport *)overlap;
  auto ctx  = impl->ctx_;
  if (ctx->mask_ & YCM_CLIENT)
  {
    do_connect_complete(static_cast<io_transport*>(overlap));
  }
  else if (ctx->mask_ & YCM_SERVER)
  {
    if (ctx->error_ == 0)
    {
      do_accept_complete(static_cast<io_transport *>(overlap));
    }
    else if (ctx->error_ == ERROR_OPERATION_ABORTED)
    {
      YASIO_LOG(
          "The tcp server channel: %d has been closed, reason:%s, it will be never accept new "
          "client connection!",
          ctx->index_, this->strerror(overlap->error_));
    }
  }
}

void io_service::do_recv_complete(io_transport *transport)
{
  auto n = do_unpack(transport);
  if (n < 0)
  {
    handle_close(transport);
    return;
  }
  else if (n > 0)
  { // still have data to perform at next frame
    this->interrupt(0, transport);
  }
  else // == 0, all buffer consumed, continue recv
  {
    start_receive_op(transport);
  }
}

#if 0
void io_service::do_io_completion(transport_ptr transport, DWORD completion_key)
{
  int n                  = -1;
  auto bytes_transferred = transport->OffsetHigh;
  if (transport->offset_ > 0 || bytes_transferred > 0 || completion_key == 0)
  {
#if _YASIO_VERBOS_LOG
    YASIO_LOG("[index: %d] perform non-blocking read operation...", transport->channel_index());
#endif
    n = do_read(transport, static_cast<int>(bytes_transferred));
    if (n < 0)
    {
      handle_close(transport);
      return;
    }
    else if (n > 0)
    { // still have data to perform at next frame
      this->interrupt(wake_for_dispatch, transport.get());
    }
    else // == 0, all buffer consumed, continue recv
    {
      start_receive_op(transport);
    }
  }

  // perform write operations
  if (!transport->send_queue_.empty())
  {
    transport->send_queue_mtx_.lock();
#if _YASIO_VERBOS_LOG
    YASIO_LOG("[index: %d] perform non-blocking write operation...", transport->channel_index());
#endif

    if (do_write(transport) < 0)
    { // TODO: check would block? for client, may
      // be unnecessary.
      transport->send_queue_mtx_.unlock();
      handle_close(transport);
      return;
    }

    if (!transport->send_queue_.empty())
    {
      this->interrupt(wake_for_dispatch, transport.get());
    }

    transport->send_queue_mtx_.unlock();
  }
}
#endif

void io_service::start_receive_op(io_transport *transport)
{
  DWORD bytes_transferred = 0;
  WSABUF wsabuf           = {static_cast<ULONG>(sizeof(transport->buffer_) - transport->offset_),
                   transport->buffer_ + transport->offset_};
  DWORD flags             = 0;

  memset(transport, 0, sizeof(OVERLAPPED));

  int result = WSARecv(transport->socket_->native_handle(), &wsabuf, 1, &bytes_transferred, &flags,
                       transport, nullptr);
  DWORD last_error = transport->update_error();
  if (last_error == ERROR_NETNAME_DELETED)
    last_error = WSAECONNRESET;
  else if (last_error == ERROR_PORT_UNREACHABLE)
    last_error = WSAECONNREFUSED;
  if (result != 0 && last_error != WSA_IO_PENDING)
  {
    this->interrupt(0, transport, bytes_transferred);
  }
}

void io_service::close(size_t channel_index)
{
  // Gets channel context
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  if (ctx->state_ != YCS_CLOSED)
  {
    ctx->state_ = YCS_CLOSED;
    ctx->socket_->close();
    // interrupt(ctx);
  }
}

void io_service::close(transport_ptr transport)
{
  if (transport->is_open())
  {
    transport->socket_->close();
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

void io_service::reopen(transport_ptr transport)
{
  auto ctx = transport->ctx_;
  if (ctx->mask_ & YCM_CLIENT) // Only client channel support reopen by transport
    open_internal(ctx);
}

void io_service::open(size_t channel_index, int channel_type)
{
  if (channel_type == YCM_UDP_SERVER)
  {
    /*
    Because Bind() the client socket to the socket address of the listening socket.  On Linux this
    essentially passes the responsibility for receiving data for the client session from the
    well-known listening socket, to the newly allocated client socket.  It is important to note
    that this behavior is not the same on other platforms, like Windows (unfortunately), detail
    see:
    https://blog.grijjy.com/2018/08/29/creating-high-performance-udp-servers-on-windows-and-linux
  */
    YASIO_LOG("[index: %d], YCM_UDP_SERVER does'n support  Microsoft Winsock provider, you can use "
              "CHANNEL_UDP_CLIENT to communicate with peer!",
              channel_index);
    return;
  }

  // Gets channel
  if (channel_index >= channels_.size())
    return;
  auto ctx = channels_[channel_index];

  ctx->mask_ = channel_type;

  open_internal(ctx);
}

void io_service::handle_close(io_transport* transport)
{
  YASIO_LOG("the connection %s --> %s is lost, error:%d, detail:%s",
            transport->local_endpoint().to_string().c_str(),
            transport->peer_endpoint().to_string().c_str(), transport->error_,
            io_service::strerror(transport->error_));

  do_close(transport);

  auto ctx = transport->ctx_;

  // @Notify connection lost
  this->handle_event(
      event_ptr(new io_event(ctx->index_, YEK_CONNECTION_LOST, transport->error_, transports_[transport->index_])));

  if (ctx->mask_ == YCM_TCP_CLIENT)
  {
    if (ctx->state_ != YCS_OPENING)
      ctx->state_ = YCS_CLOSED;
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

  // @we don't real delete it, just push to free list, make sure all objects' index in transports_
  // never change.
  this->transports_pool_.push_back(transport);
}

void io_service::write(transport_ptr transport, std::vector<char> data)
{
  this->write(transport.get(), std::move(data));
}
void io_service::write(io_transport *transport, std::vector<char> data)
{
  if (transport && transport->socket_->is_open())
  {
    a_pdu_ptr pdu(new a_pdu(std::move(data), std::chrono::microseconds(options_.send_timeout_)));

    transport->send_queue_mtx_.lock();
    transport->send_queue_.push_back(pdu);
    transport->send_queue_mtx_.unlock();

    this->interrupt(wake_for_write, transport);
  }
  else
  {
    YASIO_LOG("[transport: %p] send failed, the connection not ok!", transport);
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

void io_service::do_connect(io_base *channel)
{
  auto ctx = (io_channel *)channel;

  assert(ctx->state_ == YCS_OPENING);
  if (ctx->state_ != YCS_OPENING)
    return;
  assert(ctx->dns_queries_state_ == YDQS_READY);

  ctx->state_ = YCS_OPENING;

  auto &ep = ctx->endpoints_[0];
  if (ctx->mask_ & YCM_TCP)
  {
    BOOL ret = false;
    if (ctx->socket_->open_ex(ep.af()))
    {
      ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

      ctx->socket_->set_nonblocking(true);
      ctx->socket_->bind("0.0.0.0", ctx->local_port_);

      // must register register_descriptor before call connect_ex
      register_descriptor(ctx->socket_->native_handle());

      auto overlap = allocate_transport(ctx, ctx->socket_);
      bool result  = xxsocket::connect_ex(ctx->socket_->native_handle(), &ep.sa_, sizeof(ep.in4_),
                                         nullptr, 0, nullptr, overlap);

      auto last_error = ctx->update_error();
      if (!result && last_error != WSA_IO_PENDING)
      {
        handle_connect_succeed(overlap);
      }
      else
      {
        ctx->deadline_timer_.expires_from_now(std::chrono::microseconds(options_.connect_timeout_));
        ctx->deadline_timer_.async_wait([this, ctx](bool cancelled) {
          if (!cancelled && ctx->state_ != YCS_OPENED)
          {
            handle_connect_failed(ctx, ETIMEDOUT);
          }
        });
      }
    }
  }
  else // CHANNEL_UDP
  {
    int ret = -1;
    if (ctx->socket_->open_ex(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6, SOCK_DGRAM, 0))
    {
      ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

      ctx->socket_->set_nonblocking(true);
      ctx->socket_->bind("0.0.0.0", ctx->local_port_);

      register_descriptor(ctx->socket_->native_handle());

      auto overlap = allocate_transport(ctx, ctx->socket_);
      ret = xxsocket::connect(ctx->socket_->native_handle(), ep);
      if (ret == 0)
      {
        handle_connect_succeed(overlap);
      }
      else
      {
        this->handle_connect_failed(ctx, xxsocket::get_last_errno());
      }
    }
  }
}

void io_service::do_connect_complete(io_base *overlap)
{ // only tcp client will call this function
  auto impl = (io_transport *)overlap;

  if (impl->error_ == 0)
  {
    handle_connect_succeed(impl);
  }
  else
  {
    handle_connect_failed(impl->ctx_, overlap->error_);
  }

  impl->ctx_->deadline_timer_.cancel();
}

void io_service::do_accept(io_channel *ctx)
{ // channel is server
  do_close(ctx);
  assert(ctx->mask_ == YCM_TCP_SERVER);

  if (ctx->socket_->open_ex(ipsv_ & ipsv_ipv4 ? AF_INET : AF_INET6))
  {
    ctx->state_ = YCS_OPENING;

    ip::endpoint ep(ipsv_ & ipsv_ipv4 ? "0.0.0.0" : "::", ctx->port_);
    ctx->socket_->set_optval(SOL_SOCKET, SO_REUSEADDR, 1);
    int error = 0;
    if (ctx->socket_->bind(ep) != 0)
    {
      error = xxsocket::get_last_errno();
      YASIO_LOG("[index: %d] bind failed, ec:%d, detail:%s", ctx->index_, error,
                io_service::strerror(error));
      ctx->socket_->close();
      ctx->state_ = YCS_CLOSED;
      return;
    }

    if (ctx->socket_->listen(YASIO_SOMAXCONN) == 0)
    {
      ctx->state_ = YCS_OPENED;
      ctx->socket_->set_nonblocking(true);

      register_descriptor(ctx->socket_->native_handle());

      start_accept_op(ctx);

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
  else
    ctx->state_ = YCS_CLOSED;
}

void io_service::start_accept_op(io_channel *ctx)
{
  assert(ctx->mask_ == YCM_TCP_SERVER);

  std::shared_ptr<xxsocket> prepared_sock(new xxsocket());
  if (prepared_sock->open_ex(AF_INET))
  {
    auto overlap        = allocate_transport(ctx, prepared_sock);
    DWORD dwBytesReceived = 0;
    bool result           = xxsocket::accept_ex(
        ctx->socket_->native_handle(), prepared_sock->native_handle(), overlap->buffer_,
        0 /* Set to 0, we do not wait read any data during handshake */, sizeof(SOCKADDR_IN) + 16,
        sizeof(SOCKADDR_IN) + 16, &dwBytesReceived, overlap);
    DWORD last_error = overlap->update_error();
    if (!result && last_error != WSA_IO_PENDING)
    {
      handle_connect_succeed(overlap);
    }
  }
}

void io_service::do_accept_complete(io_transport *overlap)
{
  auto impl = (io_transport*)overlap;
  auto ctx  = impl->ctx_;
  if (ctx->state_ == YCS_OPENED)
  {
    if (ctx->error_ == 0)
    {
      handle_connect_succeed(impl);
    }
    else if (ctx->error_ != ERROR_IO_PENDING)
    {
      YASIO_LOG("The channel:%d has error, will be closed!", ctx->index_);
      do_close(ctx);
    }
  }
}

void io_service::handle_connect_succeed(io_transport* overlap)
{
  auto ctx = overlap->ctx_;

  overlap->state_ = YCS_OPENED;

  auto &connection = overlap->socket_;
  if (ctx->mask_ & YCM_CLIENT)
    ctx->state_ = YCS_OPENED;
  else
  { // tcp server, accept a new client session
    connection->set_nonblocking(true);
    register_descriptor(connection->native_handle());
  }
  if (ctx->mask_ & YCM_TCP)
  {
    if (ctx->mask_ & YCM_CLIENT)
      connection->set_optval(SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, ctx->socket_->native_handle());
    else
      connection->set_optval(SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, ctx->socket_->native_handle());

    // apply tcp keepalive options
    if (options_.tcp_keepalive_.onoff)
      connection->set_keepalive(options_.tcp_keepalive_.idle, options_.tcp_keepalive_.interval,
                                options_.tcp_keepalive_.probs);
  }

  YASIO_LOG("[index: %d] the connection [%s] ---> %s is established.", ctx->index_,
            connection->local_endpoint().to_string().c_str(),
            connection->peer_endpoint().to_string().c_str());
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, 0, transports_[overlap->index_])));

  start_receive_op(overlap);

  if (ctx->mask_ == YCM_TCP_SERVER)
    start_accept_op(ctx);
}

void io_service::handle_connect_failed(io_channel *ctx, int error)
{
  do_close(ctx);
  ctx->state_ = YCS_CLOSED;

  // int error = ctx->error_;
  this->handle_event(event_ptr(new io_event(ctx->index_, YEK_CONNECT_RESPONSE, error, nullptr)));

  YASIO_LOG("[index: %d] connect server %s:%u failed, ec:%d, detail:%s", ctx->index_,
            ctx->host_.c_str(), ctx->port_, error, io_service::strerror(error));
}

io_transport *io_service::allocate_transport(io_channel *ctx, std::shared_ptr<xxsocket> socket)
{
  transport_ptr transport;
  if (!transports_pool_.empty())
  { // allocate from free list, and do not need push to transports_ again.
    auto index      = transports_pool_.back()->index_;
    transport       = transports_[index];
    transport->ctx_ = ctx; // !important, update context.
    transports_pool_.pop_back();
    YASIO_LOG("allocate a transport from free list, index=%d, channel_index=%d", index,
              ctx->index_);
  }
  else
  {
    transport.reset(new io_transport(ctx));
    transport->index_ = static_cast<int>(this->transports_.size());
    this->transports_.push_back(transport);
  }

  transport->state_ = YCS_OPENING;

  transport->socket_ = socket;

  return transport.get();
}

bool io_service::do_close(io_base *ctx)
{
  ctx->error_ = 0;
  if (ctx->socket_->is_open())
  {
    ctx->socket_->close();
    return true;
  }
  return false;
}

int io_service::do_write(io_transport *transport)
{
  if (!transport->socket_->is_open())
    return -1;

  int n = 0;
  if (!transport->send_queue_.empty())
  {
    auto ctx               = transport->ctx_;
    auto v                 = transport->send_queue_.front();
    auto outstanding_bytes = static_cast<int>(v->data_.size() - v->offset_);
    n = transport->socket_->send_i(v->data_.data() + v->offset_, outstanding_bytes);
    if (n == outstanding_bytes)
    { // All pdu bytes sent.
      transport->send_queue_.pop_front();
#if _YASIO_VERBOS_LOG
      auto packet_size = static_cast<int>(v->data_.size());
      YASIO_LOG("[index: %d] do_write ok, A packet sent "
                "success, packet size:%d",
                ctx->index_, packet_size, transport->socket_->local_endpoint().to_string().c_str(),
                transport->socket_->peer_endpoint().to_string().c_str());
#endif
      handle_send_finished(v, YERR_OK);
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
        YASIO_LOG("[index: %d] do_write pending, %dbytes still "
                  "outstanding, "
                  "%dbytes was sent!",
                  ctx->index_, outstanding_bytes, n);
      }
      else
      { // send timeout
        transport->send_queue_.pop_front();

        auto packet_size = static_cast<int>(v->data_.size());
        YASIO_LOG("[index: %d] do_write packet timeout, packet "
                  "size:%d",
                  ctx->index_, packet_size);
        handle_send_finished(v, YERR_SEND_TIMEOUT);
      }
    }
    else
    { // n <= 0, TODO: add time
      int error = transport->update_error();
      if (SHOULD_CLOSE_1(n, error))
      {
        YASIO_LOG("[index: %d] do_write error, the connection "
                  "should be "
                  "closed, retval=%d, ec:%d, detail:%s",
                  ctx->index_, n, error, io_service::strerror(error));
        return -1;
      }
    }
  }

  return n;
}

void io_service::handle_send_finished(a_pdu_ptr /*pdu*/, int /*error*/) {}

int io_service::do_unpack(io_transport *transport)
{ // return: whether the transport buffer still have data, -1: error, connection should be close.
  auto ctx = transport->ctx_;

  int ec = transport->error_;

  int n = static_cast<int>(transport->OffsetHigh);

  if (n > 0 || !SHOULD_CLOSE_0(n, ec))
  {
#if _YASIO_VERBOS_LOG
    YASIO_LOG("[index: %d] do_read status ok, ec:%d, detail:%s", transport->channel_index(),
              transport->error_, io_service::strerror(transport->error_));
#endif
    // iocp may no need this check.
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
                       MAX_PDU_BUFFER_SIZE)); // #perfomance, avoid // memory reallocte.
        return do_unpack(transport, transport->expected_packet_size_, n);
      }
      else if (length == 0)
      {
        // header insufficient, wait readfd ready at
        // next event step.
        transport->offset_ += n;
        return 0;
      }
      else
      {
        // set_errorno(ctx, YERR_DPL_ILLEGAL_PDU);
        YASIO_LOG("[index: %d] do_read error, decode length of "
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
    transport->offset_ = n;

    return -1;
  }
}

int io_service::do_unpack(io_transport *transport, int bytes_expected, int bytes_transferred)
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
    YASIO_LOG("[index: %d] received a properly packet from peer, "
              "packet size:%d",
              transport->channel_index(), transport->expected_packet_size_);
#endif
    this->handle_event(event_ptr(
        new io_event(transport->channel_index(), YEK_PACKET, transport->take_packet(), transports_[transport->index_])));
  }
  else
  { // all buffer consumed, set offset to ZERO, pdu
    // incomplete, continue recv remain data.
    offset = 0;
  }

  return offset;
}

std::shared_ptr<deadline_timer> io_service::schedule(highp_time_t duration, timer_cb_t cb,
                                                     bool repeated)
{
  std::shared_ptr<deadline_timer> timer(new deadline_timer(*this));
  timer->expires_from_now(std::chrono::microseconds(duration), repeated);
  timer->async_wait([this, timer /*!important, hold on by lambda expression */,
                     cb](bool cancelled) { cb(cancelled); });
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
    this->interrupt(wake_for_dispatch);
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

  ctx->state_ = YCS_OPENING;
  if (ctx->socket_->is_open())
  {
    ctx->socket_->shutdown();
  }

  this->interrupt(wake_for_open, ctx);
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

// bool io_service::close_internal(io_base *ctx)
//{
//  if (ctx->socket_->is_open())
//  {
//    ctx->socket_->close();
//    return true;
//  }
//  return false;
//}

u_short io_service::update_dns_queries_state(io_channel *ctx, bool update_name)
{
  if (!update_name)
  {
    if ((ctx->dns_queries_state_ & YDQSF_QUERIES_NEEDED) &&
        !YDQS_CHECK_STATE(ctx->dns_queries_state_, YDQS_INPRROGRESS))
    {
      auto diff = (_highp_clock() - ctx->dns_queries_timestamp_);
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
      ctx->dns_queries_timestamp_ = _highp_clock();
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
    this->interrupt(wake_for_open, ctx);
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

void io_service::interrupt(DWORD completion_key, LPOVERLAPPED lpOverlapped, DWORD bytes_transferred)
{
  ::PostQueuedCompletionStatus(iocp_.handle, bytes_transferred, completion_key, lpOverlapped);
}

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
      {
        this->channels_[index]->local_port_ = (u_short)va_arg(ap, int);
      }
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
} // namespace purelib
