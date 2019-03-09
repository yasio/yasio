//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

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

#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#include "yasio/detail/endian_portable.h"
#include "yasio/detail/object_pool.h"
#include "yasio/detail/select_interrupter.hpp"
#include "yasio/detail/singleton.h"
#include "yasio/detail/string_view.hpp"
#include "yasio/xxsocket.h"

#define _USING_OBJECT_POOL 1

#if !defined(MICROSECONDS_PER_SECOND)
#  define MICROSECONDS_PER_SECOND 1000000LL
#endif

#if !defined(_ARRAYSIZE)
#  define _ARRAYSIZE(A) (sizeof(A) / sizeof((A)[0]))
#endif

namespace yasio = ::purelib;

namespace purelib
{
namespace inet
{
// options
enum
{
  YOPT_CONNECT_TIMEOUT = 1,
  YOPT_SEND_TIMEOUT,
  YOPT_RECONNECT_TIMEOUT,
  YOPT_DNS_CACHE_TIMEOUT,
  YOPT_DEFER_EVENT,
  YOPT_TCP_KEEPALIVE, // the default usually is idle=7200, interval=75, probes=10
  YOPT_RESOLV_FUNCTION,
  YOPT_LOG_FILE,
  YOPT_LFBFD_PARAMS, // length field based frame decode params
  YOPT_IO_EVENT_CALLBACK,
  YOPT_DECODE_FRAME_LENGTH_FUNCTION, // Native C++ ONLY
  YOPT_CHANNEL_LOCAL_PORT,           // Sets channel local port
  YOPT_CHANNEL_REMOTE_HOST,
  YOPT_CHANNEL_REMOTE_PORT,
  YOPT_CHANNEL_REMOTE_ENDPOINT, // Sets remote endpoint: host, port
  YOPT_NO_NEW_THREAD,           // Don't start a new thread to run event loop
};

// channel mask
enum
{
  YCM_CLIENT     = 1,
  YCM_SERVER     = 1 << 1,
  YCM_TCP        = 1 << 2,
  YCM_UDP        = 1 << 3,
  YCM_TCP_CLIENT = YCM_TCP | YCM_CLIENT,
  YCM_TCP_SERVER = YCM_TCP | YCM_SERVER,
  YCM_UDP_CLIENT = YCM_UDP | YCM_CLIENT,
  YCM_UDP_SERVER = YCM_UDP | YCM_SERVER,
};

// event kinds
enum
{
  YEK_CONN_RESP = 0,
  YEK_CONN_LOST,
  YEK_PACKET,
};

typedef std::chrono::high_resolution_clock highp_clock_t;
typedef long long highp_time_t;

typedef std::function<void()> vdcallback_t;

typedef std::function<bool(std::vector<ip::endpoint> &endpoints, const char *hostname,
                           unsigned short port)>
    resolv_fn_t;

class a_pdu; // application layer protocol data unit.

typedef std::shared_ptr<a_pdu> a_pdu_ptr;

class io_service;

class deadline_timer
{
public:
  ~deadline_timer() {}
  deadline_timer(io_service &service) : repeated_(false), service_(service) {}

  void expires_from_now(const std::chrono::microseconds &duration, bool repeated = false)
  {
    this->duration_ = duration;
    this->repeated_ = repeated;
    expire_time_    = highp_clock_t::now() + this->duration_;
  }

  void expires_from_now() { expire_time_ = highp_clock_t::now() + this->duration_; }

  // Wait timer timeout or cancelled.
  void async_wait(const std::function<void(bool cancelled)> &callback);

  // Cancel the timer
  void cancel();

  // Check if timer is expired?
  bool expired() const { return wait_duration().count() <= 0; }

  // Let timer expire immidlately
  void expire() { expire_time_ = highp_clock_t::now() - duration_; }

  // Gets wait duration of timer.
  std::chrono::microseconds wait_duration() const
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(expire_time_ -
                                                                 highp_clock_t::now());
  }

