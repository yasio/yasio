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

namespace yasio
{
using namespace purelib;
}

namespace yasio
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
  YEK_CONNECT_RESPONSE = 0,
  YEK_CONNECTION_LOST,
  YEK_PACKET,
};

// class fwds
class a_pdu; // application layer protocol data unit.
class deadline_timer;
class io_event;
class io_channel;
class io_transport;
class io_service;

// typedefs
typedef long long highp_time_t;
typedef std::chrono::high_resolution_clock highp_clock_t;
typedef std::chrono::system_clock system_clock_t;

typedef std::shared_ptr<a_pdu> a_pdu_ptr;
typedef io_transport *transport_ptr;
typedef std::unique_ptr<io_event> event_ptr;

typedef std::function<void(bool cancelled)> timer_cb_t;
typedef std::pair<deadline_timer *, timer_cb_t> timer_impl_t;
typedef std::function<void(event_ptr)> io_event_cb_t;
typedef std::function<int(void *ptr, int len)> decode_len_fn_t;
typedef std::function<int(std::vector<ip::endpoint> &, const char *, unsigned short)> resolv_fn_t;

// The high precision micro seconds timestamp
template <typename _T = highp_clock_t> inline long long highp_clock()
{
  auto duration = _T::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

struct io_hostent
{
  io_hostent() {}
  io_hostent(std::string ip, u_short port) : host_(std::move(ip)), port_(port) {}
  io_hostent(io_hostent &&rhs) : host_(std::move(rhs.host_)), port_(rhs.port_) {}
  io_hostent(const io_hostent &rhs) : host_(rhs.host_), port_(rhs.port_) {}
  void set_ip(const std::string &value) { host_ = value; }
  const std::string &get_ip() const { return host_; }
  void set_port(u_short port) { port_ = port; }
  u_short get_port() const { return port_; }
  std::string host_;
  u_short port_;
};

class deadline_timer
{
public:
  ~deadline_timer() {}
  deadline_timer(io_service &service) : service_(service), repeated_(false), cancelled_(false) {}

  void expires_from_now(const std::chrono::microseconds &duration, bool repeated = false)
  {
    this->duration_  = duration;
    this->repeated_  = repeated;
    this->cancelled_ = false;
    expire_time_     = highp_clock_t::now() + this->duration_;
  }

  void expires_from_now() { expire_time_ = highp_clock_t::now() + this->duration_; }

  // Wait timer timeout or cancelled.
  void async_wait(timer_cb_t);

  // Cancel the timer
  void cancel();

  // Check if timer is expired?
  bool expired() const { return wait_duration().count() <= 0; }

  // Remove from io_service's timer queue
  void unschedule();

  // Gets wait duration of timer.
  std::chrono::microseconds wait_duration() const
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(expire_time_ -
                                                                 highp_clock_t::now());
  }

  io_service &service_;

  bool repeated_;
  bool cancelled_;
  std::chrono::microseconds duration_;
  std::chrono::time_point<highp_clock_t> expire_time_;
};

struct io_base
{
  int update_error() { return (error_ = xxsocket::get_last_errno()); }
  void update_error(int error) { error_ = error; }

  std::shared_ptr<xxsocket> socket_;
  int error_ = 0; // socket error(>= -1), application error(< -1)

  u_short opmask_ = 0;
  short state_    = 0;
};

class io_channel : public io_base
{
public:
  io_channel(io_service &service);

  u_short mask_ = 0;
  // specific local port, if not zero, tcp/udp client will use it as fixed port
  u_short local_port_ = 0;

  std::atomic<u_short> dns_queries_state_;
  u_short port_;
  std::string host_;

  std::vector<ip::endpoint> endpoints_;

  highp_time_t dns_queries_timestamp_ = 0;

  int index_    = -1;
  int protocol_ = 0;

  // The deadline timer for resolve & connect
  deadline_timer deadline_timer_;
};

class io_transport : public io_base
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
  io_transport(io_channel *ctx);

  unsigned int id_;

  char buffer_[65536]; // recv buffer, 64K
  int offset_ = 0;     // recv buffer offset

  std::vector<char> expected_packet_;
  int expected_packet_size_ = -1;

  std::recursive_mutex send_queue_mtx_;
  std::deque<a_pdu_ptr> send_queue_;

  io_channel *ctx_;

