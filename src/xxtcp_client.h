#ifndef _XXSOCKET_AIO_H_
#define _XXSOCKET_AIO_H_
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

namespace purelib {

    namespace net {
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
            ERR_pdu_TOO_LONG, // pdu too long
            ERR_DPL_ILLEGAL_pdu, // decode pdu error.
        };


        class xxtcp_client; // A tcp client with P2P supports.
        class xxappl_pdu; // application layer protocol data unit.

        typedef std::function<void(ErrorCode)> xxappl_pdu_sent_callback_t;
        typedef std::function<void(std::vector<char>&&)> xxappl_pdu_received_callback_t;

        struct xxp2p_io_ctx
        {
            xxtcp_client*              client_;
            xxsocket*                  channel_ = nullptr;
            P2PChannelType             channel_type_ = TCP_P2P_UNKNOWN;
            char                       buffer_[65536]; // recv buffer
            int                        offset_ = 0; // recv buffer offset
            std::mutex                 send_queue_mtx_;
            std::queue<xxappl_pdu*>    send_quene_;
            std::vector<char>          receiving_pdu_;
            int                        expected_pdu_len_ = -1;
            int                        error = 0;

            bool do_read(void);
            bool do_write(void);
        };

        // A tcp client support P2P
        class xxtcp_client
        {
            friend struct xxp2p_io_ctx;
        public:

            // End user pdu decode length func
            typedef bool(*DecodepduLengthFunc)(const char* data, size_t datalen, int& len);

            // BuildErrorInfoFunc
            typedef std::vector<char>(*BuildErrorpduFunc)(int errorcode, const char* errormsg);

        public:
            xxtcp_client();
            ~xxtcp_client();

            void       set_endpoint(const char* address, const char* addressv6, u_short port);

            void       set_callbacks(DecodepduLengthFunc decode_length_func,
                BuildErrorpduFunc build_error_pdu_func,
                const xxappl_pdu_received_callback_t& callback);

            void       set_timeouts(long timeo_connect, long timeo_send);

            void       start_service();

            void       start_p2p_service();

            void       close();

            bool       is_connected(void) const { return this->impl_.is_open(); }

            ErrorCode  get_errorno(void) { return error_; }

            void       async_send(std::vector<char>&& data, const xxappl_pdu_sent_callback_t& callback = nullptr);

            void       notify_connect();

            /// p2p support
            void       update_p2p_endpoint(const ip::endpoint& ep);
            void       shutdown_p2p_chancel(void);

 
            /// call on main thread
            bool       collect_received_pdu(float dt = 0);

        private:

            void       wait_connect_notify();

            bool       connect(void);

            void       service(void);

            bool       do_write(void);
            bool       do_read(void);

            void       handle_error(void); // TODO: add errorcode parameter

            void       move_received_pdu(); // move received properly pdu to recv queue


            /// p2p supports
            bool       p2p_connect(void);
            void       p2p_service(void);
            void       p2p_wait_connect_notify();
            void       p2p_move_received_pdu(xxp2p_io_ctx* ctx);
            void       p2p_handle_error(xxp2p_io_ctx* ctx);

        private:
            bool                    app_exiting_;
            bool                    thread_started_;

            std::string             address_;
            std::string             addressv6_;
            u_short                 port_;

            xxsocket                impl_;
            ErrorCode               error_;

            int                     socket_error_;

            int                     connect_timeout_;
            int                     send_timeout_;

            bool                    connect_failed_;

            std::mutex              send_queue_mtx_;
            std::mutex              recv_queue_mtx_;

            std::queue<xxappl_pdu*> send_queue_;
            std::queue<std::vector<char>> recv_queue_;

            std::mutex              connect_notify_mtx_;
            std::condition_variable connect_notify_cv_;

            char                    buffer_[65536]; // recv buffer
            int                     offset_; // recv buffer offset
            std::vector<char>       receiving_pdu_;
            int                     expected_pdu_length_;

            xxp2p_io_ctx            channel1_; // p2p connection: local ---> peer
            xxp2p_io_ctx            channel2_; // p2p connection: peer ---> local

            // socket event set
            // fd_set readfds, writefds, excepfds;

            xxappl_pdu_received_callback_t on_received_pdu_;

            DecodepduLengthFunc  decode_pdu_length_;
            BuildErrorpduFunc    build_error_pdu_;

            bool                    idle_;


            // p2p support
            std::mutex              p2p_conn_notify_mtx_;
            std::condition_variable p2p_conn_notify_cv_;
            ip::endpoint            p2p_available_endpoint_;
            xxsocket                p2p_acceptor_;
            xxsocket                p2p_channel1_; // local --> peer socket
            xxsocket                p2p_channel2_; // peer --> local socket


            std::atomic<int>        working_counter_;

            bool                    suspended_ = false;
 
            long long               total_connect_times_ = 0;

            std::function<void(bool succeed, int ec)> connect_listener_;

        }; // xxtcp_client
    }; /* namspace purelib::net */
}; /* namespace purelib */
#endif

#define tcpcli purelib::gc::singleton<inet::xxtcp_client>::instance()
