#include "oslib.h"
#include "xxtcp_client.h"


#ifdef _WIN32
#define msleep(msec) Sleep((msec))
#define sleep(sec) msleep((sec) * 1000)
#if !defined(WINRT)
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")
#endif
#else
#define msleep(msec) usleep((msec) * 1000)
#endif

#define RECONNECT_DELAY 5000 // 5000 millseconds
#define TIMER_DURATION 100 // 100 miliseconds

#define MAX_pdu_LEN 16384

namespace purelib {
    namespace net {
        class xxappl_pdu
        {
        public:
            xxappl_pdu(std::vector<char>&& right, const xxappl_pdu_sent_callback_t& callback)
            {
                data_ = std::move(right);
                offset_ = 0;
                on_sent_ = callback;
                timestamp_ = ::millitime();
            }
            std::vector<char>          data_; // sending data
            size_t                     offset_; // offset
            xxappl_pdu_sent_callback_t on_sent_;
            long long                  timestamp_; // In milliseconds
        };

        void xxp2p_io_ctx::reset()
        {
            connected_ = false;
            receiving_pdu_.clear();
            offset_ = 0;
            expected_pdu_length_ = -1;
            error = 0;
            std::queue<xxappl_pdu*> empty_queue;
            send_queue_.swap(empty_queue);

            if (this->impl_.is_open()) {
                this->impl_.close();
            }
        }

        xxtcp_client::xxtcp_client() : app_exiting_(false),
            thread_started_(false),
            address_("192.168.1.104"),
            port_(8001),
            connect_timeout_(3),
            send_timeout_(3),
            decode_pdu_length_(nullptr),
            error_(ErrorCode::ERR_OK),
            idle_(true),
            connect_failed_(false)
        {
            FD_ZERO(&readfds_);
            FD_ZERO(&writefds_);
            FD_ZERO(&excepfds_);

            reset();
        }

        xxtcp_client::~xxtcp_client()
        {
            app_exiting_ = true;

            // shutdown_p2p_chancel();
            this->p2p_acceptor_.close();

            close();

            this->connect_notify_cv_.notify_all();
            while (working_counter_ > 0) { // wait all workers exited
                msleep(1);
            }
        }

        void xxtcp_client::set_endpoint(const char* address, const char* addressv6, u_short port)
        {
            this->address_ = address;
            this->addressv6_ = address;
            this->port_ = port;
        }

        void xxtcp_client::start_service(int working_counter)
        {
            if (!thread_started_) {
                thread_started_ = true;
                //  this->scheduleCollectResponse();    //
                this->working_counter_ = working_counter;
                for (auto i = 0; i < working_counter; ++i) {
                    std::thread t([this] {
                        // INETLOG("xxtcp_client thread running...");
                        this->service();
                        --working_counter_;
                        //cocos2d::log("p2s thread exited.");
                    });
                    t.detach();
                }
            }
        }


        void xxtcp_client::wait_connect_notify()
        {
            std::unique_lock<std::mutex> autolock(connect_notify_mtx_);
            connect_notify_cv_.wait(autolock); // wait 1 minutes, if no notify, wakeup self.
        }

