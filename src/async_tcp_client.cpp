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

#define _USING_IN_COCOS2DX 0

#define INET_ENABLE_VERBOSE_LOG 0

#if _USING_IN_COCOS2DX
#include "cocos2d.h"
#define INET_LOG(format,...) cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]{cocos2d::log((format), ##__VA_ARGS__);})
#else
#if defined(_WIN32)
static void odprintf(const char* format, ...) {
    va_list	args;
    va_start(args, format);
    int len = _vscprintf(format, args);
    if (len > 0) {
        len += (1 + 2);
        char* buf = (char*)malloc(len);
        if (buf) {
            len = vsprintf_s(buf, len, format, args);
            if (len > 0) {
                while (len && isspace(buf[len - 1])) len--;
                buf[len++] = '\r';
                buf[len++] = '\n';
                buf[len] = 0;
                OutputDebugStringA(buf);
            }
            free(buf);
        }
        va_end(args);
    }
}
#define INET_LOG(format,...) odprintf((format "\r\n"),##__VA_ARGS__) // fprintf(stdout,(format "\n"),##__VA_ARGS__)
#else
#define INET_LOG(format,...) fprintf(stdout,(format "\n"),##__VA_ARGS__)
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
    state_ = channel_state::IDLE;
    receiving_pdu_.clear();
    offset_ = 0;
    expected_pdu_length_ = -1;
    error = 0;
    report_error_ = true;
    reconnecting_ = false;
    index_ = static_cast<size_t>(-1);
    if (this->impl_.is_open()) {
        this->impl_.close();
    }
}

async_tcp_client::async_tcp_client() : app_exiting_(false),
    thread_started_(false),
    interrupter_(),
    connect_timeout_(3LL * 1000 * 1000),
    send_timeout_((std::numeric_limits<int>::max)()),
    decode_pdu_length_(nullptr),
    error_number_(error_number::ERR_OK)
{
    FD_ZERO(&fds_array_[read_op]);
    FD_ZERO(&fds_array_[write_op]);
    FD_ZERO(&fds_array_[read_op]);

    maxfdp_ = 0;
}

async_tcp_client::~async_tcp_client()
{
    app_exiting_ = true;

    close();

    if (this->worker_thread_.joinable())
        this->worker_thread_.join();

    clear_channels();
}

void async_tcp_client::set_timeouts(long timeo_connect, long timeo_send)
{
    this->connect_timeout_ = static_cast<long long>(timeo_connect) * 1000 * 1000;
    this->send_timeout_ = timeo_send;
}

void  async_tcp_client::new_channel(const std::string& address, const std::string& addressv6, u_short port)
{
    auto channel = new channel_context();
    channel->reset();
    channel->address_ = address;
    channel->addressv6_ = addressv6;
    channel->port_ = port;
    channel->index_ = this->channels_.size();
    this->channels_.push_back(channel);
}

void async_tcp_client::clear_channels()
{
    for (auto iter = channels_.begin(); iter != channels_.end(); ) {
        (*iter)->impl_.close();
        delete *(iter);
        channels_.erase(iter++);
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

        // Initialize channels
        for (auto i = 0; i < channel_count; ++i)
        {
            auto& channel_ep = channel_eps[i];
            new_channel(channel_ep.address_, channel_ep.addressv6_, channel_ep.port_);
        }

        thread_started_ = true;
        worker_thread_ = std::thread([this] {
            INET_LOG("async_tcp_client thread running...");
            this->service();
            INET_LOG("async_tcp_client thread exited.");
        });
    }
}

void  async_tcp_client::set_endpoint(size_t channel_index, const char* address, const char* addressv6, u_short port)
{
    // Gets channel
    if (channel_index >= channels_.size())
        return;
    auto channel = channels_[channel_index];

    channel->address_ = address;
    channel->addressv6_ = addressv6;
    channel->port_ = port;
}

