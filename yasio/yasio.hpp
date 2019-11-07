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
#ifndef YASIO__CORE_HPP
#define YASIO__CORE_HPP
#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <ctime>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <functional>
#if defined(_WIN32)
#  include <map>
#endif
#include "yasio/detail/config.hpp"
#include "yasio/detail/endian_portable.h"
#include "yasio/detail/object_pool.h"
#include "yasio/detail/singleton.h"
#include "yasio/detail/select_interrupter.hpp"
#include "yasio/detail/concurrent_queue.hpp"
#include "yasio/cxx17/string_view.hpp"
#include "yasio/xxsocket.hpp"

#if !defined(MICROSECONDS_PER_SECOND)
#  define MICROSECONDS_PER_SECOND 1000000LL
#endif

#if defined(YASIO_HAVE_KCP)
typedef struct IKCPCB ikcpcb;
#endif

namespace yasio
{
namespace inet
{
// options
enum
{
  YOPT_CONNECT_TIMEOUT = 1,
  YOPT_RECONNECT_TIMEOUT,
  YOPT_DNS_CACHE_TIMEOUT,
  YOPT_DEFER_EVENT,
  YOPT_DEFER_HANDLER,
  YOPT_TCP_KEEPALIVE, // the default usually is idle=7200, interval=75, probes=10
  YOPT_RESOLV_FN,
  YOPT_PRINT_FN,
  YOPT_IO_EVENT_CB,
  YOPT_NO_NEW_THREAD,    // Don't start a new thread to run event loop
  YOPT_CHANNEL_LFBFD_FN, // length field based frame decode function, Native C++ ONLY
  YOPT_CHANNEL_LFBFD_PARAMS,
  YOPT_CHANNEL_LOCAL_PORT, // Sets channel local port
  YOPT_CHANNEL_REMOTE_HOST,
  YOPT_CHANNEL_REMOTE_PORT,
  YOPT_CHANNEL_REMOTE_ENDPOINT, // Sets remote endpoint: host, port
  YOPT_TRANSPORT_SOCKOPT, // Sets transport SOL_SOCKET, params: transport_handle, level, optname,
                          // optval, optlen
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
#if defined(YASIO_HAVE_KCP)
  YCM_KCP        = 1 << 4,
  YCM_KCP_CLIENT = YCM_KCP | YCM_UDP_CLIENT,
  YCM_KCP_SERVER = YCM_KCP | YCM_UDP_SERVER,
#endif
  YCM_MULTICAST        = 1 << 5,
  YCM_MULTICAST_CLIENT = YCM_MULTICAST | YCM_UDP_CLIENT,
  YCM_MULTICAST_SERVER = YCM_MULTICAST | YCM_UDP_SERVER,
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
class io_transport_posix;
class io_service;

// recommand user always use transport_handle_t, in the future, it's maybe void* or intptr_t
typedef io_transport* transport_handle_t;

// typedefs
typedef long long highp_time_t;
typedef std::chrono::high_resolution_clock highp_clock_t;
typedef std::chrono::system_clock system_clock_t;

typedef std::shared_ptr<a_pdu> a_pdu_ptr;
typedef std::unique_ptr<io_event> event_ptr;
typedef std::shared_ptr<deadline_timer> deadline_timer_ptr;

typedef std::function<void(bool)> timer_cb_t;
typedef std::pair<deadline_timer*, timer_cb_t> timer_impl_t;
typedef std::function<void(event_ptr&&)> io_event_cb_t;
typedef std::function<int(void* ptr, int len)> decode_len_fn_t;
typedef std::function<int(std::vector<ip::endpoint>&, const char*, unsigned short)> resolv_fn_t;
typedef std::function<void(const char*)> print_fn_t;

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
  io_hostent(io_hostent&& rhs) : host_(std::move(rhs.host_)), port_(rhs.port_) {}
  io_hostent(const io_hostent& rhs) : host_(rhs.host_), port_(rhs.port_) {}
  void set_ip(const std::string& value) { host_ = value; }
  const std::string& get_ip() const { return host_; }
  void set_port(u_short port) { port_ = port; }
  u_short get_port() const { return port_; }
  std::string host_;
  u_short port_;
};

class deadline_timer
{
public:
  ~deadline_timer() {}
  deadline_timer(io_service& service) : service_(service), repeated_(false), cancelled_(false) {}