  bool repeated_;
  io_service &service_;
  std::chrono::microseconds duration_;
  std::chrono::time_point<highp_clock_t> expire_time_;
  std::function<void(bool cancelled)> callback_;
};

struct io_hostent
{
  io_hostent() {}
  io_hostent(stdport::string_view ip, u_short port) : host_(ip.data(), ip.length()), port_(port) {}
  io_hostent(io_hostent &&rhs) : host_(std::move(rhs.host_)), port_(rhs.port_) {}
  io_hostent(const io_hostent &rhs) : host_(rhs.host_), port_(rhs.port_) {}
  void set_ip(const std::string &value) { host_ = value; }
  const std::string &get_ip() const { return host_; }
  void set_port(u_short port) { port_ = port; }
  u_short get_port() const { return port_; }
  std::string host_;
  u_short port_;
};

struct io_transport;

struct io_base
{
  int update_error() { return (error_ = xxsocket::get_last_errno()); }
  void update_error(int error) { error_ = error; }

  std::shared_ptr<xxsocket> socket_;
  int error_             = 0; // socket error(>= -1), application error(< -1)
  u_short shutdown_mask_ = 0;
};

struct io_channel : public io_base
{
  io_channel(io_service &service);

  u_short type_ = 0;

  u_short state_; // 0: CLOSED, 1: OPENING, 2: OPENED
  // specific local port, if not zero, tcp/udp client will use it as fixed port
  u_short local_port_ = 0;

  bool dns_queries_needed_;
  u_short port_;
  std::string host_;

  std::vector<ip::endpoint> endpoints_;
  std::atomic<resolve_state> resolve_state_;

  highp_time_t dns_queries_timestamp_ = 0;

  int index_ = -1;
  int protocol_ = 0;

  // The deadline timer for resolve & connect
  deadline_timer deadline_timer_;

  void reset();
};

struct io_transport : public io_base
{
  friend class io_service;

public:
  bool is_open() const { return socket_ != nullptr && socket_->is_open(); }
  ip::endpoint local_endpoint() const { return socket_->local_endpoint(); }
  ip::endpoint peer_endpoint() const { return socket_->peer_endpoint(); }
  int channel_index() const { return ctx_->index_; }
  int status() const { return error_; }
  inline std::vector<char> take_packet()
  {
    expected_packet_size_ = -1;
    return std::move(expected_packet_);
  }

private:
  io_transport(io_channel *ctx) : ctx_(ctx) {}

  char buffer_[65535]; // recv buffer, 64K
  int offset_ = 0;     // recv buffer offset

  std::vector<char> expected_packet_;
  int expected_packet_size_ = -1;

  std::recursive_mutex send_queue_mtx_;
  std::deque<a_pdu_ptr> send_queue_;

  io_channel *ctx_;

public:
  void *ud_ = nullptr;
};

typedef std::shared_ptr<io_transport> transport_ptr;

class io_event final
{
public:
  io_event(int channel_index, int kind, int error, transport_ptr transport)
      : channel_index_(channel_index), kind_(kind), status_(error), transport_(std::move(transport))
  {}
  io_event(int channel_index, int type, std::vector<char> packet, transport_ptr transport)
      : channel_index_(channel_index), kind_(type), status_(0), transport_(std::move(transport)),
        packet_(std::move(packet))
  {}
  io_event(io_event &&rhs)
      : channel_index_(rhs.channel_index_), kind_(rhs.kind_), status_(rhs.status_),
        transport_(std::move(rhs.transport_)), packet_(std::move(rhs.packet_))
  {}

  ~io_event() {}

  int channel_index() const { return channel_index_; }

  int kind() const { return kind_; }
  int status() const { return status_; }

  transport_ptr transport() { return transport_; }

