#ifndef _XXTCP_CLIENT_H_
#define _XXTCP_CLIENT_H_
#include "xxsocket.h"
#include "endian_portable.h"
#include "singleton.h"
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <ctime>
#include <condition_variable>
#include <atomic>
#include <queue>

#define _USING_IN_COCOS2DX 0
#if _USING_IN_COCOS2DX
#include "cocos2d.h"
#define INET_LOG(format,...) cocos2d::Director::getInstance()->getScheduler()->performFunctionInCocosThread([=]{cocos2d::log((format), ##__VA_ARGS__);})
#else
#define INET_LOG(format,...) fprintf(stdout,(format "\n"),##__VA_ARGS__)
#endif

namespace purelib {

    namespace inet {
        enum P2PChannelType {
            TCP_P2P_UNKNOWN = -1,
            TCP_P2P_CLIENT = 1, // local --> peer
            TCP_P2P_SERVER, // peer --> local
        };

        enum ErrorCode {
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

        class xxappl_pdu; // application layer protocol data unit.

        typedef std::function<void(ErrorCode)> xxappl_pdu_send_callback_t;
        typedef std::function<void(std::vector<char>&&)> xxappl_pdu_recv_callback_t;

        struct xxp2p_io_ctx
        {
            xxsocket                   impl_;
            bool                       connected_;
            P2PChannelType             channel_type_ = TCP_P2P_UNKNOWN;
            char                       buffer_[65536]; // recv buffer
            int                        offset_ = 0; // recv buffer offset
            
            std::vector<char>          receiving_pdu_;
            int                        expected_pdu_length_ = -1;
            int                        error = 0;

            std::recursive_mutex       send_queue_mtx_;
            std::deque<xxappl_pdu*>    send_queue_;

            void                       reset();
        };

        // A tcp client support P2P
        class xxtcp_client : public xxp2p_io_ctx
        {
            friend struct xxp2p_io_ctx;
        public:

            // End user pdu decode length func
            typedef bool(*decode_pdu_length_func)(const char* data, size_t datalen, int& len);

            // BuildErrorInfoFunc
            typedef std::vector<char>(*build_error_func)(int errorcode, const char* errormsg);

            typedef std::function<void(bool succeed, int ec)> connect_listener;

        public:
            xxtcp_client();
            ~xxtcp_client();

            // must be call on main thread(such cocos2d-x opengl thread)
            bool       collect_received_pdu();

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
                build_error_func build_error_pdu_func,
                const xxappl_pdu_recv_callback_t& callback, 
                const std::function<void(const vdcallback_t&)>& threadsafe_call); 

            void       set_connect_listener(const connect_listener& listener);

            // set connect and send timeouts.
            void       set_timeouts(long timeo_connect, long timeo_send);

            // start async socket service
            void       start_service(int working_counter = 1);

            // notify tcp_client to connect server
            void       notify_connect();

            // close tcp_client
            void       close();

            // Whether the client-->server connection  established.
            bool       is_connected(void) const { return this->connected_; }

            // Gets network error code
            ErrorCode  get_errorno(void) { return error_; }

            // post a async send request.
            void       async_send(std::vector<char>&& data, const xxappl_pdu_send_callback_t& callback = nullptr);


            // ***********  p2p support *****************
            // open p2p by local endpoint of "server --> local connection"
            void       p2p_open();

            // peer connect request detected by 'select', peer --> local connection established.
            bool       p2p_do_accept(void);

            // try to connect peer. local --> peer connection
            void       p2p_async_connect(const ip::endpoint& ep);

            // shutdown p2p, TODO: implement
            void       p2p_shutdown(void);

        private:

            void       register_descriptor(const socket_native_type fd, int flags);
            void       unregister_descriptor(const socket_native_type fd, int flags);

            void       wait_connect_notify();

            bool       connect(void);

            void       service(void);

            bool       do_write(xxp2p_io_ctx*);
            bool       do_read(xxp2p_io_ctx*);
            void       move_received_pdu(xxp2p_io_ctx*); // move received properly pdu to recv queue

            void       handle_error(void); // TODO: add errorcode parameter

        private:
            bool                    app_exiting_;
            bool                    thread_started_;

            std::string             address_;
            std::string             addressv6_;
            u_short                 port_;

            ErrorCode               error_;

            int                     socket_error_;

            int                     connect_timeout_;
            int                     send_timeout_;

            bool                    connect_failed_;

            std::mutex              recv_queue_mtx_;
            std::queue<std::vector<char>> recv_queue_; // the recev_queue_ for connections: local-->server, local-->peer, peer-->local 

            std::mutex              connect_notify_mtx_;
            std::condition_variable connect_notify_cv_;


            // socket event set
            int maxfdp_;
            fd_set readfds_, writefds_, excepfds_;
            connect_listener        connect_listener_;
            xxappl_pdu_recv_callback_t on_received_pdu_;
            decode_pdu_length_func  decode_pdu_length_;
            build_error_func        build_error_pdu_;
            std::function<void(const vdcallback_t&)> call_tsf_;

            bool                    idle_;

            bool                    suspended_ = false;
            long long               total_connect_times_ = 0;

            std::atomic<int>        working_counter_; // service working counter, default: 1

            // p2p support
            xxsocket                p2p_acceptor_; // p2p: the acceptor for connections: peer-->local
            ip::endpoint            p2p_peer_endpoint_; // p2p: this should tell by server
            xxp2p_io_ctx            p2p_channel1_; // p2p: connection: local ---> peer
            xxp2p_io_ctx            p2p_channel2_; // p2p: connection: peer ---> local 
        }; // xxtcp_client
    }; /* namspace purelib::net */
}; /* namespace purelib */
#endif

#define tcpcli purelib::gc::singleton<purelib::inet::xxtcp_client>::instance()
