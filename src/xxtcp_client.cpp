#include "xxtcp_client.h"
#include "oslib.h"

#define HAS_SELECT_OP 0

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
                sending_data_ = std::move(right);
                send_offset_ = 0;
                on_sent_ = callback;
                timestamp_ = ::millitime();
            }
            std::vector<char>          sending_data_;
            xxappl_pdu_sent_callback_t on_sent_;
            size_t                     send_offset_;
            long long                  timestamp_; // In milliseconds
        };

        xxtcp_client::xxtcp_client() : app_exiting_(false),
            thread_started_(false),
            address_("192.168.1.104"),
            port_(8001),
            connect_timeout_(3),
            send_timeout_(3),
            decode_pdu_length_(nullptr),
            offset_(0),
            error_(ErrorCode::ERR_OK),
            expected_pdu_length_(-1),
            idle_(true),
            connect_failed_(false),
            working_counter_(0)
        {
            channel1_.client_ = this;
            channel2_.client_ = this;
        }

        xxtcp_client::~xxtcp_client()
        {
            app_exiting_ = true;

            shutdown_p2p_chancel();
            this->p2p_acceptor_.close();

            close();

            this->connect_notify_cv_.notify_all();
            this->p2p_conn_notify_cv_.notify_all();
            while (working_counter_ > 0) {
                msleep(1);
            }
        }

        void xxtcp_client::set_endpoint(const char* address, const char* addressv6, u_short port)
        {
            this->address_ = address;
            this->addressv6_ = address;
            this->port_ = port;
        }

        void xxtcp_client::start_service()
        {
            if (!thread_started_) {
                thread_started_ = true;
               //  this->scheduleCollectResponse();    //

                working_counter_ = 1;

                std::thread t([this] {
                    // INETLOG("xxtcp_client thread running...");
                    this->service();
                    --working_counter_;
                    //cocos2d::log("p2s thread exited.");
                });

                t.detach();
            }
        }

        void xxtcp_client::start_p2p_service()
        {
            channel1_.channel_ = &p2p_channel1_;
            channel2_.channel_ = &p2p_channel2_;
            channel1_.channel_type_ = TCP_P2P_CLIENT;
            channel2_.channel_type_ = TCP_P2P_SERVER;

            std::thread p2p([this] {
                p2p_service();
                --working_counter_;
            });

            p2p.detach();
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

            while (!app_exiting_) {

                bool connection_ok = impl_.is_open();

                if (!connection_ok)
                { // if no connect, wait connect notify.
                    if (total_connect_times_ >= 0)
                        wait_connect_notify();
                    ++total_connect_times_;
                    connection_ok = this->connect();
                    if (!connection_ok) { /// connect failed, waiting connect notify
                        // reconnClock = clock(); never use clock api on android platform, it's a ¿Ó»õ
                        continue;
                    }
                }


                // event loop
#if HAS_SELECT_OP == 1
                fd_set read_set, write_set, excep_set;
                timeval timeout;
#endif

                for (; !app_exiting_;)
                {
                    idle_ = true;

#if HAS_SELECT_OP == 1
                    timeout.tv_sec = 5 * 60; // 5 minutes
                    timeout.tv_usec = 0;     // 10 milliseconds

                    memcpy(&read_set, &readfds, sizeof(fd_set));
                    memcpy(&write_set, &writefds, sizeof(fd_set));
                    memcpy(&excep_set, &excepfds, sizeof(fd_set));

                    int connfd = impl.native_handle();
                    int nfds = ::select(connfd + 1, &read_set, &write_set, &excep_set, &timeout);

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
#endif

#if HAS_SELECT_OP == 1
                    if (FD_ISSET(impl, &write_set))
                    { // do send
#endif
                        if (!do_write())
                            break;
#if HAS_SELECT_OP == 1
                    }
#endif

#if HAS_SELECT_OP == 1
                    if (FD_ISSET(impl, &read_set))
                    { // do read
#endif
                        if (!do_read())
                            break;

#if HAS_SELECT_OP == 1
                        idle = false;
                    }
#endif

#if HAS_SELECT_OP == 1
                    if (FD_ISSET(impl, &excep_set))
                    { // do close
                        handleError();
                        break; // end loop, try next connect
                    }
#endif

                    // Avoid high Occupation CPU
                    if (idle_) {
                        msleep(1);
                    }
                }

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

            // assert(this->recvingpdu.empty());

            this->receiving_pdu_ = build_error_pdu_(socket_error_, errs);;
            move_received_pdu();
        }

        /*void xxtcp_client::setBuildErrorpduFunc(BuildErrorpduFunc func)
        {
            buildErrorpdu = func;
        }

        void xxtcp_client::setDecodeLengthFunc(DecodepduLengthFunc func)
        {
            decodepduLength = func;
        }

        void xxtcp_client::setOnRecvListener(const OnpduRecvCallback& callback)
        {
            onRecvpdu = callback;
        }*/

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

        void xxtcp_client::move_received_pdu()
        {
            //cocos2d::log("moveReceivedpdu...");

            std::unique_lock<std::mutex> autolock(recv_queue_mtx_);
            recv_queue_.push(std::move(this->receiving_pdu_));
            autolock.unlock();

            expected_pdu_length_ = -1;

            // CCSCHTASKS->resumeTarget(this);
        }

        void xxtcp_client::p2p_move_received_pdu(xxp2p_io_ctx* ctx)
        {
            std::unique_lock<std::mutex> autolock(recv_queue_mtx_); // recvQueue is shared all channel
            recv_queue_.push(std::move(ctx->receiving_pdu_));
            autolock.unlock();

            ctx->expected_pdu_len_ = -1;

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
            //impl.open();
            //ret = impl.connect_n(this->address.c_str(), this->port, this->timeoutForConnect);
#if 1
            int flags = xxsocket::getipsv();
            if (flags & ipsv_ipv4) {
                ret = impl_.pconnect_n(this->address_.c_str(), this->port_, this->connect_timeout_);
            }
            else if (flags & ipsv_ipv6)
            { // client is IPV6_Only
                //cocos2d::log("Client needs a ipv6 server address to connect!");
                ret = impl_.pconnect_n(this->addressv6_.c_str(), this->port_, this->connect_timeout_);
            }
#endif
            if (ret == 0)
            {
#if HAS_SELECT_OP == 1
                FD_ZERO(&readfds);
                FD_ZERO(&writefds);
                FD_ZERO(&excepfds);

                auto fd = impl.native_handle();
                FD_SET(fd, &readfds);
                FD_SET(fd, &writefds);
                FD_SET(fd, &excepfds);
#endif

                std::unique_lock<std::mutex> autolock(send_queue_mtx_);
                std::queue<xxappl_pdu*> emptypduQueue;
                //this->send_queue_.swap(emptypduQueue);

                impl_.set_nonblocking(true);

                expected_pdu_length_ = -1;
                error_ = ErrorCode::ERR_OK;
                offset_ = 0;
                receiving_pdu_.clear();

                this->impl_.set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

                //INETLOG("connect server: %s:%u succeed.", address.c_str(), port);

                auto endp = this->impl_.local_endpoint();
                //INETLOG("local endpoint: %s", endp.to_string().c_str());
                /// start p2p listener
                if (p2p_acceptor_.reopen()) {
                    if (p2p_acceptor_.bind(endp) == 0)
                    {
                        if (p2p_acceptor_.listen() == 0)
                        {
                            // set socket to async
                            p2p_acceptor_.set_nonblocking(true);
                            // INETLOG("listening at local %s successfully.", endp.to_string().c_str());
                        }
                    }
                }

                if (this->connect_listener_ != nullptr) {
                   /* CCRUNONGL([this] {
                        connectListener(true, 0);
                    });*/
                }

                return true;
            }
            else {
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

        bool xxtcp_client::do_write(void)
        {
            bool bRet = false;

            do {
                int n;

                if (!this->impl.is_open())
                    break;

                std::unique_lock<std::mutex> autolock(this->sendQueueMtx);
                if (!sendQueue.empty()) {
                    auto& v = sendQueue.front();
                    n = impl.send_i(v->data.data(), v->data.size());
                    if (n == v->data.size()) {
                        sendQueue.pop();
                        /*CCRUNONGL([v] {
                            if (v->onSend != nullptr)
                                v->onSend(ErrorCode::ERR_OK);
                            v->release();
                        });*/
                    }
                    else if (n > 0) { // TODO: add time
                        if ((millitime() - v->timestamp) < (timeoutForSend * 1000))
                        { // erase sent data, remain data will send next time.
                            v->data.erase(v->data.begin(), v->data.begin() + n);
                        }
                        else { // send timeout
                            sendQueue.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_SEND_TIMEOUT);
                                v->release();
                            });*/
                        }
                    }
                    else { // n <= 0, TODO: add time
                        socket_error = xxsocket::get_last_errno();
                        if (SHOULD_CLOSE_1(n, socket_error)) {
                            sendQueue.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_CONNECTION_LOST);
                                v->release();
                            });*/

                            break;
                        }
                    }
                    idle = false;
                }
                autolock.unlock();

                bRet = true;
            } while (false);

            return bRet;
        }

        bool xxtcp_client::do_read(void)
        {
            bool bRet = false;
            do {
                if (!this->impl.is_open())
                    break;

                int n = impl.recv_i(this->buffer + offset, sizeof(buffer) - offset);

                if (n > 0 || (n == -1 && offset != 0)) {

                    if (n == -1)
                        n = 0;

                    // INETLOG("xxtcp_client::doRead --- received data len: %d, buffer data len: %d", n, n + offset);

                    if (expectedpduLength == -1) { // decode length
                        if (decodepduLength(this->buffer, offset + n, expectedpduLength))
                        {
                            if (expectedpduLength < 0) { // header insuff
                                offset += n;
                            }
                            else { // ok
                                auto bytes_transferred = n + offset;
                                this->recvingpdu.insert(recvingpdu.end(), buffer, buffer + (std::min)(expectedpduLength, bytes_transferred));
                                if (bytes_transferred >= expectedpduLength)
                                { // pdu received properly
                                    offset = bytes_transferred - expectedpduLength; // set offset to bytes of remain buffer
                                    if (offset > 0)  // move remain data to head of buffer and hold offset.
                                        ::memmove(this->buffer, this->buffer + expectedpduLength, offset);

                                    // move properly pdu to ready queue, GL thread will retrieve it.
                                    moveReceivedpdu();
                                }
                                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                                    offset = 0;
                                }
                            }
                        }
                        else {
                            error = ErrorCode::ERR_DPL_ILLEGAL_pdu;
                            break;
                        }
                    }
                    else { // recv remain pdu data
                        auto bytes_transferred = n + offset;// bytes transferred at this time
                        if ((recvingpdu.size() + bytes_transferred) > MAX_pdu_LEN) // TODO: config MAX_pdu_LEN, now is 16384
                        {
                            error = ErrorCode::ERR_pdu_TOO_LONG;
                            break;
                        }
                        else {
                            auto bytesNeeded = expectedpduLength - static_cast<int>(recvingpdu.size()); // never equal zero or less than zero
                            if (bytesNeeded > 0) {
                                this->recvingpdu.insert(recvingpdu.end(), buffer, buffer + (std::min)(bytes_transferred, bytesNeeded));

                                offset = bytes_transferred - bytesNeeded; // set offset to bytes of remain buffer
                                if (offset >= 0)
                                { // pdu received properly
                                    if (offset > 0)  // move remain data to head of buffer and hold offset.
                                        ::memmove(this->buffer, this->buffer + bytesNeeded, offset);

                                    // move properly pdu to ready queue, GL thread will retrieve it.
                                    moveReceivedpdu();
                                }
                                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                                    offset = 0;
                                }
                            }
                            else {
                                //assert(false);
                            }
                        }
                    }

                    idle = false;
                }
                else {
                    socket_error = xxsocket::get_last_errno();
                    if (SHOULD_CLOSE_0(n, socket_error)) {
                        if (n == 0) {
                            //INETLOG("server close the connection!");
                        }
                        break;
                    }
                }

                bRet = true;

            } while (false);

            if (bRet) {
                /// try to accept peer connect request.
                if (!p2pChannel2.is_open())
                {
                    if (p2pAcceptor.is_open())
                    {
                        p2pChannel2 = this->p2pAcceptor.accept();
                        if (p2pChannel2.is_open())
                        {
                           // INETLOG("p2p: peer connection income, peer endpoint:%s", p2pChannel2.peer_endpoint().to_string().c_str());
                        }
                    }
                }
            }

            return bRet;
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

        ///////////////////////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////// TCP P2P Supports  ///////////////////////////////////////////

        bool xxtcp_client::p2p_connect(void)
        {
           // INETLOG("p2p: connecting peer %s:%u...", p2pAvaiableEndpoint.to_string().c_str());
            p2pChannel1.open();
            int ret = p2pChannel1.connect_n(p2pAvaiableEndpoint, this->timeoutForConnect);
            if (ret == 0)
            {
                p2pChannel1.set_nonblocking(true);

                channel1.expectedpduLength = -1;
                channel1.error = ErrorCode::ERR_OK;
                channel1.offset = 0;
                channel1.recvingpdu.clear();

               // INETLOG("p2p: connect peer: %s succeed.", p2pAvaiableEndpoint.to_string().c_str());

                auto endp = p2pChannel1.local_endpoint();
               // INETLOG("p2p: local endpoint: %s", endp.to_string().c_str());

#if 0
                p2pChannel1.set_optval(SOL_SOCKET, SO_REUSEADDR, 1);

                /// start p2p listener
                if (p2pListener.reopen()) {
                    if (p2pListener.bind(endp) == 0)
                    {
                        if (p2pListener.listen() == 0)
                        {
                            // set socket to async
                            p2pListener.set_nonblocking(true);
                            INETLOG("p2p: listening at local %s successfully.", endp.to_string_full().c_str());
                        }
                    }
                }
                if (this->connectListener != nullptr) {
                    CCRUNONGL([this] {
                        connectListener(true, 0);
                    });
                }
#endif

                return true;
            }
            else {
                int ec = xxsocket::get_last_errno();

#if 0
                if (this->connectListener != nullptr) {
                    CCRUNONGL([this, ec] {
                        connectListener(false, ec);
                    });
                }
#endif

               // INETLOG("p2p: connect peer: %s failed, error code:%d, error msg:%s!", p2pAvaiableEndpoint.to_string().c_str(), ec, xxsocket::get_error_msg(ec));

                return false;
            }
        }

        void xxtcp_client::update_p2p_endpoint(const ip::endpoint& ep)
        {
            this->p2p_avialable_endpoint_ = ep;
            this->p2p_conn_notify_cv_.notify_one();
        }

        void xxtcp_client::p2p_wait_connect_notify()
        {
            std::unique_lock<std::mutex> autolock(p2p_conn_notify_mtx_);
            p2p_conn_notify_cv_.wait(autolock);
        }

        void xxtcp_client::shutdown_p2p_chancel(void)
        {
            this->p2p_channel1_.close();
            this->p2p_channel2_.close();
        }

        void xxtcp_client::p2p_handle_error(xxp2p_io_ctx* ctx)
        {
            ctx->channel_->close();

            if (ctx->channel_type_ == P2PChannelType::TCP_P2P_CLIENT)
            {
               // INETLOG("p2p: local-->peer connection lost!");
            }
            else {
              //  INETLOG("p2p: peer-->local connection lost!");
            }
        }

        void xxtcp_client::p2p_service(void)
        {
#if defined(_WIN32) && !defined(WINRT)
            timeBeginPeriod(1);
#endif
            while (!bAppExiting) {

                bool connection_ok = this->p2pChannel1.is_open();

                if (!connection_ok)
                { // if no connect, wait connect notify.
                    p2p_waitConnectNotify();

                    if (bAppExiting)
                        break;

                    connection_ok = this->p2p_connect();
                    if (!connection_ok) { /// connect failed, waiting connect notify
                        // reconnClock = clock(); never use clock api on android platform, it's a ¿Ó»õ
                        continue;
                    }
                }

                // p2p connection ok
                // start communication 
              //  INETLOG("p2p channels ok, start communication");

                std::vector<P2PIoContext*> channels = { &channel1, &channel2 };

                for (; !bAppExiting;)
                {
                    idle = true;

                    for (auto iter = channels.begin(); iter != channels.end();)
                    {
                        if (!(*iter)->doWrite())
                        {
                            iter = channels.erase(iter);
                            continue;
                        }

                        if (!(*iter)->doRead())
                        {
                            iter = channels.erase(iter);
                            continue;
                        }

                        ++iter;
                    }

                    if (channels.empty()) // no channel active
                        break;

                    // p2p always wait 1 millisecond, Avoid high Occupation CPU
                    msleep(1);
                }
            }

#if defined(_WIN32) && !defined(WINRT)
            timeEndPeriod(1);
#endif
        }

        bool xxp2p_io_ctx::do_write()
        {
            bool bRet = false;

            if (this->channel == nullptr)
                return false;

            auto& channel = *this->channel;

            do {
                int n;

                std::unique_lock<std::mutex> autolock(this->sendQueueMtx);
                if (!this->sendQueue.empty()) {
                    auto& v = this->sendQueue.front();
                    n = channel.send_i(v->data.data(), v->data.size());
                    if (n == v->data.size()) {
                        this->sendQueue.pop();
                       /* CCRUNONGL([v] {
                            if (v->onSend != nullptr)
                                v->onSend(ErrorCode::ERR_OK);
                            v->release();
                        });*/
                    }
                    else if (n > 0) { // TODO: add time
                        if ((millitime() - v->timestamp) < (this->client->timeoutForSend * 1000))
                        { // erase sent data, remain data will send next time.
                            v->data.erase(v->data.begin(), v->data.begin() + n);
                        }
                        else { // send timeout
                            this->sendQueue.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_SEND_TIMEOUT);
                                v->release();
                            });*/
                        }
                    }
                    else { // n <= 0, TODO: add time
                        int ec = xxsocket::get_last_errno();
                        if (SHOULD_CLOSE_1(n, ec)) {
                            this->sendQueue.pop();
                            /*CCRUNONGL([v] {
                                if (v->onSend)
                                    v->onSend(ErrorCode::ERR_CONNECTION_LOST);
                                v->release();
                            });*/

                            client->p2p_handleError(this); //  TODO: handle error
                            break;
                        }
                    }
                    // idle = false;
                }
                autolock.unlock();

                bRet = true;
            } while (false);

            return bRet;
        }

        bool xxp2p_io_ctx::do_read(void)
        {
            bool bRet = false;

            if (!this->channel->is_open())
                return false;

            auto& channel = *this->channel;

            do {
                int n = channel.recv_i(this->buffer + this->offset, sizeof(this->buffer) - this->offset);

                if (n > 0 || (n == -1 && this->offset != 0)) {

                    if (n == -1)
                        n = 0;

                    // INETLOG("p2p: xxtcp_client::doRead --- received data len: %d, buffer data len: %d", n, n + this->offset);

                    if (this->expectedpduLength == -1) { // decode length
                        if (client->decodepduLength(this->buffer, offset + n, this->expectedpduLength))
                        {
                            if (this->expectedpduLength < 0) { // header insuff
                                this->offset += n;
                            }
                            else { // ok
                                auto bytes_transferred = n + this->offset;
                                this->recvingpdu.insert(this->recvingpdu.end(), this->buffer, this->buffer + (std::min)(this->expectedpduLength, bytes_transferred));
                                if (bytes_transferred >= expectedpduLength)
                                { // pdu received properly
                                    offset = bytes_transferred - expectedpduLength; // set offset to bytes of remain buffer
                                    if (offset > 0)  // move remain data to head of buffer and hold offset.
                                        ::memmove(this->buffer, this->buffer + this->expectedpduLength, this->offset);

                                    // move properly pdu to ready queue, GL thread will retrieve it.
                                    client->p2p_moveReceivedpdu(this);
                                }
                                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                                    this->offset = 0;
                                }
                            }
                        }
                        else {
                            this->error = ErrorCode::ERR_DPL_ILLEGAL_pdu;
                            client->p2p_handleError(this);
                            break;
                        }
                    }
                    else { // recv remain pdu data
                        auto bytes_transferred = n + offset;// bytes transferred at this time
                        if ((recvingpdu.size() + bytes_transferred) > MAX_pdu_LEN) // TODO: config MAX_pdu_LEN, now is 16384
                        {
                            this->error = ErrorCode::ERR_pdu_TOO_LONG;
                            client->p2p_handleError(this);
                            break;
                        }
                        else {
                            auto bytesNeeded = expectedpduLength - static_cast<int>(recvingpdu.size()); // never equal zero or less than zero
                            if (bytesNeeded > 0) {
                                this->recvingpdu.insert(recvingpdu.end(), buffer, buffer + (std::min)(bytes_transferred, bytesNeeded));

                                offset = bytes_transferred - bytesNeeded; // set offset to bytes of remain buffer
                                if (offset >= 0)
                                { // pdu received properly
                                    if (offset > 0)  // move remain data to head of buffer and hold offset.
                                        ::memmove(this->buffer, this->buffer + bytesNeeded, offset);

                                    // move properly pdu to ready queue, GL thread will retrieve it.
                                    client->p2p_moveReceivedpdu(this);
                                }
                                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                                    offset = 0;
                                }
                            }
                            else {
                               // assert(false);
                            }
                        }
                    }

                    // idle = false;
                }
                else {
                    int ec = xxsocket::get_last_errno();
                    if (SHOULD_CLOSE_0(n, ec)) {
                        if (n == 0) {
                            // INETLOG("p2p: peer close the connection.");
                        }
                        client->p2p_handleError(this);
                        break;
                    }
                }

                bRet = true;

            } while (false);

            return bRet;
        }

    }
};