  const std::vector<char> &packet() const { return packet_; }
  std::vector<char> take_packet() { return std::move(packet_); }

#if _USING_OBJECT_POOL
  DEFINE_CONCURRENT_OBJECT_POOL_ALLOCATION(io_event, 512)
#endif

private:
  int channel_index_;
  int kind_;
  int status_;
  transport_ptr transport_;
  std::vector<char> packet_;
};
typedef std::unique_ptr<io_event> event_ptr;

class deadline_timer;

typedef std::function<void(error_number)> send_pdu_callback_t;
typedef std::function<void(event_ptr)> io_event_callback_t;
typedef std::function<int(io_service *service, void *ptr, int len)> decode_frame_length_fn_t;

class io_service
{
public:
  // End user pdu decode length func
  // connection callbacks
  typedef std::function<void(transport_ptr)> connection_lost_callback_t;
  typedef std::function<void(size_t, transport_ptr, int ec)> connect_response_callback_t;
  enum class state
  {
    IDLE,
    INITIALIZED,
    RUNNING,
    STOPPING,
    STOPPED,
  };

public:
  io_service();
  ~io_service();

  // start async socket io service
  void start_service(const io_hostent *channel_eps, int channel_count, io_event_callback_t cb);
  void start_service(const io_hostent *channel_eps, io_event_callback_t cb)
  {
    this->start_service(channel_eps, 1, std::move(cb));
  }
  void start_service(std::vector<io_hostent> channel_eps, io_event_callback_t cb)
  {
    if (!channel_eps.empty())
    {
      this->start_service(&channel_eps.front(), static_cast<int>(channel_eps.size()),
                          std::move(cb));
    }
  }

  void stop_service();
  void wait_service();

  bool is_running() const { return this->state_ == io_service::state::RUNNING; }

  // should call at the thread who care about async io
  // events(CONNECT_RESPONSE,CONNECTION_LOST,PACKET), such cocos2d-x opengl or
  // any other game engines' render thread.
  void dispatch_events(int count = 512);

  /* option: YASIO_OPT_CONNECT_TIMEOUT   timeout:int
             YASIO_OPT_SEND_TIMEOUT      timeout:int
             YASIO_OPT_RECONNECT_TIMEOUT timeout:int
             YASIO_OPT_DNS_CACHE_TIMEOUT timeout:int
             YASIO_OPT_DEFER_EVENT       defer:int
             YASIO_OPT_TCP_KEEPALIVE     idle:int, interal:int, probes:int
             YASIO_OPT_RESOLV_FUNCTION   func:resolv_fn_t*
             YASIO_OPT_LFBFD_PARAMS max_frame_length:int, length_field_offst:int,
     length_field_length:int, length_adjustment:int YASIO_OPT_IO_EVENT_CALLBACK
     func:io_event_callback_t*
             YASIO_OPT_CHANNEL_LOCAL_PORT  index:int, port:int
             YASIO_OPT_CHANNEL_REMOTE_HOST index:int, ip:const char*
             YASIO_OPT_CHANNEL_REMOTE_PORT index:int, port:int
             YASIO_OPT_CHANNEL_REMOTE_ENDPOINT index:int, ip:const char*, port:int
             YASIO_OPT_NO_NEW_THREAD value:int
  */
  void set_option(int option, ...);

  // open a channel, default: YCM_TCP_CLIENT
  void open(size_t channel_index, int channel_mask = YCM_TCP_CLIENT);

  void reopen(transport_ptr);

  // close transport
  void close(transport_ptr);

  // close server
  void close(size_t channel_index = 0);

  // check whether the channel is open
  bool is_open(size_t cahnnel_index = 0) const;

  void write(transport_ptr transport, std::vector<char> data);
  void write(io_transport *transport, std::vector<char> data);

  // timer support
  void schedule_timer(deadline_timer *);
  void cancel_timer(deadline_timer *);

  void interrupt();

  // Start a async resolve, It's only for internal use
  bool start_resolve(io_channel *);

  bool resolve(std::vector<ip::endpoint> &endpoints, const char *hostname, unsigned short port = 0);

  void cleanup();

private:
  void init(const io_hostent *channel_eps, int channel_count, io_event_callback_t cb);

  void open_internal(io_channel *);

  void perform_transports(fd_set *fds_array);
  void perform_channels(fd_set *fds_array);
  void perform_timers();

  long long get_wait_duration(long long usec);

  int do_evpoll(fd_set *fds_array, timeval &timeout);