  void expires_from_now(const std::chrono::microseconds& duration, bool repeated = false)
  {
    this->duration_  = duration;
    this->repeated_  = repeated;
    this->cancelled_ = false;
    expire_time_     = highp_clock_t::now() + this->duration_;
  }

  void expires_from_now() { expire_time_ = highp_clock_t::now() + this->duration_; }

  // Wait timer timeout or cancelled.
  YASIO__DECL void async_wait(timer_cb_t);

  // Cancel the timer
  YASIO__DECL void cancel();

  // Check if timer is expired?
  bool expired() const { return wait_duration().count() <= 0; }

  // Remove from io_service's timer queue
  YASIO__DECL void unschedule();

  // Gets wait duration of timer.
  std::chrono::microseconds wait_duration() const
  {
    return std::chrono::duration_cast<std::chrono::microseconds>(expire_time_ -
                                                                 highp_clock_t::now());
  }

  io_service& service_;

  bool repeated_;
  bool cancelled_;
  std::chrono::microseconds duration_;
  std::chrono::time_point<highp_clock_t> expire_time_;
};

struct io_base
{
  void set_last_errno(int error) { error_ = error; }

  std::shared_ptr<xxsocket> socket_;
  int error_ = 0; // socket error(>= -1), application error(< -1)

  u_short opmask_ = 0;
  short state_    = 0;
};

class io_channel : public io_base
{
  friend class io_service;
  friend class io_transport_posix;

public:
  io_service& get_service() { return deadline_timer_.service_; }
  inline int index() { return index_; }

  void setup_multicast(std::shared_ptr<xxsocket>&);
  void cleanup_multicast(std::shared_ptr<xxsocket>&);

private:
  YASIO__DECL io_channel(io_service& service);

  inline void setup_hostent(std::string host, u_short port)
  {
    setup_host(host);
    setup_port(port);
  }

  YASIO__DECL void setup_host(std::string host);
  YASIO__DECL void setup_port(u_short port);

  // -1 indicate failed, connection will be closed
  YASIO__DECL int __builtin_decode_len(void* ptr, int len);

  u_short mask_ = 0;
  // specific local port, if not zero, tcp/udp client will use it as fixed port
  u_short local_port_ = 0;

  std::atomic<u_short> dns_queries_state_;
  u_short port_ = 0;
  std::string host_;

  std::vector<ip::endpoint> endpoints_;

  highp_time_t dns_queries_timestamp_ = 0;

  int index_    = -1;
  int protocol_ = 0;

  // The deadline timer for resolve & connect
  deadline_timer deadline_timer_;

  struct __unnamed01
  {
    int length_field_offset = -1; // -1: directly, >= 0: store as 1~4bytes integer, default value=0
    int length_field_length = 4;  // 1,2,3,4
    int length_adjustment   = 0;
    int max_frame_length    = 10 * 1024 * 1024; // 10MBytes
  } lfb_;
  decode_len_fn_t decode_len_;

  // The buffer for udp only
  std::vector<char> udp_buffer_;
};

class io_transport : public io_base
{
  friend class io_service;

public:
  bool is_open() const { return socket_ != nullptr && socket_->is_open(); }
  ip::endpoint local_endpoint() const { return socket_->local_endpoint(); }
  ip::endpoint peer_endpoint() const { return socket_->peer_endpoint(); }
  int channel_index() const { return ctx_->index(); }
  int status() const { return error_; }
  inline std::vector<char> take_packet()
  {
    expected_packet_size_ = -1;
    return std::move(expected_packet_);
  }

  io_service& get_service() { return ctx_->get_service(); }

private:
  virtual void write(std::vector<char>&&, std::function<void()>&&) = 0;
  virtual int do_read(int& error)                                  = 0;

  // Try flush pending packet
  virtual bool do_write(long long& max_wait_duration) = 0;

protected:
  YASIO__DECL io_transport(io_channel* ctx, std::shared_ptr<xxsocket> sock);
  virtual ~io_transport() {}
  unsigned int id_;

