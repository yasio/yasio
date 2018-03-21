//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
// version: 3.0-developing
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
#include "async_tcp_client.h"
#include <limits>
#include <stdarg.h>
#include <string>

#define _USING_IN_COCOS2DX 0

#define INET_ENABLE_VERBOSE_LOG 0

namespace {
    /*--- This is a C++ universal sprintf in the future.
    **  @pitfall: The behavior of vsnprintf between VS2013 and VS2015/2017 is different
    **      VS2013 or Unix-Like System will return -1 when buffer not enough, but VS2015/2017 will return the actural needed length for buffer at this station
    **      The _vsnprintf behavior is compatible API which always return -1 when buffer isn't enough at VS2013/2015/2017
    **      Yes, The vsnprintf is more efficient implemented by MSVC 19.0 or later, AND it's also standard-compliant, see reference: http://www.cplusplus.com/reference/cstdio/vsnprintf/
    */
    std::string _string_format(const char* format, ...)
    {
#define CC_VSNPRINTF_BUFFER_LENGTH 512
        va_list args;
        std::string buffer(CC_VSNPRINTF_BUFFER_LENGTH, '\0');

        va_start(args, format);
        int nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
        va_end(args);

        if (nret >= 0) {
            if ((unsigned int)nret < buffer.length()) {
                buffer.resize(nret);
            }
            else if ((unsigned int)nret > buffer.length()) { // VS2015/2017 or later Visual Studio Version
                buffer.resize(nret);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

                assert(nret == buffer.length());
            }
            // else equals, do nothing.
        }
        else { // less or equal VS2013 and Unix System glibc implement.
            do {
                buffer.resize(buffer.length() * 3 / 2);

                va_start(args, format);
                nret = vsnprintf(&buffer.front(), buffer.length() + 1, format, args);
                va_end(args);

            } while (nret < 0);

            buffer.resize(nret);
        }

        return buffer;
    }
}

#if _USING_IN_COCOS2DX
#include "cocos2d.h"
#if COCOS2D_VERSION >= 0x00030000
#define INET_LOG(format,...) do { \
   std::string msg = _string_format(("[%lld]" format "\r\n"), static_cast<long long>(time(NULL)), ##__VA_ARGS__); \
   cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([=] {cocos2d::log("%s", msg.c_str()); }); \
} while(false)
#else
#define INET_LOG(format,...) do { \
   std::string msg = _string_format(("[%lld]" format "\r\n"), static_cast<long long>(time(NULL)), ##__VA_ARGS__); \
   cocos2d::CCDirector::sharedDirector()->getScheduler()->performFunctionInCocosThread([=] {cocos2d::CCLog("%s", msg.c_str()); }); \
} while(false)
#endif
#else
#if defined(_WIN32)
#define INET_LOG(format,...) OutputDebugStringA(_string_format(("[%lld]" format "\r\n"), static_cast<long long>(time(NULL)), ##__VA_ARGS__).c_str())
#else
#define INET_LOG(format,...) fprintf(stdout,("[%lld]" format "\n"), static_cast<long long>(time(NULL)), ##__VA_ARGS__)
#endif
#endif


#ifdef _WIN32
#if !defined(WINRT)
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")
#endif
#endif

#define MAX_WAIT_DURATION 5 * 60 * 1000 * 1000 // 5 minites

#define MAX_PDU_LEN SZ(10, M)

#define TSF_CALL(stmt) this->tsf_call_([=]{(stmt);});

namespace purelib {
namespace inet {
class appl_pdu
{
public:
    appl_pdu(std::vector<char>&& right, const appl_pdu_send_callback_t& callback, const std::chrono::microseconds& duration)
    {
        data_ = std::move(right);
        offset_ = 0;
        on_sent_ = callback;
        expire_time_ = std::chrono::steady_clock::now() + duration;
    }
    bool expired() const {
        return (expire_time_ - std::chrono::steady_clock::now()).count() < 0;
    }
    std::vector<char>          data_; // sending data
    size_t                     offset_; // offset
    appl_pdu_send_callback_t   on_sent_;
    compatible_timepoint_t     expire_time_;