void async_tcp_client::service()
{
#if defined(_WIN32) && !defined(WINRT)
    timeBeginPeriod(1);
#endif

    // event loop
    fd_set fds_array[3];
    timeval timeout;

    register_descriptor(interrupter_.read_descriptor(), socket_event_read);

    for (; !app_exiting_;)
    {
        int nfds = static_cast<int>(channels_.size());
        std::chrono::microseconds wait_duration(MAX_WAIT_DURATION);
        int pending_connects = 0;
        for (auto channel : channels_) {
            switch (channel->state_) {
            case channel_state::REQUEST_CONNECT:
                wait_duration = (std::min)(wait_duration, process_connect_request(channel));
                --nfds;
                ++pending_connects;
                break;
            case channel_state::CONNECTED:
                if (channel->send_queue_.empty() && channel->offset_ <= 0) --nfds;
                break;
            case channel_state::CONNECTING:
                wait_duration = (std::min)(wait_duration, get_connect_wait_duration(channel));
                --nfds;
                ++pending_connects;
                break;
            default:--nfds;
            }
        }

        auto wait_duration_usec = wait_duration.count();
        if (nfds <= 0 || wait_duration.count() > 0) {
            auto wait_duration = get_wait_duration(pending_connects == 0 ? MAX_WAIT_DURATION : wait_duration_usec);
            if (wait_duration > 0) {
                timeout.tv_sec = static_cast<long>(wait_duration / 1000000);
                timeout.tv_usec = static_cast<long>(wait_duration % 1000000);
                ::memcpy(&fds_array, this->fds_array_, sizeof(this->fds_array_));
                nfds = do_select(fds_array, timeout);
            }
            else {
                nfds = 2;
            }
        }
        if (nfds == -1)
        {
            int ec = xxsocket::get_last_errno();
            INET_LOG("socket.select failed, error code: %d, error msg:%s\n", ec, xxsocket::get_error_msg(ec));
            if (ec == EBADF) {
                goto _L_error;
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
#if 0
        // we should check whether the connection have exception before any operations.
        if (FD_ISSET(this->impl_.native_handle(), &(fds_array[except_op])))
        {
            int ec = xxsocket::get_last_errno();
            INET_LOG("socket.select exception triggered, error code: %d, error msg:%s\n", ec, xxsocket::get_error_msg(ec));
            // goto _L_error; // TODO: handle_error for impl_
        }
#endif
        for (auto channel : channels_) {
            // do connect
            if (channel->state_ == channel_state::CONNECTING) {
                do_connect(fds_array, channel);
            }

            if (nfds > 0 || channel->offset_ > 0) {
#if INET_ENABLE_VERBOSE_LOG
                INET_LOG("perform read operation...");
#endif
                if (!do_read(channel)) {
                    // INET_LOG("do read failed...");
                    handle_error(channel); // goto _L_error; // TODO: handle_error for impl_
                }
            }

            // perform write operations
            if (!channel->send_queue_.empty()) {
                channel->send_queue_mtx_.lock();
#if INET_ENABLE_VERBOSE_LOG
                INET_LOG("perform write operation...");
#endif
                if (!do_write(channel))
                { // TODO: check would block? for client, may be unnecessary.
                    channel->send_queue_mtx_.unlock();
                    handle_error(channel); // goto _L_error; // TODO: handle_error for impl_
                }

                channel->send_queue_mtx_.unlock();
            }
        }
            
        perform_timeout_timers();
    }

_L_error:
     unregister_descriptor(interrupter_.read_descriptor(), socket_event_read);

#if defined(_WIN32) && !defined(WINRT)
    timeEndPeriod(1);
#endif
}

std::chrono::microseconds async_tcp_client::get_connect_wait_duration(channel_context* channel)
{
    if (channel->state_ == channel_state::CONNECTING) {
        return std::chrono::duration_cast<std::chrono::microseconds>(channel->connect_expire_time_ - std::chrono::steady_clock::now());
    }

    return std::chrono::microseconds(0);
}

std::chrono::microseconds async_tcp_client::process_connect_request(channel_context* ctx)
{
    assert(ctx->state_ == channel_state::REQUEST_CONNECT);
    if (ctx->state_ == channel_state::REQUEST_CONNECT)
    {
        auto timestamp = time(NULL);
        INET_LOG("[%lld]connecting server %s:%u...", timestamp, ctx->address_.c_str(), ctx->port_);

        int ret = -1;
        ctx->state_ = channel_state::CONNECTING;
        int flags = xxsocket::getipsv();
        if (flags & ipsv_ipv4) {
            ret = ctx->impl_.pconnect_n(ctx->address_.c_str(), ctx->port_);
        }
        else if (flags & ipsv_ipv6)
        { // client is IPV6_Only
            INET_LOG("Client needs a ipv6 server address to connect!");
            ret = ctx->impl_.pconnect_n(ctx->addressv6_.c_str(), ctx->port_);
        }

        if (ret < 0)
        { // connect succeed, reset fds
            ctx->connect_expire_time_ = std::chrono::steady_clock::now() + std::chrono::microseconds(this->connect_timeout_);
            ctx->error = xxsocket::get_last_errno();
            if (ctx->error != EINPROGRESS && ctx->error != EWOULDBLOCK) {
                ctx->state_ = channel_state::IDLE;
                return std::chrono::microseconds(0);
            }
            register_descriptor(ctx->impl_.native_handle(), socket_event_read | socket_event_write);

            ctx->impl_.set_nonblocking(true);

            ctx->expected_pdu_length_ = -1;
            error_number_ = error_number::ERR_OK;
            ctx->offset_ = 0;
            ctx->receiving_pdu_.clear();

            ctx->impl_.set_optval(SOL_SOCKET, SO_REUSEADDR, 1); // set opt for p2p

            return get_connect_wait_duration(ctx);
        }
        else if (ret == 0) { // connect server succed immidiately.
            ctx->state_ = channel_state::CONNECTED;
            ctx->connect_expire_time_ = std::chrono::steady_clock::now();
        }
        else
            ctx->connect_expire_time_ = std::chrono::steady_clock::now();
    }

    return std::chrono::microseconds(0);
}

void async_tcp_client::close(size_t channel_index)
{
    // Gets channel
    if (channel_index >= channels_.size())
        return;
    auto channel = channels_[channel_index];

    if (channel->impl_.is_open()) {
        channel->impl_.shutdown();
        interrupter_.interrupt();
    }
}

void async_tcp_client::async_connect(size_t channel_index)
{
    // Gets channel
    if (channel_index >= channels_.size())
        return;
    auto channel = channels_[channel_index];

    channel->state_ = channel_state::REQUEST_CONNECT;

    interrupter_.interrupt();
}

void async_tcp_client::handle_error(channel_context* channel)
{
    if (channel->impl_.is_open()) {
        unregister_descriptor(channel->impl_.native_handle(), socket_event_read | socket_event_write);
        channel->impl_.close();
    }
    else {
        INET_LOG("local close the connection!");
    }

    channel->state_ = channel_state::IDLE;

    // @Notify connection lost
    if (this->on_connection_lost_) {
        int ec = error_number_;
        TSF_CALL(on_connection_lost_(channel->index_, ec, xxsocket::get_error_msg(ec)));
    }

    // @Clear all sending messages
    
    if (!channel->send_queue_.empty()) {
        channel->send_queue_mtx_.lock();

        for (auto pdu : channel->send_queue_)
            delete pdu;
        channel->send_queue_.clear();

        channel->send_queue_mtx_.unlock();
    }

    // @Notify all timers are cancelled.
    if (!this->timer_queue_.empty()) {
        this->timer_queue_mtx_.lock();
        //for (auto& timer : timer_queue_)
        //    timer->callback_(true);
        this->timer_queue_.clear();
        this->timer_queue_mtx_.unlock();
    }

    interrupter_.reset();
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
    auto channel = channels_[channel_index];

    if (channel->impl_.is_open())
    {
        auto pdu = new appl_pdu(std::move(data), callback, std::chrono::seconds(this->send_timeout_));

        channel->send_queue_mtx_.lock();
        channel->send_queue_.push_back(pdu);
        channel->send_queue_mtx_.unlock();

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

void async_tcp_client::do_connect(fd_set* fds_array, channel_context* ctx)
{
    if (ctx->state_ == channel_state::CONNECTING)
    {
        int error = -1;
        if (FD_ISSET(ctx->impl_.native_handle(), &fds_array[write_op])
            || FD_ISSET(ctx->impl_.native_handle(), &fds_array[read_op])) {
            socklen_t len = sizeof(error);
            if (::getsockopt(ctx->impl_.native_handle(), SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
                error = (-1);  /* Solaris pending error */
        }

        if (error == 0) {
            ctx->state_ = channel_state::CONNECTED;
            unregister_descriptor(ctx->impl_.native_handle(), socket_event_write); // remove write event avoid high-CPU occupation

            auto timestamp = time(NULL);
            INET_LOG("[%lld]connect server %s:%u succeed.", timestamp, ctx->address_.c_str(), ctx->port_);
            
            // TODO: send connect succeed event
        }
        else {
            // Check whether expire
            auto wait_duration = get_connect_wait_duration(ctx);
            if (wait_duration.count() <= 0) { // connect failed
                unregister_descriptor(ctx->impl_.native_handle(), socket_event_read | socket_event_write);
                ctx->impl_.close(); /* just in case */
                ctx->state_ = channel_state::IDLE;
                xxsocket::set_last_errno(error);

                auto timestamp = time(NULL);
                INET_LOG("[%lld]connect server %s:%u failed, error(%d)!", timestamp, ctx->address_.c_str(), ctx->port_, error);

                // TODO: send connect failed event
            }
#if INET_ENABLE_VERBOSE_LOG
            else {
                INET_LOG("continue waiting connection: [local] --> [%s:%u] to establish...", ctx->address_.c_str(), ctx->port_);
            }
#endif
        }
    }
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
                    auto timestamp = static_cast<long long>(time(NULL));
                    INET_LOG("=======> [%lld]send not complete %d bytes remained, %dbytes was sent!", timestamp, temp_left, n);
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
                error_number_ = xxsocket::get_last_errno();
                if (SHOULD_CLOSE_1(n, error_number_)) {
                    ctx->send_queue_.pop_front();

                    int ec = error_number_;
                    std::string errormsg = xxsocket::get_error_msg(ec);

                    auto timestamp = static_cast<long long>(time(NULL));
                    INET_LOG("[%lld]async_tcp_client::do_write failed, the connection should be closed, retval=%d, socket error:%d, detail:%s", timestamp, n, ec, errormsg.c_str());

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
                    error_number_ = error_number::ERR_DPL_ILLEGAL_PDU;
                    auto timestamp = static_cast<long long>(time(NULL));
                    INET_LOG("[%lld]async_tcp_client::do_read error, decode length of pdu failed!", timestamp);
                    break;
                }
            }
            else { // recv remain pdu data
                auto bytes_transferred = n + ctx->offset_;// bytes transferred at this time
                if ((ctx->receiving_pdu_.size() + bytes_transferred) > MAX_PDU_LEN) // TODO: config MAX_PDU_LEN, now is 16384
                {
                    error_number_ = error_number::ERR_PDU_TOO_LONG;
                    auto timestamp = static_cast<long long>(time(NULL));
                    INET_LOG("[%lld]async_tcp_client::do_read error, The length of pdu too long!", timestamp);
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
            error_number_ = xxsocket::get_last_errno();
            if (SHOULD_CLOSE_0(n, error_number_)) {
                int ec = error_number_;
                std::string errormsg = xxsocket::get_error_msg(ec);
                auto timestamp = static_cast<long long>(time(NULL));
                if (n == 0) {
                    INET_LOG("[%lld]async_tcp_client::do_read error, the server close the connection, retval=%d, socket error:%d, detail:%s", timestamp, n, ec, errormsg.c_str());
                }
                else {
                    INET_LOG("[%lld]async_tcp_client::do_read error, the connection should be closed, retval=%d, socket error:%d, detail:%s", timestamp, n, ec, errormsg.c_str());
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
        timer->callback_(true);
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
            auto tmpcallback = earliest->callback_;
            TSF_CALL(tmpcallback(false));
//            TSF_CALL(earliest->callback_(false));
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
    @Optimize, set default nfds is 2, make sure do_read & do_write event chould be perform when no need to call socket.select
    However, the connection exception will detected through do_read or do_write, but it's ok.
    */
#if INET_ENABLE_VERBOSE_LOG
    INET_LOG("socket.select maxfdp:%d waiting... %ld milliseconds", maxfdp_, tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
    int nfds = ::select(this->maxfdp_, &(fds_array[read_op]), &(fds_array[write_op]), nullptr, &tv);
#if INET_ENABLE_VERBOSE_LOG
    INET_LOG("socket.select waked up, retval=%d", nfds);
#endif

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

} /* namespace purelib::net */
} /* namespace purelib */