  char buffer_[YASIO_INET_BUFFER_SIZE]; // recv buffer, 64K
  int offset_ = 0;                      // recv buffer offset

  std::vector<char> expected_packet_;
  int expected_packet_size_ = -1;

  io_channel* ctx_;

public:
  void* ud_ = nullptr;
};

class io_transport_posix : public io_transport
{
public:
  io_transport_posix(io_channel* ctx, std::shared_ptr<xxsocket> sock) : io_transport(ctx, sock) {}

private:
  YASIO__DECL void write(std::vector<char>&&, std::function<void()>&&) override;
  YASIO__DECL int do_read(int& error) override;
  YASIO__DECL bool do_write(long long& max_wait_duration) override;

  concurrency::concurrent_queue<a_pdu_ptr> send_queue_;
};

#if defined(YASIO_HAVE_KCP)
class io_transport_kcp : public io_transport
{
public:
  YASIO__DECL io_transport_kcp(io_channel* ctx, std::shared_ptr<xxsocket> sock);
  YASIO__DECL ~io_transport_kcp();

private:
  YASIO__DECL void write(std::vector<char>&&, std::function<void()>&&) override;
  YASIO__DECL int do_read(int& error) override;
  YASIO__DECL bool do_write(long long& max_wait_duration) override;
  ikcpcb* kcp_;
  std::recursive_mutex send_mtx_;
};
#endif

class io_event final
{
public:
  io_event(int channel_index, int kind, int error, transport_handle_t transport)
      : timestamp_(highp_clock()), cindex_(channel_index), kind_(kind), status_(error),
        transport_(std::move(transport))
  {}
  io_event(int channel_index, int type, std::vector<char> packet, transport_handle_t transport)
      : timestamp_(highp_clock()), cindex_(channel_index), kind_(type), status_(0),
        transport_(std::move(transport)), packet_(std::move(packet))
  {}
  io_event(io_event&& rhs)
      : timestamp_(rhs.timestamp_), cindex_(rhs.cindex_), kind_(rhs.kind_), status_(rhs.status_),
        transport_(std::move(rhs.transport_)), packet_(std::move(rhs.packet_))
  {}

  ~io_event() {}

  int cindex() const { return cindex_; }

  int kind() const { return kind_; }
  int status() const { return status_; }

  transport_handle_t transport() { return transport_; }

  std::vector<char>& packet() { return packet_; }
  long long timestamp() const { return timestamp_; }

#if !defined(YASIO_USE_OBJECT_POOL)
  DEFINE_CONCURRENT_OBJECT_POOL_ALLOCATION(io_event, 512)
#endif

private:
  long long timestamp_;
  int cindex_;
  int kind_;
  int status_;
  transport_handle_t transport_;
  std::vector<char> packet_;
};

class io_service
{
  friend class deadline_timer;
  friend class io_transport_posix;
  friend class io_transport_kcp;

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
  YASIO__DECL io_service();
  YASIO__DECL ~io_service();

  // start async socket io service
  YASIO__DECL void start_service(const io_hostent* channel_eps, int channel_count,
                                 io_event_cb_t cb);
  void start_service(const io_hostent* channel_eps, io_event_cb_t cb)
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

  YASIO__DECL void stop_service();
  YASIO__DECL void wait_service();

  bool is_running() const { return this->state_ == io_service::state::RUNNING; }

  // should call at the thread who care about async io
  // events(CONNECT_RESPONSE,CONNECTION_LOST,PACKET), such cocos2d-x opengl or
  // any other game engines' render thread.
  YASIO__DECL void dispatch(int count = 512);

  // This function will be removed in the future, please use dispatch instead.
  YASIO_OBSOLETE_DEPRECATE(yasio::inet::io_service::dispatch)
  YASIO__DECL void dispatch_events(int count = 512) { dispatch(count); }