public:
  void *ud_ = nullptr;
};

class io_event final
{
public:
  io_event(int channel_index, int kind, int error, transport_ptr transport)
      : timestamp_(highp_clock()), cindex_(channel_index), kind_(kind), status_(error),
        transport_(std::move(transport))
  {}
  io_event(int channel_index, int type, std::vector<char> packet, transport_ptr transport)
      : timestamp_(highp_clock()), cindex_(channel_index), kind_(type), status_(0),
        transport_(std::move(transport)), packet_(std::move(packet))
  {}
  io_event(io_event &&rhs)
      : timestamp_(rhs.timestamp_), cindex_(rhs.cindex_), kind_(rhs.kind_), status_(rhs.status_),
        transport_(std::move(rhs.transport_)), packet_(std::move(rhs.packet_))
  {}

  ~io_event() {}

  int cindex() const { return cindex_; }

  int kind() const { return kind_; }
  int status() const { return status_; }

  transport_ptr transport() { return transport_; }

  std::vector<char> &packet() { return packet_; }
  long long timestamp() const { return timestamp_; }

#if _USING_OBJECT_POOL
  DEFINE_CONCURRENT_OBJECT_POOL_ALLOCATION(io_event, 512)
#endif

private:
  long long timestamp_;
  int cindex_;
  int kind_;
  int status_;
  transport_ptr transport_;
  std::vector<char> packet_;
};

class io_service
{
  friend class deadline_timer;

public:
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
  void start_service(const io_hostent *channel_eps, int channel_count, io_event_cb_t cb);
  void start_service(const io_hostent *channel_eps, io_event_cb_t cb)
  {
    this->start_service(channel_eps, 1, std::move(cb));
  }
  void start_service(std::vector<io_hostent> channel_eps, io_event_cb_t cb)
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

  /* option: YOPT_CONNECT_TIMEOUT   timeout:int
             YOPT_SEND_TIMEOUT      timeout:int
             YOPT_RECONNECT_TIMEOUT timeout:int
             YOPT_DNS_CACHE_TIMEOUT timeout:int
             YOPT_DEFER_EVENT       defer:int
             YOPT_TCP_KEEPALIVE     idle:int, interal:int, probes:int
             YOPT_RESOLV_FUNCTION   func:resolv_fn_t*
             YOPT_LFBFD_PARAMS max_frame_length:int, length_field_offst:int,
     length_field_length:int, length_adjustment:int YOPT_IO_EVENT_CALLBACK
     func:io_event_callback_t*
             YOPT_CHANNEL_LOCAL_PORT  index:int, port:int
             YOPT_CHANNEL_REMOTE_HOST index:int, ip:const char*
             YOPT_CHANNEL_REMOTE_PORT index:int, port:int
             YOPT_CHANNEL_REMOTE_ENDPOINT index:int, ip:const char*, port:int
             YOPT_NO_NEW_THREAD value:int
  */
  void set_option(int option, ...);

  // open a channel, default: YCM_TCP_CLIENT
  void open(size_t channel_index, int channel_mask = YCM_TCP_CLIENT);

  void reopen(transport_ptr);

  // close transport
  void close(transport_ptr);

  // close channel
  void close(size_t channel_index = 0);

  // check whether the channel is open
  bool is_open(size_t cahnnel_index = 0) const;

  // check whether the transport is open
  bool is_open(transport_ptr) const;

  int write(transport_ptr transport, std::vector<char> data);

  // The deadlien_timer support, !important, the callback is called on the thread of io_service
  std::shared_ptr<deadline_timer> schedule(highp_time_t duration, timer_cb_t,
                                           bool repeated = false);

  void cleanup();

  int __builtin_resolv(std::vector<ip::endpoint> &endpoints, const char *hostname,
                       unsigned short port = 0);
  // -1 indicate failed, connection will be closed
  int __builtin_decode_len(void *ptr, int len);

private:
  void schedule_timer(deadline_timer *, timer_cb_t &);
  void remove_timer(deadline_timer *);

  inline std::vector<timer_impl_t>::iterator find_timer(deadline_timer *key)
  {
    return std::find_if(timer_queue_.begin(), timer_queue_.end(),
                        [=](const timer_impl_t &timer) { return timer.first == key; });
  }
  inline void sort_timers_unlocked()
  {
    std::sort(this->timer_queue_.begin(), this->timer_queue_.end(),
              [](const timer_impl_t &lhs, const timer_impl_t &rhs) {
                return lhs.first->wait_duration() > rhs.first->wait_duration();
              });
  }