    DEFINE_OBJECT_POOL_ALLOCATION(appl_pdu, 512)
};

void channel_context::reset()
{
    state_ = channel_state::INACTIVE;
    receiving_pdu_.clear();
    offset_ = 0;
    expected_pdu_length_ = -1;
    error_ = 0;
    resolve_state_ = resolve_state::IDLE;
    index_ = static_cast<size_t>(-1);
}

async_tcp_client::async_tcp_client() : stopping_(false),
    thread_started_(false),
    interrupter_(),
    connect_timeout_(3LL * 1000 * 1000),
    send_timeout_((std::numeric_limits<int>::max)()),
    decode_pdu_length_(nullptr),
    error_(error_number::ERR_OK)
{
    FD_ZERO(&fds_array_[read_op]);
    FD_ZERO(&fds_array_[write_op]);
    FD_ZERO(&fds_array_[read_op]);

    maxfdp_ = 0;
    nfds_ = 0;
}

async_tcp_client::~async_tcp_client()
{
    stop_service();
}

void async_tcp_client::stop_service()
{
    if (thread_started_) {
        stopping_ = true;

        this->resolver_ops_cv_.notify_all();
        if (this->resolver_thread_.joinable())
            this->resolver_thread_.join();

        this->resolver_ops_mtx_.lock();
        this->resolver_ops_.clear();
        this->resolver_ops_mtx_.unlock();

        for (auto ctx : channels_)
        {
            cancel_connect_timeout_timer(ctx);
            if (ctx->impl_.is_open()) {
                ctx->impl_.shutdown();
            }
        }

        interrupter_.interrupt();
        if (this->worker_thread_.joinable())
            this->worker_thread_.join();

        clear_channels();

        unregister_descriptor(interrupter_.read_descriptor(), socket_event_read);
        thread_started_ = false;
    }
}

void async_tcp_client::set_timeouts(long timeo_connect, long timeo_send)
{
    this->connect_timeout_ = static_cast<long long>(timeo_connect) * 1000 * 1000;
    this->send_timeout_ = timeo_send;
}

channel_context* async_tcp_client::new_channel(const channel_endpoint& ep)
{
    auto ctx = new channel_context();
    ctx->reset();
    ctx->address_ = ep.address_;
    ctx->addressv6_ = ep.addressv6_;
    ctx->port_ = ep.port_;
    ctx->index_ = this->channels_.size();
    this->channels_.push_back(ctx);
    return ctx;
}

void async_tcp_client::clear_channels()
{
    for (auto iter = channels_.begin(); iter != channels_.end(); ) {
        (*iter)->impl_.close();
        delete *(iter);
        iter = channels_.erase(iter);
    }
}

void async_tcp_client::set_callbacks(
    decode_pdu_length_func decode_length_func,
    const connect_response_callback_t& on_connect_response,
    const connection_lost_callback_t& on_connection_lost,
    const appl_pdu_recv_callback_t& on_pdu_recv,
    const std::function<void(const vdcallback_t&)>& threadsafe_call)
{
    this->decode_pdu_length_ = decode_length_func;
    this->on_connect_resposne_ = on_connect_response;
    this->on_received_pdu_ = on_pdu_recv;
    this->on_connection_lost_ = on_connection_lost;
    this->tsf_call_ = threadsafe_call;
}

size_t async_tcp_client::get_received_pdu_count(void) const
{
    return recv_queue_.size();
}

void async_tcp_client::dispatch_received_pdu(int count) {
    assert(this->on_received_pdu_ != nullptr);

    if (this->recv_queue_.empty())
        return;

    std::lock_guard<std::mutex> autolock(this->recv_queue_mtx_);
    do {
        auto packet = std::move(this->recv_queue_.front());
        this->recv_queue_.pop_front();
        this->on_received_pdu_(std::move(packet));
    } while (!this->recv_queue_.empty() && --count > 0);
}

void async_tcp_client::start_service(const channel_endpoint* channel_eps, int channel_count)
{
    if (!thread_started_) {
        if (channel_count <= 0)
            return;

        stopping_ = false;
        thread_started_ = true;

        register_descriptor(interrupter_.read_descriptor(), socket_event_read);

        resolver_thread_ = std::thread([this] {
            this->resolve_service();
        });

        // Initialize channels
        for (auto i = 0; i < channel_count; ++i)
        {
            auto& channel_ep = channel_eps[i];
            auto ctx = new_channel(channel_ep);
            if (channel_ep.port_ > 0) {
                async_resolve(ctx);
            }
        }

        worker_thread_ = std::thread([this] {
            INET_LOG("async_tcp_client thread running...");
            this->service();
            INET_LOG("async_tcp_client thread exited.");
        });
    }
}

void  async_tcp_client::set_endpoint(size_t channel_index, const char* address, const char* addressv6, u_short port)
{
    // Gets channel context
    if (channel_index >= channels_.size())
        return;
    auto ctx = channels_[channel_index];

    ctx->address_ = address;
    ctx->addressv6_ = addressv6;
    ctx->port_ = port;

    ctx->resolve_state_ = ctx->port_ > 0 ? resolve_state::DIRTY : resolve_state::IDLE;
}

void async_tcp_client::service()
{
#if defined(_WIN32) && !defined(WINRT)
    timeBeginPeriod(1);
#endif

    // event loop
    fd_set fds_array[3];
    timeval timeout;

    for (; !stopping_;)
    {
        int nfds = do_select(fds_array, timeout);

        if (stopping_) break;

        if (nfds == -1)
        {
            int ec = xxsocket::get_last_errno();
            INET_LOG("socket.select failed, error code: %d, error msg:%s\n", ec, xxsocket::get_error_msg(ec));
            if (ec == EBADF) {
                goto _L_end;
            }
            continue; // try select again.
        }

        if (nfds == 0) {
#if INET_ENABLE_VERBOSE_LOG
            INET_LOG("socket.select is timeout, do perform_timeout_timers()");
#endif
        }
        // Reset the interrupter.
        else if (nfds > 0 && FD_ISSET(this->interrupter_.read_descriptor(), &(fds_array[read_op])))
        {
            bool was_interrupt = interrupter_.reset();
#if INET_ENABLE_VERBOSE_LOG
            INET_LOG("socket.select waked up by interrupt, interrupter fd:%d, was_interrupt:%s", this->interrupter_.read_descriptor(), was_interrupt ? "true" : "false");
#endif
            --nfds;
        }

        for (auto ctx : channels_) {
            if (ctx->state_ == channel_state::CONNECTED) {
                if (FD_ISSET(ctx->impl_.native_handle(), &(fds_array[read_op])) || ctx->offset_ > 0) {
#if INET_ENABLE_VERBOSE_LOG
                    INET_LOG("perform read operation...");
#endif
                    if (!do_read(ctx)) {
                        INET_LOG("do read for %s failed!", ctx->address_.c_str());
                        handle_error(ctx);
                    }
                }

                // perform write operations
                if (!ctx->send_queue_.empty()) {
                    ctx->send_queue_mtx_.lock();
#if INET_ENABLE_VERBOSE_LOG
                    INET_LOG("perform write operation...");
#endif
                    if (!do_write(ctx))
                    { // TODO: check would block? for client, may be unnecessary.
                        ctx->send_queue_mtx_.unlock();
                        handle_error(ctx);
                    }

                    ctx->send_queue_mtx_.unlock();
                }

                if (!ctx->send_queue_.empty()) ++this->nfds_;
                if (ctx->offset_ > 0)++this->nfds_;
            }
            else if (ctx->state_ == channel_state::REQUEST_CONNECT) {
                do_nonblocking_connect(ctx);
            }
            else if (ctx->state_ == channel_state::CONNECTING) {
                do_nonblocking_connect_completion(fds_array, ctx);
            }
        }
            
        perform_timeout_timers();
    }

_L_end:
#if defined(_WIN32) && !defined(WINRT)
    timeEndPeriod(1);
#endif
}

void  async_tcp_client::resolve_service(void)
{
    for (; !stopping_;)
    {
        std::unique_lock<std::mutex> lk(this->resolver_ops_mtx_);
        this->resolver_ops_cv_.wait(lk, [this] {return stopping_ || !this->resolver_ops_.empty(); });

        if (!stopping_) {
            auto ctx = this->resolver_ops_.front();
            this->resolver_ops_.pop_front();
            lk.unlock();

            bool succeed = false;
            int flags = xxsocket::getipsv();
            if (flags & ipsv_ipv4) {
                ctx->endpoint_ = xxsocket::resolve(ctx->address_.c_str(), ctx->port_);
                succeed = ctx->endpoint_.port() > 0;
            }
            else if (flags & ipsv_ipv6)
            { // client is IPV6_Only
                INET_LOG("async_tcp_client::resolve_service ---> needs a ipv6 server address to connect!");
                ctx->endpoint_ = xxsocket::resolve(ctx->addressv6_.c_str(), ctx->port_);
                succeed = ctx->endpoint_.port() > 0;
            }

            ctx->resolve_state_ = succeed ? resolve_state::READY : resolve_state::FAILED;

            this->set_errorno(ctx, xxsocket::get_last_errno());
            if (!succeed) {
                INET_LOG("async_tcp_client::resolve_service ---> resolve failed, ec:%d, detail:%s", error_, xxsocket::get_error_msg(error_));
            }

            if (succeed && ctx->state_ == channel_state::REQUEST_CONNECT)
                this->interrupter_.interrupt();
        }
    }
}

bool async_tcp_client::async_resolve(channel_context* ctx)
{
    if (ctx->port_ == 0) {
        ctx->resolve_state_ = resolve_state::IDLE;
        return false;
    }

    std::unique_lock<std::mutex> lk(this->resolver_ops_mtx_);
    this->resolver_ops_.push_back(ctx);
    this->resolver_ops_cv_.notify_one();
    return true;
}

void async_tcp_client::do_nonblocking_connect(channel_context* ctx)
{
    assert(ctx->state_ == channel_state::REQUEST_CONNECT);

    if (ctx->state_ == channel_state::REQUEST_CONNECT)
    {
        if (ctx->resolve_state_ == resolve_state::READY) {

            if (ctx->impl_.is_open()) { // cleanup descriptor if possible
                cleanup_descriptor(ctx);
            }

            INET_LOG("connecting server %s:%u...", ctx->address_.c_str(), ctx->port_);

            ctx->state_ = channel_state::CONNECTING;
            int ret = ctx->impl_.pconnect_n(ctx->endpoint_);

            if (ret < 0)
            { // setup no blocking connect
                int error = xxsocket::get_last_errno();
                if (error != EINPROGRESS && error != EWOULDBLOCK) {
                    this->handle_connect_failed(ctx, error);
                    return;
                }
                else {
                    this->set_errorno(ctx, error);

                    register_descriptor(ctx->impl_.native_handle(), socket_event_read | socket_event_write);
                    ctx->impl_.set_optval(SOL_SOCKET, SO_REUSEADDR, 1); // set opt for p2p

                    ctx->expected_pdu_length_ = -1;
                    ctx->offset_ = 0;
                    ctx->receiving_pdu_.clear();

                    start_connect_timeout_timer(ctx);
                }
            }
            else if (ret == 0) { // connect server succed immidiately.
                handle_connect_succeed(ctx, error_number::ERR_OK);
            }
            else // MAY NEVER GO HERE
                this->handle_connect_failed(ctx, ctx->error_);
        }
        else if (ctx->resolve_state_ == resolve_state::FAILED) {
            handle_connect_failed(ctx, ERR_RESOLVE_HOST_FAILED); 
        } // DIRTY,IDLE do nothing
    }
}

void async_tcp_client::start_connect_timeout_timer(channel_context* ctx)
{
    ctx->connect_timeout_timer_ = new deadline_timer(*this);
    ctx->connect_timeout_timer_->expires_from_now(std::chrono::microseconds(this->connect_timeout_));
    ctx->connect_timeout_timer_->async_wait([this, ctx](bool cancelled) {
        if (!cancelled && ctx->state_ != channel_state::CONNECTED) {
            handle_connect_failed(ctx, ERR_CONNECT_TIMEOUT);
        }
    });
}

void async_tcp_client::cancel_connect_timeout_timer(channel_context* ctx)
{
    if (ctx->connect_timeout_timer_ != nullptr) {
        ctx->connect_timeout_timer_->cancel();
        delete ctx->connect_timeout_timer_;
        ctx->connect_timeout_timer_ = nullptr;
    }
}

void async_tcp_client::close(size_t channel_index)
{
    // Gets channel context
    if (channel_index >= channels_.size())
        return;
    auto ctx = channels_[channel_index];

    if (ctx->impl_.is_open()) {
        ctx->impl_.shutdown();
        // interrupter_.interrupt();
    }
}

bool async_tcp_client::is_connected(size_t channel_index) const
{
    // Gets channel
    if (channel_index >= channels_.size())
        return false;
    auto ctx = channels_[channel_index];
    return ctx->state_ == channel_state::CONNECTED;
}

void async_tcp_client::async_connect(size_t channel_index)
{
    // Gets channel
    if (channel_index >= channels_.size())
        return;
    auto ctx = channels_[channel_index];

    if (ctx->state_ == channel_state::REQUEST_CONNECT || 
        ctx->state_ == channel_state::CONNECTING) 
    { // in-progress, do nothing
        INET_LOG("async_connect --> the connect request is already in progress!");
        return;
    } 

    if (ctx->resolve_state_ != resolve_state::READY) {
        if (!async_resolve(ctx)) {
            handle_connect_failed(ctx, ERR_INVALID_PORT);
            return;
        }
    }
    ctx->state_ = channel_state::REQUEST_CONNECT;
    if (ctx->impl_.is_open()) {
        ctx->impl_.shutdown();
    }
    else interrupter_.interrupt();
}

void async_tcp_client::handle_error(channel_context* ctx)
{
    if (!cleanup_descriptor(ctx)) {
        INET_LOG("local close the connection!");
    }

    // @Notify connection lost
    if (this->on_connection_lost_ && ctx->state_ == channel_state::CONNECTED) {
        TSF_CALL(on_connection_lost_(ctx->index_, ctx->error_, xxsocket::get_error_msg(ctx->error_)));
    }

    ctx->state_ = channel_state::INACTIVE;

    // @Clear all sending messages
    if (!ctx->send_queue_.empty()) {
        ctx->send_queue_mtx_.lock();

        for (auto pdu : ctx->send_queue_)
            delete pdu;
        ctx->send_queue_.clear();

        ctx->send_queue_mtx_.unlock();
    }

    // @Notify all timers are cancelled.
    if (!this->timer_queue_.empty()) {
        this->timer_queue_mtx_.lock();
        //for (auto& timer : timer_queue_)
        //    timer->callback_(true);
        this->timer_queue_.clear();
        this->timer_queue_mtx_.unlock();
    }
}

void async_tcp_client::register_descriptor(const socket_native_type fd, int flags)
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

void async_tcp_client::unregister_descriptor(const socket_native_type fd, int flags)
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

void async_tcp_client::async_send(std::vector<char>&& data, size_t channel_index, const appl_pdu_send_callback_t& callback)
{
    // Gets channel
    if (channel_index >= channels_.size())
        return;
    auto ctx = channels_[channel_index];

    if (ctx->impl_.is_open())
    {
        auto pdu = new appl_pdu(std::move(data), callback, std::chrono::seconds(this->send_timeout_));

        ctx->send_queue_mtx_.lock();
        ctx->send_queue_.push_back(pdu);
        ctx->send_queue_mtx_.unlock();

        interrupter_.interrupt();
    }
    else {
        INET_LOG("async_tcp_client::send failed, The connection not ok!!!");
    }
}

void async_tcp_client::move_received_pdu(channel_context* ctx)
{
#if INET_ENABLE_VERBOSE_LOG
    INET_LOG("move_received_pdu...");
#endif
    recv_queue_mtx_.lock();
    recv_queue_.push_back(std::move(ctx->receiving_pdu_)); // without data copy
    recv_queue_mtx_.unlock();

    ctx->expected_pdu_length_ = -1;
}

void async_tcp_client::do_nonblocking_connect_completion(fd_set* fds_array, channel_context* ctx)
{
    if (ctx->state_ == channel_state::CONNECTING)
    {
        int error = -1;
        if (FD_ISSET(ctx->impl_.native_handle(), &fds_array[write_op])
            || FD_ISSET(ctx->impl_.native_handle(), &fds_array[read_op])) {
            socklen_t len = sizeof(error);
            if (::getsockopt(ctx->impl_.native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &len) >= 0 && error == 0) {
                handle_connect_succeed(ctx, error);
                cancel_connect_timeout_timer(ctx);
            }
            else {
                handle_connect_failed(ctx, ERR_CONNECT_FAILED);
                cancel_connect_timeout_timer(ctx);
            }
        }
        else { // Check whether connect is timeout.
#if 0 // Will check by deadline_timer automatically
            auto wait_duration = get_connect_wait_duration(ctx);
            if (wait_duration <= 0) { // connect failed
                handle_connect_failed(ctx, ERR_CONNECT_TIMEOUT);
                cancel_connect_timeout_timer(ctx);
            } // else INET_LOG("continue waiting connection: [local] --> [%s:%u] to establish...", ctx->address_.c_str(), ctx->port_);
#endif
        }
    }
}

void async_tcp_client::handle_connect_succeed(channel_context* ctx, int error) 
{
    ctx->state_ = channel_state::CONNECTED;
    unregister_descriptor(ctx->impl_.native_handle(), socket_event_write); // remove write event avoid high-CPU occupation

    this->set_errorno(ctx, error);
    TSF_CALL(this->on_connect_resposne_(ctx->index_, true, error));
    INET_LOG("connect server %s:%u succeed, local endpoint[%s]", ctx->address_.c_str(), ctx->port_, ctx->impl_.local_endpoint().to_string().c_str());
}

void async_tcp_client::handle_connect_failed(channel_context* ctx, int error)
{
    cleanup_descriptor(ctx);

    ctx->state_ = channel_state::INACTIVE;
    this->set_errorno(ctx, error);

    TSF_CALL(this->on_connect_resposne_(ctx->index_, false, error));

    INET_LOG("connect server %s:%u failed, error(%d)!", ctx->address_.c_str(), ctx->port_, error); 
}

bool async_tcp_client::do_write(channel_context* ctx)
{
    if (ctx->state_ != channel_state::CONNECTED)
        return true;

    bool bRet = false;

    do {
        int n;

        if (!ctx->impl_.is_open())
            break;

        if (!ctx->send_queue_.empty()) {
            auto& v = ctx->send_queue_.front();
            auto bytes_left = v->data_.size() - v->offset_;
            n = ctx->impl_.send_i(v->data_.data() + v->offset_, bytes_left);
            if (n == bytes_left) { // All pdu bytes sent.
                ctx->send_queue_.pop_front();
                this->tsf_call_([v] {
#if INET_ENABLE_VERBOSE_LOG
                    auto packetSize = v->data_.size();
                    INET_LOG("async_tcp_client::do_write ---> A packet sent success, packet size:%d", packetSize);
#endif
                    if (v->on_sent_ != nullptr)
                        v->on_sent_(error_number::ERR_OK);
                    delete v;
                });
            }
            else if (n > 0) { // TODO: add time
                if (!v->expired())
                { // change offset, remain data will send next time.
                    // v->data_.erase(v->data_.begin(), v->data_.begin() + n);
                    v->offset_ += n;
                    int temp_left = v->data_.size() - v->offset_;
                    INET_LOG("send not complete %d bytes remained, %dbytes was sent!", temp_left, n);
                }
                else { // send timeout
                    ctx->send_queue_.pop_front();
                    this->tsf_call_([v] {

                        auto packetSize = v->data_.size();
                        INET_LOG("async_tcp_client::do_write ---> A packet sent timeout, packet size:%d", packetSize);

                        if (v->on_sent_)
                            v->on_sent_(error_number::ERR_SEND_TIMEOUT);
                        delete v;
                    });
                }
            }
            else { // n <= 0, TODO: add time
                set_errorno(ctx, xxsocket::get_last_errno());
                if (SHOULD_CLOSE_1(n, error_)) {
                    ctx->send_queue_.pop_front();

                    INET_LOG("async_tcp_client::do_write failed, the connection should be closed, retval=%d, socket error:%d, detail:%s", n, error_, xxsocket::get_error_msg(error_));

                    this->tsf_call_([v] {
                        if (v->on_sent_)
                            v->on_sent_(error_number::ERR_CONNECTION_LOST);
                        delete v;
                    });

                    break;
                }
            }
        }

        bRet = true;
    } while (false);

    return bRet;
}

bool async_tcp_client::do_read(channel_context* ctx)
{
    if (ctx->state_ != channel_state::CONNECTED)
        return true;

    bool bRet = false;
    do {
        if (!ctx->impl_.is_open())
            break;

        int n = ctx->impl_.recv_i(ctx->buffer_ + ctx->offset_, sizeof(ctx->buffer_) - ctx->offset_);

        if (n > 0 || (n == -1 && ctx->offset_ != 0)) {

            if (n == -1)
                n = 0;
#if INET_ENABLE_VERBOSE_LOG
            INET_LOG("async_tcp_client::do_read --- received data len: %d, buffer data len: %d", n, n + ctx->offset_);
#endif
            if (ctx->expected_pdu_length_ == -1) { // decode length
                if (decode_pdu_length_(ctx->buffer_, ctx->offset_ + n, ctx->expected_pdu_length_))
                {
                    if (ctx->expected_pdu_length_ < 0) { // header insuff
                        ctx->offset_ += n;
                    }
                    else { // ok
                        ctx->receiving_pdu_.reserve(ctx->expected_pdu_length_); // #perfomance, avoid memory reallocte.

                        auto bytes_transferred = n + ctx->offset_;
                        ctx->receiving_pdu_.insert(ctx->receiving_pdu_.end(), ctx->buffer_, ctx->buffer_ + (std::min)(ctx->expected_pdu_length_, bytes_transferred));
                        if (bytes_transferred >= ctx->expected_pdu_length_)
                        { // pdu received properly
                            ctx->offset_ = bytes_transferred - ctx->expected_pdu_length_; // set offset to bytes of remain buffer
                            if (ctx->offset_ > 0)  // move remain data to head of buffer and hold offset.
                                ::memmove(ctx->buffer_, ctx->buffer_ + ctx->expected_pdu_length_, ctx->offset_);

                            // move properly pdu to ready queue, GL thread will retrieve it.
                            move_received_pdu(ctx);
                        }
                        else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                            ctx->offset_ = 0;
                        }
                    }
                }
                else {
                    set_errorno(ctx, error_number::ERR_DPL_ILLEGAL_PDU);
                    INET_LOG("%s", "async_tcp_client::do_read error, decode length of pdu failed!");
                    break;
                }
            }
            else { // recv remain pdu data
                auto bytes_transferred = n + ctx->offset_;// bytes transferred at this time
                if ((ctx->receiving_pdu_.size() + bytes_transferred) > MAX_PDU_LEN) // TODO: config MAX_PDU_LEN, now is 16384
                {
                    set_errorno(ctx, error_number::ERR_PDU_TOO_LONG);
                    INET_LOG("%s", "async_tcp_client::do_read error, The length of pdu too long!");
                    break;
                }
                else {
                    auto bytes_needed = ctx->expected_pdu_length_ - static_cast<int>(ctx->receiving_pdu_.size()); // never equal zero or less than zero
                    if (bytes_needed > 0) {
                        ctx->receiving_pdu_.insert(ctx->receiving_pdu_.end(), ctx->buffer_, ctx->buffer_ + (std::min)(bytes_transferred, bytes_needed));

                        ctx->offset_ = bytes_transferred - bytes_needed; // set offset to bytes of remain buffer
                        if (ctx->offset_ >= 0)
                        { // pdu received properly
                            if (ctx->offset_ > 0)  // move remain data to head of buffer and hold offset.
                                ::memmove(ctx->buffer_, ctx->buffer_ + bytes_needed, ctx->offset_);

                            // move properly pdu to ready queue, GL thread will retrieve it.
                            move_received_pdu(ctx);
                        }
                        else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                            ctx->offset_ = 0;
                        }
                    }
                    else {
                        //assert(false);
                    }
                }
            }
        }
        else {
            this->set_errorno(ctx, xxsocket::get_last_errno());
            if (SHOULD_CLOSE_0(n, error_)) {
                const char* errormsg = xxsocket::get_error_msg(error_);
                if (n == 0) {
                    INET_LOG("async_tcp_client::do_read error, the server close the connection, retval=%d, socket error:%d, detail:%s", n, error_, errormsg);
                }
                else {
                    INET_LOG("async_tcp_client::do_read error, the connection should be closed, retval=%d, socket error:%d, detail:%s", n, error_, errormsg);
                }
                break;
            }
        }