  /* option: YOPT_CONNECT_TIMEOUT   timeout:int
             YOPT_SEND_TIMEOUT      timeout:int
             YOPT_RECONNECT_TIMEOUT timeout:int
             YOPT_DNS_CACHE_TIMEOUT timeout:int
             YOPT_DEFER_EVENT       defer:int
             YOPT_TCP_KEEPALIVE     idle:int, interal:int, probes:int
             YOPT_RESOLV_FN   func:resolv_fn_t*
             YOPT_PRINT_FN func:print_fn_t, native only, you must ensure thread safe of it.
             YOPT_CHANNEL_LFBFD_PARAMS    index:int, max_frame_length:int, length_field_offst:int,
                                    length_field_length:int, length_adjustment:int
             YOPT_IO_EVENT_CB func:io_event_callback_t*
             YOPT_CHANNEL_LOCAL_PORT  index:int, port:int
             YOPT_CHANNEL_REMOTE_HOST index:int, ip:const char*
             YOPT_CHANNEL_REMOTE_PORT index:int, port:int
             YOPT_CHANNEL_REMOTE_ENDPOINT index:int, ip:const char*, port:int
             YOPT_NO_NEW_THREAD value:int
  */
  YASIO__DECL void set_option(int option, ...);

  // open a channel, default: YCM_TCP_CLIENT
  YASIO__DECL void open(size_t channel_index, int channel_mask = YCM_TCP_CLIENT);

  YASIO__DECL void reopen(transport_handle_t);

  // close transport
  YASIO__DECL void close(transport_handle_t);

  // close channel
  YASIO__DECL void close(size_t channel_index = 0);

  // check whether the transport is open
  YASIO__DECL bool is_open(transport_handle_t) const;

  // check whether the channel is open
  YASIO__DECL bool is_open(size_t cahnnel_index = 0) const;

  YASIO__DECL int write(transport_handle_t transport, std::vector<char> buffer,
                        std::function<void()> = nullptr);

  // The deadlien_timer support, !important, the callback is called on the thread of io_service
  deadline_timer_ptr schedule(highp_time_t duration, timer_cb_t cb, bool repeated = false)
  {
    return schedule(std::chrono::microseconds(duration), std::move(cb), repeated);
  }
  YASIO__DECL deadline_timer_ptr schedule(const std::chrono::microseconds& duration, timer_cb_t,
                                          bool repeated = false);

  YASIO__DECL void cleanup();

  YASIO__DECL int __builtin_resolv(std::vector<ip::endpoint>& endpoints, const char* hostname,
                                   unsigned short port = 0);

private:
  YASIO__DECL void schedule_timer(deadline_timer*, timer_cb_t&);
  YASIO__DECL void remove_timer(deadline_timer*);

  inline std::vector<timer_impl_t>::iterator find_timer(deadline_timer* key)
  {
    return std::find_if(timer_queue_.begin(), timer_queue_.end(),
                        [=](const timer_impl_t& timer) { return timer.first == key; });
  }
  inline void sort_timers()
  {
    std::sort(this->timer_queue_.begin(), this->timer_queue_.end(),
              [](const timer_impl_t& lhs, const timer_impl_t& rhs) {
                return lhs.first->wait_duration() > rhs.first->wait_duration();
              });
  }

  // Start a async resolve, It's only for internal use
  YASIO__DECL void start_resolve(io_channel*);

  YASIO__DECL void init(const io_hostent* channel_eps, int channel_count, io_event_cb_t& cb);

  YASIO__DECL void open_internal(io_channel*, bool ignore_state = false);

  YASIO__DECL void perform_transports(fd_set* fds_array, long long& max_wait_duration);
  YASIO__DECL void perform_channels(fd_set* fds_array);
  YASIO__DECL void perform_timers();

  YASIO__DECL void interrupt();

  YASIO__DECL long long get_wait_duration(long long usec);

  YASIO__DECL int do_evpoll(fd_set* fds_array, long long max_wait_duration);

  YASIO__DECL void do_nonblocking_connect(io_channel*);
  YASIO__DECL void do_nonblocking_connect_completion(io_channel*, fd_set* fds_array);