  bool do_nonblocking_connect(io_channel *);
  bool do_nonblocking_connect_completion(io_channel *, fd_set *fds_array);

  inline void handle_connect_succeed(io_channel *ctx, std::shared_ptr<xxsocket> socket)
  {
    handle_connect_succeed(allocate_transport(ctx, std::move(socket)));
  }
  void handle_connect_succeed(transport_ptr);
  void handle_connect_failed(io_channel *);

  transport_ptr allocate_transport(io_channel *, std::shared_ptr<xxsocket>);

  void register_descriptor(const socket_native_type fd, int flags);
  void unregister_descriptor(const socket_native_type fd, int flags);

  // The major non-blocking event-loop
  void run(void);

  bool do_write(transport_ptr);
  bool do_read(transport_ptr);
  void do_unpack(transport_ptr, int bytes_expected, int bytes_transferred);

  bool do_close(io_base *ctx);
  
  void handle_close(transport_ptr);

  void handle_event(event_ptr event);

  // new/delete client socket connection channel
  // please call this at initialization, don't new channel at runtime
  // dynmaically: because this API is not thread safe.
  io_channel *new_channel(const io_hostent &ep);

  // Clear all channels after service exit.
  void clear_channels(); // destroy all channels

  void close_internal(io_channel*);

  // Update resolve state for new endpoint set
  void update_resolve_state(io_channel *ctx);

  void handle_send_finished(a_pdu_ptr, error_number);

  // supporting server
  void do_nonblocking_accept(io_channel *);
  void do_nonblocking_accept_completion(io_channel *, fd_set *fds_array);

  // -1 indicate failed, connection will be closed
  int builtin_decode_frame_length(void *ptr, int len);

  static const char *strerror(int error);

private:
  state state_;
  std::thread worker_thread_;
  std::thread::id worker_id_;

  std::recursive_mutex event_queue_mtx_;
  std::deque<event_ptr> event_queue_;

  std::vector<io_channel *> channels_;

  std::recursive_mutex active_channels_mtx_;
  std::vector<io_channel *> active_channels_;

  std::vector<transport_ptr> transports_;

  // select interrupter
  select_interrupter interrupter_;

  // timer support
  std::vector<deadline_timer *> timer_queue_;
  std::recursive_mutex timer_queue_mtx_;

  // socket event set
  int maxfdp_;
  enum
  {
    read_op,  // for async read and write(trigger by interrupt)
    write_op, // for async connect
    except_op,
  };
  fd_set fds_array_[3];

  // optimize record incomplete works
  int outstanding_work_;

  // the event callback
  io_event_callback_t on_event_;

  /*
  options_.tcp.keepalive.
  options_.length_field_offset: -1: direct, >= 0: store as 4bytes integer, default value=0
  options_.max_pdu_size: default value=10M
  options_.log_file:
  */
  struct __unnamed_options
  {
    highp_time_t connect_timeout_   = 5LL * MICROSECONDS_PER_SECOND;
    highp_time_t send_timeout_      = (std::numeric_limits<int>::max)();
    highp_time_t reconnect_timeout_ = -1;
    // Default dns cache time: 10 minutes
    highp_time_t dns_cache_timeout_ = 600LL * MICROSECONDS_PER_SECOND;

    bool deferred_event_ = true;
    // tcp keepalive settings
    struct __unnamed01
    {
      int onoff    = 0;
      int idle     = 7200;
      int interval = 75;
      int probs    = 10;
    } tcp_keepalive_;

    struct __unnamed02
    {
      int length_field_offset = 0;
      int length_field_length = 4; // 1,2,3,4
      int length_adjustment   = 0;
      int max_frame_length    = SZ(10, M);
    } lfb_;
    FILE *outf_         = nullptr;
    bool no_new_thread_ = false;
  } options_;

  // The decode frame length function
  decode_frame_length_fn_t xdec_len_;

  // The resolve function
  resolv_fn_t xresolv_;

  // The ip stack version supported by local host
  int ipsv_;
}; // io_service
}; // namespace inet
}; /* namespace purelib */

#define myasio purelib::gc::singleton<purelib::inet::io_service>::instance()