        bRet = true;

    } while (false);

    return bRet;
}

void async_tcp_client::schedule_timer(deadline_timer* timer)
{
    // pitfall: this service only hold the weak pointer of the timer object, so before dispose the timer object
    // need call cancel_timer to cancel it.
    if(timer == nullptr)
        return;
    
    std::lock_guard<std::recursive_mutex> lk(this->timer_queue_mtx_);
    if (std::find(timer_queue_.begin(), timer_queue_.end(), timer) != timer_queue_.end())
        return;

    this->timer_queue_.push_back(timer);

    std::sort(this->timer_queue_.begin(), this->timer_queue_.end(), [](deadline_timer* lhs, deadline_timer* rhs) {
        return lhs->wait_duration() > rhs->wait_duration();
    });

    if (timer == *this->timer_queue_.begin())
        interrupter_.interrupt();
}

void async_tcp_client::cancel_timer(deadline_timer* timer)
{
    std::lock_guard<std::recursive_mutex> lk(this->timer_queue_mtx_);

    auto iter = std::find(timer_queue_.begin(), timer_queue_.end(), timer);
    if (iter != timer_queue_.end()) {
        auto callback = timer->callback_;
        callback(true);
        timer_queue_.erase(iter);
    }
}

void async_tcp_client::perform_timeout_timers()
{
    if (this->timer_queue_.empty())
        return;

    std::lock_guard<std::recursive_mutex> lk(this->timer_queue_mtx_);

    std::vector<deadline_timer*> loop_timers;
    while (!this->timer_queue_.empty())
    {
        auto earliest = timer_queue_.back();
        if (earliest->expired())
        {
            timer_queue_.pop_back();
            auto callback = earliest->callback_;
            callback(false);
            if (earliest->loop_) {
                earliest->expires_from_now();
                loop_timers.push_back(earliest);
            }
        }
        else {
            break;
        }
    }

    if (!loop_timers.empty()) {
        this->timer_queue_.insert(this->timer_queue_.end(), loop_timers.begin(), loop_timers.end());
        std::sort(this->timer_queue_.begin(), this->timer_queue_.end(), [](deadline_timer* lhs, deadline_timer* rhs) {
            return lhs->wait_duration() > rhs->wait_duration();
        });
    }
}