        void xxtcp_client::service()
        {
#if defined(_WIN32) && !defined(WINRT)
            timeBeginPeriod(1);
#endif

            if (this->suspended_)
                return;

            while (!app_exiting_) 
            {
                bool connection_ok = impl_.is_open();

                if (!connection_ok)
                { // if no connect, wait connect notify.
                    if (total_connect_times_ >= 0)
                        wait_connect_notify();
                    ++total_connect_times_;
                    connection_ok = this->connect();
                    if (!connection_ok) { /// connect failed, waiting connect notify
                        // reconnClock = clock(); never use clock api on android platform
                        continue;
                    }
                }


                // event loop
                fd_set read_set, write_set, excep_set;
                timeval timeout;

                for (; !app_exiting_;)
                {
                    idle_ = true;

                    timeout.tv_sec = 0; // 5 minutes
                    timeout.tv_usec = 10 * 1000;     // 10 milliseconds

                    memcpy(&read_set, &readfds_, sizeof(fd_set));
                    memcpy(&write_set, &writefds_, sizeof(fd_set));
                    memcpy(&excep_set, &excepfds_, sizeof(fd_set));

                    int nfds = ::select(0, &read_set, &write_set, &excep_set, &timeout);

                    if (nfds == -1)
                    {
                        // log("select failed, error code: %d\n", GetLastError());
                        msleep(TIMER_DURATION);
                        continue;            // select again
                    }

                    if (nfds == 0)
                    {
                        continue;
                    }

                    /*
                    ** check and handle readable sockets
                    */
                    for (auto i = 0; i < excepfds_.fd_count; ++i) {
                        if (excepfds_.fd_array[i] == this->impl_)
                        {
                            goto _L_error;
                        }
                    }

                    /*
                    ** check and handle readable sockets
                    */
                    for (auto i = 0; i < read_set.fd_count; ++i) {
                        const auto readfd = read_set.fd_array[i];
                        if (readfd == this->p2p_acceptor_)
                        { // process p2p connect request from peer
                            p2p_do_accept();
                        }
                        else if (readfd == this->impl_)
                        {
                            if (!do_read(this))
                                goto _L_error;
                            idle_ = false;
                        }
                        else if (readfd == this->p2p_channel2_.impl_) {
                            if (!do_read(&this->p2p_channel2_))
                            { // read failed
                                this->p2p_channel2_.reset();
                            }
                        }
                        else if (readfd == this->p2p_channel1_.impl_) {
                            if (this->p2p_channel1_.connected_)
                            {
                                if (!do_read(&this->p2p_channel1_))
                                { // read failed
                                    this->p2p_channel2_.reset();
                                }
                            }
                            else {
                                printf("P2P: The connection local --> peer established. \n");
                                this->p2p_channel1_.connected_ = true;
                            }
                        }
                    }
                }

                // perform write operations
                if (!do_write(this))
                { // TODO: check would block? for client, may be unnecessory.
                    goto _L_error;
                }

                if (this->p2p_channel1_.connected_) {
                    if (!do_write(&p2p_channel1_))
                        p2p_channel1_.reset();
                }

                if (this->p2p_channel1_.connected_) {
                    if (!do_write(&p2p_channel2_))
                        p2p_channel2_.reset();
                }

                // Avoid high Occupation CPU
                if (idle_) {
                    msleep(1);
                }

                continue;

            _L_error:
                handle_error();
            }

#if defined(_WIN32) && !defined(WINRT)
            timeEndPeriod(1);
#endif
        }

        void xxtcp_client::close()
        {
            if (impl_.is_open())
                impl_.close();
        }

        void xxtcp_client::notify_connect()
        {
            std::unique_lock<std::mutex> autolock(connect_notify_mtx_);
            connect_notify_cv_.notify_one();
        }

        void xxtcp_client::handle_error(void)
        {
            socket_error_ = xxsocket::get_last_errno();
            const char* errs = xxsocket::get_error_msg(socket_error_);

            if (impl_.is_open()) {
                impl_.close();
            }
            else {
                // INETLOG("local close the connection!");
            }

            this->receiving_pdu_ = build_error_pdu_(socket_error_, errs);;
            move_received_pdu(this);
        }

        void xxtcp_client::register_descriptor(const socket_native_type fd, int flags)
        {
            if ((flags & impl_event_read) != 0)
            {
                FD_SET(fd, &this->readfds_);
            }

            if ((flags & impl_event_write) != 0)
            {
                FD_SET(fd, &this->writefds_);
            }

            if ((flags & impl_event_except) != 0)
            {
                FD_SET(fd, &this->excepfds_);
            }
        }

        void xxtcp_client::unregister_descriptor(const socket_native_type fd, int flags)
        {
            if ((flags & impl_event_read) != 0)
            {
                FD_CLR(fd, &this->readfds_);
            }

            if ((flags & impl_event_write) != 0)
            {
                FD_CLR(fd, &this->writefds_);
            }

            if ((flags & impl_event_except) != 0)
            {
                FD_CLR(fd, &this->excepfds_);
            }
        }

        void xxtcp_client::async_send(std::vector<char>&& data, const xxappl_pdu_sent_callback_t& callback)
        {
            if (this->impl_.is_open())
            {
                auto pdu = new xxappl_pdu(std::move(data), callback);

                std::unique_lock<std::mutex> autolock(send_queue_mtx_);
                send_queue_.push(pdu);
            }
            else {
                // cocos2d::log("xxtcp_client::send failed, The connection not ok!!!");
            }
        }

        void xxtcp_client::move_received_pdu(xxp2p_io_ctx* ctx)
        {
            //cocos2d::log("moveReceivedpdu...");

            std::unique_lock<std::mutex> autolock(recv_queue_mtx_);
            recv_queue_.push(std::move(ctx->receiving_pdu_));
            autolock.unlock();

            ctx->expected_pdu_length_ = -1;

            // CCSCHTASKS->resumeTarget(this);
        }


