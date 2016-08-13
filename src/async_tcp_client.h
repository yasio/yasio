#ifndef _async_tcp_client_H_
#define _async_tcp_client_H_
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
#include "select_interrupter.h"

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

        class appl_pdu; // application layer protocol data unit.

        typedef std::function<void(ErrorCode)> appl_pdu_send_callback_t;
        typedef std::function<void(std::vector<char>&&)> appl_pdu_recv_callback_t;

        template<typename _Ty, typename _Pr = std::less<_Ty>>
        class sorted_multilist
        {
            struct _List_entry
            {
                _Ty      value;
                _List_entry* next;
#if 1
                static void * operator new(size_t /*size*/)
                {
                    return get_pool().get();
                }

                static void operator delete(void *p)
                {
                    get_pool().release(p);
                }

                static purelib::gc::object_pool<_List_entry>& get_pool()
                {
                    static purelib::gc::object_pool<_List_entry> s_pool;
                    return s_pool;
                }
#endif
            };

        public:
            sorted_multilist() : _Mysize(0), _Myhead(nullptr)
            {
            }

            ~sorted_multilist()
            {
                if (!empty()) {
                    clear();
                }
            }

            bool empty() const
            {
                return _Mysize == 0;
            }

            size_t size() const
            {
                return _Mysize;
            }

            void clear()
            {
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    *ptr = entry->next;
                    delete (entry);
                }
                _Myhead = nullptr;
                _Mysize = 0;
            }

            template<typename _Fty>
            size_t remove_if(const _Fty& cond)
            {
                size_t count = 0;
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    if (cond(entry->value)) {
                        *ptr = entry->next;
                        delete (entry);
                        ++count;
                    }
                    else {
                        ptr = &entry->next;
                    }
                }
                _Mysize -= count;
                return count;
            }

            bool remove(const _Ty& value)
            {
                for (auto ptr = &_Myhead; *ptr;)
                {
                    auto entry = *ptr;
                    if (value == entry->value) {
                        *ptr = entry->next;
                        delete (entry);
                        --_Mysize;
                        return true;
                    }
                    else {
                        ptr = &entry->next;
                    }
                }
                return false;
            }

            template<typename _Fty>
            void foreach(const _Fty& callback)
            {
                for (auto ptr = _Myhead; ptr != nullptr; ptr = ptr->next)
                {
                    callback(ptr->value);
                }
            }

            template<typename _Fty>
            void foreach(const _Fty& callback) const
            {
                for (auto ptr = _Myhead; ptr != nullptr; ptr = ptr->next) 
                {
                    callback(ptr->value);
                }
            }

            const _Ty& front() const
            {
                assert(!empty());
                return _Myhead->value;
            }

            _Ty& front()
            {
                assert(!empty());
                return _Myhead->value;
            }

            void pop_front()
            {
                assert(!empty());
                auto deleting = _Myhead;
                _Myhead = _Myhead->next;
                delete deleting;
                --_Mysize;
            }

            void insert(const _Ty& value)
            {
                const _Pr comparator;
                if (_Myhead != nullptr) {
                    auto ptr = _Myhead;
                    while (comparator(ptr->value, value) && ptr->next != nullptr)
                    {
                        ptr = ptr->next;
                    }

                    // if (value == ptr->value)
                    // { // duplicate
                    //    return 0;
                    // }

                    auto newNode = _Buynode(value);

                    if (ptr->next != nullptr)
                    {
                        newNode->next = ptr->next;
                        ptr->next = newNode;
                    }
                    else
                    {
                        ptr->next = newNode;
                    }

                    if (comparator(value, ptr->value))
                    {
                        std::swap(ptr->value, newNode->value);
                    }
                }
                else {
                    _Myhead = _Buynode(value);
                }
                ++_Mysize;
            }
        private:
            _List_entry* _Buynode(const _Ty& value)
            {
                auto _Newnode = new _List_entry;
                _Newnode->value = value;
                _Newnode->next = nullptr;
                return _Newnode;
            }
        private:
            _List_entry* _Myhead;
            size_t       _Mysize;
        };

        template<typename _Ty, typename _Mtx = std::recursive_mutex>
        class threadsafe_slist
        {
        public:
            void insert(const _Ty& value)
            {
                std::lock_guard<_Mtx> guard(mtx);
                queue_.insert(value);
            }

            void insert_unlocked(const _Ty& value)
            {
                queue_.insert(value);
            }

            void remove(const _Ty& value)
            {
                std::lock_guard<_Mtx> guard(mtx);
                queue_.remove(value);
            }

            bool empty() const
            {
                std::lock_guard<_Mtx> guard(this->mtx_);
                this->queue_.empty();
            }

            size_t size() const
            {
                std::lock_guard<_Mtx> guard(this->mtx_);
                this->queue_.size();
            }

            void clear()
            {
                std::lock_guard<_Mtx> guard(this->mtx_);
                queue_.clear();
            }

            template<typename _Fty>
            void foreach(const _Fty& callback) const
            {
                std::lock_guard<_Mtx> guard(this->mtx_);
                queue_.foreach(callback);
            }

            template<typename _Fty>
            void foreach(const _Fty& callback)
            {
                std::lock_guard<_Mtx> guard(this->mtx_);
                queue_.foreach(callback);
            }

        private:
            sorted_multilist<_Ty> queue_;
            mutable _Mtx mtx_;
        };

        struct p2p_io_ctx
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
            std::deque<appl_pdu*>      send_queue_;

            void                       reset();
        };

        struct per_timer_data
        {
            void expires_from_now(const std::chrono::microseconds& duration)
            {
                expire_time = std::chrono::steady_clock::now() + duration;
            }

            bool expired() const 
            {
               return get_duration().count() <= 0;
            }

            std::chrono::microseconds get_duration() const
            {
                return std::chrono::duration_cast<std::chrono::microseconds>(expire_time - std::chrono::steady_clock::now());
            }

            bool loop;
            std::function<void(bool cancelled)> callback;

            std::chrono::time_point<std::chrono::steady_clock> expire_time;
        };

        // A tcp client support P2P
        class async_tcp_client : public p2p_io_ctx
        {
            friend struct p2p_io_ctx;
        public:

            // End user pdu decode length func
            typedef bool(*decode_pdu_length_func)(const char* data, size_t datalen, int& len);

            // BuildErrorInfoFunc
            typedef std::vector<char>(*build_error_func)(int errorcode, const char* errormsg);

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
                build_error_func build_error_pdu_func,
                const appl_pdu_recv_callback_t& callback,
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
            per_timer_data*  async_wait(const std::function<void(bool cancelled)>& callback, 
                const std::chrono::milliseconds& duration, 
                bool loop = false);

            void cancel_timer(per_timer_data*);

            

        private:
            void       perform_timeout_timers(); // ALL timer expired
            void       set_timeout(timeval& tv, long long max_duration);
            long long  wait_duration_usec(long long usec);

            void       register_descriptor(const socket_native_type fd, int flags);
            void       unregister_descriptor(const socket_native_type fd, int flags);

            void       wait_connect_notify();

            bool       connect(void);

            void       service(void);

            bool       do_write(p2p_io_ctx*);
            bool       do_read(p2p_io_ctx*);
            void       move_received_pdu(p2p_io_ctx*); // move received properly pdu to recv queue

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
            std::deque<std::vector<char>> recv_queue_; // the recev_queue_ for connections: local-->server, local-->peer, peer-->local 

            std::mutex              connect_notify_mtx_;
            std::condition_variable connect_notify_cv_;

            // select interrupter
            select_interrupter    interrupter_;

            // timer support
            sorted_multilist<per_timer_data*> timer_queue_;
            std::recursive_mutex              timer_queue_mtx_;

            // socket event set
            int maxfdp_;
            enum {
                read_op,
                write_op,
                except_op,
            };
            fd_set fdss_[3];
            // fd_set readfds_, writefds_, excepfds_;
            connect_listener        connect_listener_;
            appl_pdu_recv_callback_t on_received_pdu_;
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
            p2p_io_ctx              p2p_channel1_; // p2p: connection: local ---> peer
            p2p_io_ctx              p2p_channel2_; // p2p: connection: peer ---> local 
        }; // async_tcp_client
    }; /* namspace purelib::net */
}; /* namespace purelib */
#endif

#define tcpcli purelib::gc::singleton<purelib::inet::async_tcp_client>::instance()