int async_tcp_client::do_select(fd_set* fds_array, timeval& tv)
{
    /*
    @Optimize, swap nfds, make sure do_read & do_write event chould be perform when no need to call socket.select
    However, the connection exception will detected through do_read or do_write, but it's ok.
    */
    int nfds = 0;
    std::swap(nfds, this->nfds_);
    ::memcpy(fds_array, this->fds_array_, sizeof(this->fds_array_));
    if (nfds <= 0) {
        auto wait_duration = get_wait_duration(MAX_WAIT_DURATION);
        if (wait_duration > 0) {
            tv.tv_sec = static_cast<long>(wait_duration / 1000000);
            tv.tv_usec = static_cast<long>(wait_duration % 1000000);
#if INET_ENABLE_VERBOSE_LOG
            INET_LOG("socket.select maxfdp:%d waiting... %ld milliseconds", maxfdp_, timeout.tv_sec * 1000 + timeout.tv_usec / 1000);
#endif
            nfds = ::select(this->maxfdp_, &(fds_array[read_op]), &(fds_array[write_op]), nullptr, &tv);
#if INET_ENABLE_VERBOSE_LOG
            INET_LOG("socket.select waked up, retval=%d", nfds);
#endif
        }
        else {
            nfds = static_cast<int>(channels_.size()) << 1;
        }
    }

    return nfds;
}

long long  async_tcp_client::get_wait_duration(long long usec)
{
    if(this->timer_queue_.empty())
    {
        return usec;
    }
    
    std::lock_guard<std::recursive_mutex> autolock(this->timer_queue_mtx_);
    deadline_timer* earliest = timer_queue_.back();
    
    // microseconds
    auto duration = earliest->wait_duration();
    if (std::chrono::microseconds(usec) > duration)
        return duration.count();
    else
        return usec;
}

bool async_tcp_client::cleanup_descriptor(channel_context* ctx)
{
    if (ctx->impl_.is_open()) {
        unregister_descriptor(ctx->impl_.native_handle(), socket_event_read | socket_event_write);
        ctx->impl_.close();
        return true;
    }
    return false;
}

int async_tcp_client::set_errorno(channel_context* ctx, int error)
{
    ctx->error_ = error;
    error_ = error;
    return error;
}

} /* namespace purelib::net */
} /* namespace purelib */