  // Start a async resolve, It's only for internal use
  void start_resolve(io_channel *);

  void init(const io_hostent *channel_eps, int channel_count, io_event_cb_t &cb);

  void open_internal(io_channel *, bool ignore_state = false);

  void perform_transports(fd_set *fds_array);
  void perform_channels(fd_set *fds_array);
  void perform_timers();

  void interrupt();

  long long get_wait_duration(long long usec);

  int do_evpoll(fd_set *fds_array);

  void do_nonblocking_connect(io_channel *);
  void do_nonblocking_connect_completion(io_channel *, fd_set *fds_array);

  inline void handle_connect_succeed(io_channel *ctx, std::shared_ptr<xxsocket> socket)
  {
    handle_connect_succeed(allocate_transport(ctx, std::move(socket)));
  }
  void handle_connect_succeed(transport_ptr);
  void handle_connect_failed(io_channel *, int ec);

  transport_ptr allocate_transport(io_channel *, std::shared_ptr<xxsocket>);

  void register_descriptor(const socket_native_type fd, int flags);
  void unregister_descriptor(const socket_native_type fd, int flags);

  // The major non-blocking event-loop
  void run(void);

  bool do_write(transport_ptr);
  bool do_read(transport_ptr, fd_set *fds_array);
  void do_unpack(transport_ptr, int bytes_expected, int bytes_transferred);

  // The op mask will be cleared, the state will be set CLOSED
  bool cleanup_io(io_base *ctx);

  void handle_close(transport_ptr);
  void handle_event(event_ptr event);

  // new/delete client socket connection channel
  // please call this at initialization, don't new channel at runtime
  // dynmaically: because this API is not thread safe.
  io_channel *new_channel(const io_hostent &ep);
  // Clear all channels after service exit.
  void clear_channels();   // destroy all channels
  void clear_transports(); // destroy all transports
  bool close_internal(io_channel *);

  // Update resolve state for new endpoint set
  u_short update_dns_queries_state(io_channel *ctx, bool update_name);

  void handle_send_finished(a_pdu_ptr, int);

  // supporting server
  void do_nonblocking_accept(io_channel *);
  void do_nonblocking_accept_completion(io_channel *, fd_set *fds_array);

  static const char *strerror(int error);

private:
  state state_; // The service state
  std::thread worker_;
  std::thread::id worker_id_;

  std::recursive_mutex event_queue_mtx_;
  std::deque<event_ptr> event_queue_;

  std::vector<io_channel *> channels_;

  std::recursive_mutex channel_ops_mtx_;
  std::vector<io_channel *> channel_ops_;

  std::vector<transport_ptr> transports_;
  std::vector<transport_ptr> transports_dypool_;

  // select interrupter
  select_interrupter interrupter_;

  // timer support timer_pair
  std::vector<timer_impl_t> timer_queue_;
  std::recursive_mutex timer_queue_mtx_;

  // socket event set
  int maxfdp_;
  enum
  {
    read_op,
    write_op,
    except_op,
    max_ops,
  };
  fd_set fds_array_[max_ops];

  // optimize record incomplete works
  int outstanding_work_;

  // the event callback
  io_event_cb_t on_event_;

  // options
  struct __unnamed_options
  {
    highp_time_t connect_timeout_   = 10LL * MICROSECONDS_PER_SECOND;
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
      int length_field_offset = 0; // -1: directly, >= 0: store as 1~4bytes integer, default value=0
      int length_field_length = 4; // 1,2,3,4
      int length_adjustment   = 0;
      int max_frame_length    = SZ(10, M);
    } lfb_;
    int outf_           = -1;
    int outf_max_size_  = SZ(5, M);
    bool no_new_thread_ = false;
  } options_;

  // The decode frame length function
  decode_len_fn_t decode_len_;

  // The resolve function
  resolv_fn_t resolv_;

  // The ip stack version supported by local host
  u_short ipsv_ = 0;
}; // io_service
} // namespace inet
} /* namespace yasio */

#define myasio yasio::gc::singleton<yasio::inet::io_service>::instance()