        bool xxtcp_client::connect(void)
        {
            if (connect_failed_)
            {
                // simpleHttpReqServerIP();
                connect_failed_ = false;
            }

            //INETLOG("connecting server %s:%u...", address.c_str(), port);
            //impl.open();
            //auto ep = xxsocket::resolve(this->address.c_str(), this->port);

            int ret = -1;

            int flags = xxsocket::getipsv();
            if (flags & ipsv_ipv4) {
                ret = impl_.pconnect_n(this->address_.c_str(), this->port_, this->connect_timeout_);
            }
            else if (flags & ipsv_ipv6)
            { // client is IPV6_Only
                //cocos2d::log("Client needs a ipv6 server address to connect!");
                ret = impl_.pconnect_n(this->addressv6_.c_str(), this->port_, this->connect_timeout_);
            }
            if (ret == 0)
            { // connect succeed, reset fds
                FD_ZERO(&readfds_);
                FD_ZERO(&writefds_);
                FD_ZERO(&excepfds_);

                auto fd = impl_.native_handle();
                register_descriptor(fd, impl_event_read | impl_event_except);

                std::unique_lock<std::mutex> autolock(send_queue_mtx_);
                std::queue<xxappl_pdu*> emptypduQueue;
                this->send_queue_.swap(emptypduQueue);

                impl_.set_nonblocking(true);

                expected_pdu_length_ = -1;
                error_ = ErrorCode::ERR_OK;
                offset_ = 0;
                receiving_pdu_.clear();

                this->impl_.set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

                // INETLOG("connect server: %s:%u succeed.", address.c_str(), port);

#if 0 // provided as API
                auto endp = this->impl_.local_endpoint();
                // INETLOG("local endpoint: %s", endp.to_string().c_str());

                if (p2p_acceptor_.reopen())
                { // start p2p listener
                    if (p2p_acceptor_.bind(endp) == 0)
                    {
                        if (p2p_acceptor_.listen() == 0)
                        {
                            // set socket to async
                            p2p_acceptor_.set_nonblocking(true);

                            // register p2p's peer connect event
                            register_descriptor(p2p_acceptor_, impl_event_read);
                            // INETLOG("listening at local %s successfully.", endp.to_string().c_str());
                        }
                    }
                }
#endif

                if (this->connect_listener_ != nullptr) {
                    this->connect_listener_(true, 0);
                }

                return true;
            }
            else { // connect server failed.
                connect_failed_ = true;
                int ec = xxsocket::get_last_errno();
                if (this->connect_listener_ != nullptr) {
                    /* CCRUNONGL([this, ec] {
                         connectListener(false, ec);
                     });*/
                }

                // INETLOG("connect server: %s:%u failed, error code:%d, error msg:%s!", address.c_str(), port, ec, xxsocket::get_error_msg(ec));

                return false;
            }
        }

        bool xxtcp_client::do_write(xxp2p_io_ctx* ctx)
        {
            bool bRet = false;

            do {
                int n;

                if (!ctx->impl_.is_open())
                    break;

                std::unique_lock<std::mutex> autolock(ctx->send_queue_mtx_);
                if (!ctx->send_queue_.empty()) {
                    auto& v = ctx->send_queue_.front();
                    auto bytes_left = v->data_.size() - v->offset_;
                    n = ctx->impl_.send_i(v->data_.data() + v->offset_, bytes_left);
                    if (n == bytes_left) { // All pdu bytes sent.
                        ctx->send_queue_.pop();
                        /*CCRUNONGL([v] {
                            if (v->onSend != nullptr)
                                v->onSend(ErrorCode::ERR_OK);
                            v->release();
                        });*/
                    }
                    else if (n > 0) { // TODO: add time
                        if ((millitime() - v->timestamp_) < (send_timeout_ * 1000))
                        { // change offset, remain data will send next time.
                            // v->data_.erase(v->data_.begin(), v->data_.begin() + n);
                            v->offset_ += n;
                        }
                        else { // send timeout
                            ctx->send_queue_.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_SEND_TIMEOUT);
                                v->release();
                            });*/
                        }
                    }
                    else { // n <= 0, TODO: add time
                        socket_error_ = xxsocket::get_last_errno();
                        if (SHOULD_CLOSE_1(n, socket_error_)) {
                            ctx->send_queue_.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_CONNECTION_LOST);
                                v->release();
                            });*/

