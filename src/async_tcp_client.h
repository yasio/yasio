//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
// version: 2.3
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2016 halx99

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

#ifndef _ASYNC_TCP_CLIENT_H_
#define _ASYNC_TCP_CLIENT_H_
#include "xxsocket.h"
#include "endian_portable.h"
#include "singleton.h"
#include <vector>
#include <thread>
#include <mutex>
#include <ctime>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <set>
#include <algorithm>
#include "object_pool.h"
#include "select_interrupter.hpp"
#include "deadline_timer.h"

namespace purelib {

    namespace inet {
        enum {
            TCP_P2P_UNKNOWN = -1,
            TCP_P2P_CLIENT = 1, // local --> peer
            TCP_P2P_SERVER, // peer --> local
        };

        enum error_number {
            ERR_OK, // NO ERROR
            ERR_CONNECT_FAILED, // connect failed
            ERR_CONNECT_TIMEOUT, // connect timeout
            ERR_SEND_FAILED, // send error, failed
            ERR_SEND_TIMEOUT, // send timeout
            ERR_RECV_FAILED, // recv failed
            ERR_NETWORK_UNREACHABLE, // wifi or 2,3,4G not open
            ERR_CONNECTION_LOST, // connection lost
            ERR_PDU_TOO_LONG, // pdu too long
            ERR_DPL_ILLEGAL_PDU, // decode pdu error.
        };

        enum {
            socket_event_read = 1,
            socket_event_write = 2,
            socket_event_except = 4,
        };

        typedef std::function<void()> vdcallback_t;

        class appl_pdu; // application layer protocol data unit.

        typedef std::function<void(error_number)> appl_pdu_send_callback_t;
        typedef std::function<void(std::vector<char>&&)> appl_pdu_recv_callback_t;

        struct p2p_io_ctx
        {
            xxsocket                   impl_;
            bool                       connected_;
            int                        channel_type_ = TCP_P2P_UNKNOWN;
            char                       buffer_[65536]; // recv buffer
            int                        offset_ = 0; // recv buffer offset

            std::vector<char>          receiving_pdu_;
            int                        expected_pdu_length_ = -1;
            int                        error = 0;

            std::recursive_mutex       send_queue_mtx_;
            std::deque<appl_pdu*>      send_queue_;

            void                       reset();
        };

        class deadline_timer;

        // A tcp client support P2P
        class async_tcp_client : public p2p_io_ctx
        {
            friend struct p2p_io_ctx;
        public:

            // End user pdu decode length func
            typedef bool(*decode_pdu_length_func)(const char* data, size_t datalen, int& len);

            // connection_lost_callback_t
            typedef std::function<void(int error, const char* errormsg)> connection_lost_callback_t;

            typedef std::function<void(bool succeed, int ec)> connect_listener;

        public:
            async_tcp_client();
            ~async_tcp_client();

            size_t     get_received_pdu_count(void) const;

            // must be call on main thread(such cocos2d-x opengl thread)
            void       dispatch_received_pdu(int count = 1);

            // set endpoint of server.
            void       set_endpoint(const char* address, const char* addressv6, u_short port);

            // set callbacks, required API, must call by user
            /*
              threadsafe_call: for cocos2d-x should be:
              [](const vdcallback_t& callback) {
                  cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]{
                      callback();
                  });
              }
            */
            void       set_callbacks(
                decode_pdu_length_func decode_length_func,
                const connection_lost_callback_t& on_connection_lost,
                const appl_pdu_recv_callback_t& callback,
                const std::function<void(const vdcallback_t&)>& threadsafe_call);

            void       set_connect_listener(const connect_listener& listener);

            // set connect and send timeouts.
            void       set_timeouts(long timeo_connect, long timeo_send);

            // start async socket service
            void       start_service(int working_counter = 1);

            void       set_connect_wait_timeout(long seconds = -1/*-1: disable auto connect */);

            // notify tcp_client to connect server
            void       notify_connect();

            // close tcp_client
            void       close();

            // Whether the client-->server connection  established.
            bool       is_connected(void) const { return this->connected_; }

            // Gets network error code
            error_number  get_errorno(void) { return error_; }

            // post a async send request.
            void       async_send(std::vector<char>&& data, const appl_pdu_send_callback_t& callback = nullptr);


            // ***********  p2p support *****************
            // open p2p by local endpoint of "server --> local connection"
            void       p2p_open();

            // peer connect request detected by 'select', peer --> local connection established.
            bool       p2p_do_accept(void);

            // try to connect peer. local --> peer connection
            void       p2p_async_connect(const ip::endpoint& ep);

            // shutdown p2p, TODO: implement
            void       p2p_shutdown(void);

            // timer support
            void       schedule_timer(deadline_timer*);
            void       cancel_timer(deadline_timer*);

        private:
            void       perform_timeout_timers(); // ALL timer expired

            void       get_wait_duration(timeval& tv, long long usec);

            void       register_descriptor(const socket_native_type fd, int flags);
            void       unregister_descriptor(const socket_native_type fd, int flags);

            void       wait_connect_notify();

            bool       connect(void);

            void       service(void);

            bool       do_write(p2p_io_ctx*);
            bool       do_read(p2p_io_ctx*);
            void       move_received_pdu(p2p_io_ctx*); // move received properly pdu to recv queue

            void       handle_error(void); // TODO: add error_number parameter

        private:
            bool                    app_exiting_;
            bool                    thread_started_;
            std::thread             worker_thread_;

            std::string             address_;
            std::string             addressv6_;
            u_short                 port_;

            error_number               error_;

            int                     socket_error_;

            int                     connect_timeout_;
            int                     send_timeout_;

            bool                    connect_failed_;

            std::mutex              recv_queue_mtx_;
            std::deque<std::vector<char>> recv_queue_; // the recev_queue_ for connections: local-->server, local-->peer, peer-->local 

            std::mutex              connect_notify_mtx_;
            std::condition_variable connect_notify_cv_;
            long                    connect_wait_timeout_;

            // select interrupter
            select_interrupter      interrupter_;

            // timer support
            std::vector<deadline_timer*> timer_queue_;
            std::recursive_mutex         timer_queue_mtx_;

            // socket event set
            int maxfdp_;
            enum {
                read_op,
                write_op,
                except_op,
            };
            fd_set fds_array_[3];
            // fd_set readfds_, writefds_, excepfds_;
            connect_listener        connect_listener_;
            appl_pdu_recv_callback_t on_received_pdu_;
            decode_pdu_length_func  decode_pdu_length_;
            connection_lost_callback_t  on_connection_lost_;
            std::function<void(const vdcallback_t&)> call_tsf_;

            bool                    idle_;

            bool                    suspended_ = false;
            long long               total_connect_times_ = 0;

            // p2p support
            xxsocket                p2p_acceptor_; // p2p: the acceptor for connections: peer-->local
            ip::endpoint            p2p_peer_endpoint_; // p2p: this should tell by server
            p2p_io_ctx              p2p_channel1_; // p2p: connection: local ---> peer
            p2p_io_ctx              p2p_channel2_; // p2p: connection: peer ---> local 
        }; // async_tcp_client
    }; /* namspace purelib::net */
}; /* namespace purelib */
#endif

#define tcpcli purelib::gc::singleton<purelib::inet::async_tcp_client>::instance()