  inline void handle_connect_succeed(io_channel* ctx, std::shared_ptr<xxsocket> socket)
  {
    handle_connect_succeed(allocate_transport(ctx, std::move(socket)));
  }
  YASIO__DECL void handle_connect_succeed(transport_handle_t);
  YASIO__DECL void handle_connect_failed(io_channel*, int ec);
  YASIO__DECL void notify_connect_succeed(transport_handle_t);

  YASIO__DECL transport_handle_t allocate_transport(io_channel*, std::shared_ptr<xxsocket>);

  YASIO__DECL void register_descriptor(const socket_native_type fd, int flags);
  YASIO__DECL void unregister_descriptor(const socket_native_type fd, int flags);

  // The major non-blocking event-loop
  YASIO__DECL void run(void);

  YASIO__DECL bool do_read(transport_handle_t, fd_set* fds_array, long long& max_wait_duration);
  YASIO__DECL bool do_write(transport_handle_t transport, long long& max_wait_duration)
  {
    return transport->do_write(max_wait_duration);
  }
  YASIO__DECL void unpack(transport_handle_t, int bytes_expected, int bytes_transferred,
                          long long& max_wait_duration);

  // The op mask will be cleared, the state will be set CLOSED
  YASIO__DECL bool cleanup_io(io_base* ctx);

  YASIO__DECL void handle_close(transport_handle_t);
  YASIO__DECL void handle_event(event_ptr event);

  // new/delete client socket connection channel
  // please call this at initialization, don't new channel at runtime
  // dynmaically: because this API is not thread safe.
  YASIO__DECL io_channel* new_channel(const io_hostent& ep);
  // Clear all channels after service exit.
  YASIO__DECL void clear_channels();   // destroy all channels
  YASIO__DECL void clear_transports(); // destroy all transports
  YASIO__DECL bool close_internal(io_channel*);

  /*
  ** Summay: Query async resolve state for new endpoint set
  ** @returns:
  **   YDQS_READY, YDQS_INPRROGRESS, YDQS_FAILED
  ** @remark: will start a async resolv when the state is: YDQS_DIRTY
  */
  YASIO__DECL u_short query_ares_state(io_channel* ctx);

  // supporting server
  YASIO__DECL void do_nonblocking_accept(io_channel*);
  YASIO__DECL void do_nonblocking_accept_completion(io_channel*, fd_set* fds_array);

  YASIO__DECL static const char* strerror(int error);

  YASIO__DECL transport_handle_t make_transport(io_channel*, ip::endpoint& peer);

private:
  state state_; // The service state
  std::thread worker_;
  std::thread::id worker_id_;

  concurrency::concurrent_queue<event_ptr, true> events_;
  concurrency::concurrent_queue<std::function<void()>, true> handlers_;

  std::vector<io_channel*> channels_;

  std::recursive_mutex channel_ops_mtx_;
  std::vector<io_channel*> channel_ops_;

  std::vector<transport_handle_t> transports_;
  std::vector<transport_handle_t> transports_pool_;

#if defined(_WIN32)
  std::map<ip::endpoint, transport_handle_t> dgram_transports_;
#endif

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

  // options
  struct __unnamed_options
  {
    highp_time_t connect_timeout_   = 10LL * MICROSECONDS_PER_SECOND;
    highp_time_t reconnect_timeout_ = -1;
    // Default dns cache time: 10 minutes
    highp_time_t dns_cache_timeout_ = 600LL * MICROSECONDS_PER_SECOND;

    bool deferred_event_   = true;
    bool deferred_handler_ = true;
    // tcp keepalive settings
    struct __unnamed01
    {
      int onoff    = 0;
      int idle     = 7200;
      int interval = 75;
      int probs    = 10;
    } tcp_keepalive_;

    bool no_new_thread_ = false;

    // The resolve function
    resolv_fn_t resolv_;
    // the event callback
    io_event_cb_t on_event_;
    // The custom debug print function
    print_fn_t print_;
  } options_;

  // The ip stack version supported by localhost
  u_short ipsv_ = 0;
}; // io_service
} // namespace inet
} /* namespace yasio */

#define yasio_shared_service yasio::gc::singleton<yasio::inet::io_service>::instance()

#if defined(YASIO_HEADER_ONLY)
#  include "yasio/yasio.cpp"
#endif

#endif