                            break;
                        }
                    }
                    idle_ = false;
                }
                autolock.unlock();

                bRet = true;
            } while (false);

            return bRet;
        }

        bool xxtcp_client::do_read(xxp2p_io_ctx* ctx)
        {
            bool bRet = false;
            do {
                if (!ctx->impl_.is_open())
                    break;

                int n = ctx->impl_.recv_i(ctx->buffer_ + ctx->offset_, sizeof(buffer_) - ctx->offset_);

                if (n > 0 || (n == -1 && ctx->offset_ != 0)) {

                    if (n == -1)
                        n = 0;

                    // INETLOG("xxtcp_client::doRead --- received data len: %d, buffer data len: %d", n, n + offset);

                    if (ctx->expected_pdu_length_ == -1) { // decode length
                        if (decode_pdu_length_(ctx->buffer_, ctx->offset_ + n, ctx->expected_pdu_length_))
                        {
                            if (ctx->expected_pdu_length_ < 0) { // header insuff
                                ctx->offset_ += n;
                            }
                            else { // ok
                                auto bytes_transferred = n + ctx->offset_;
                                ctx->receiving_pdu_.insert(ctx->receiving_pdu_.end(), ctx->buffer_, ctx->buffer_ + (std::min)(ctx->expected_pdu_length_, bytes_transferred));
                                if (bytes_transferred >= expected_pdu_length_)
                                { // pdu received properly
                                    ctx->offset_ = bytes_transferred - ctx->expected_pdu_length_; // set offset to bytes of remain buffer
                                    if (offset_ > 0)  // move remain data to head of buffer and hold offset.
                                        ::memmove(ctx->buffer_, ctx->buffer_ + ctx->expected_pdu_length_, offset_);

                                    // move properly pdu to ready queue, GL thread will retrieve it.
                                    move_received_pdu(ctx);
                                }
                                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                                    ctx->offset_ = 0;
                                }
                            }
                        }
                        else {
                            error_ = ErrorCode::ERR_DPL_ILLEGAL_pdu;
                            break;
                        }
                    }
                    else { // recv remain pdu data
                        auto bytes_transferred = n + ctx->offset_;// bytes transferred at this time
                        if ((receiving_pdu_.size() + bytes_transferred) > MAX_pdu_LEN) // TODO: config MAX_pdu_LEN, now is 16384
                        {
                            error_ = ErrorCode::ERR_PDU_TOO_LONG;
                            break;
                        }
                        else {
                            auto bytes_needed = ctx->expected_pdu_length_ - static_cast<int>(ctx->receiving_pdu_.size()); // never equal zero or less than zero
                            if (bytes_needed > 0) {
                                ctx->receiving_pdu_.insert(ctx->receiving_pdu_.end(), ctx->buffer_, ctx->buffer_ + (std::min)(bytes_transferred, bytes_needed));

                                ctx->offset_ = bytes_transferred - bytes_needed; // set offset to bytes of remain buffer
                                if (ctx->offset_ >= 0)
                                { // pdu received properly
                                    if (offset_ > 0)  // move remain data to head of buffer and hold offset.
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

                    this->idle_ = false;
                }
                else {
                    socket_error_ = xxsocket::get_last_errno();
                    if (SHOULD_CLOSE_0(n, socket_error_)) {
                        if (n == 0) {
                            //INETLOG("server close the connection!");
                        }
                        break;
                    }
                }

                bRet = true;

            } while (false);

            return bRet;
        }

        void xxtcp_client::p2p_open()
        {
            if (is_connected()) {
                if (this->p2p_acceptor_.reopen())
                {
                    this->p2p_acceptor_.bind(this->impl_.local_endpoint());
                    this->p2p_acceptor_.listen(1); // We just listen one connection for p2p
                    register_descriptor(this->p2p_acceptor_, impl_event_read);
                }
            }
        }

        bool xxtcp_client::p2p_do_accept(void)
        {
            if (this->p2p_channel2_.impl_.is_open())
            { // Just ignore other connect request for 1 <<-->> 1 connections.
                this->p2p_acceptor_.accept().close();
                return true;
            }

            this->idle_ = false;
            this->p2p_channel2_.impl_ = this->p2p_acceptor_.accept();
            return this->p2p_channel2_.impl_.is_open();
        }
#if 0
        void xxtcp_client::dispatchResponseCallbacks(float delta)
        {
            std::unique_lock<std::mutex> autolock(this->recvQueueMtx);
            if (!this->recvQueue.empty()) {
                auto pdu = std::move(this->recvQueue.front());
                this->recvQueue.pop();

                if (this->recvQueue.empty()) {
                    autolock.unlock();
                    //CCSCHTASKS->pauseTarget(this);
                }
                else
                    autolock.unlock();

                if (onRecvpdu != nullptr)
                    this->onRecvpdu(std::move(pdu));
            }
            else {
                autolock.unlock();
                //CCSCHTASKS->pauseTarget(this);
            }
        }
#endif
    } /* namespace purelib::net */
} /* namespace purelib */
