#include "player_message_handler.h"
#include "player_server.h"
#include "game_hall.h"
#include "player_session_manager.h"
#include "auto_resources.h"
#include "server_config.h"
#include "player_session.h"

#ifdef _WIN32
#pragma warning(disable:4355) // disable the warnning about: used in base member initializer list, for use pool exactly
#endif

static const char tgw_header0[] = "tgw_l7_forward\r\nHost: app100700801.qzone?";
static const char tgw_header1[] = "tgw_l7_forward\r\nHost: app100700801.qzone.qzoneapp.com:8008\r\n\r\n";
static const char tgw_header2[] = "tgw_l7_forward\r\nHost: app100700801.qzoneapp.com:8008\r\n\r\n";
static const char tgw_header3[] = "tgw_l7_forward\r\nHost: app100700801.t.tqapp.cn:8008\r\n\r\n";
static const char TGW_H1_CHAR = '.';
static const char TGW_H2_CHAR = 'a';
static const char TGW_H3_CHAR = 'p';

player_session::player_session(player_session_manager& owner, boost::asio::io_service& io_service) : manager_(owner), 
    message_handler_(*this, owner.owner_.get_hall_manager()),
    s_(io_service),
    bytes_to_consume_(0),
    is_kicked_(false),
    is_logged_off_(false),
    timeout_check_timer_(io_service)
{
}

player_session::~player_session(void)
{
    this->close();
}

void player_session::close(void)
{
    if(this->s_.is_open())
    {
        this->s_.close();
    }
}

tcp::socket& player_session::get_socket(void)
{
    return this->s_;
}

uint32_t player_session::get_id(void) const
{
    return this->message_handler_.player_info_.ci_.id_;
}

const char*  player_session::get_addr_info(void) const
{
    return this->detail_;
}

void player_session::set_options(void)
{
    //xxsocket::set_keepalive(this->s_.native_handle(), 1, 30, 10, 3);
    //the tcp keepalive is not valid when to player in a same room, and one of the two
    //disconnect exceptional(such cut down network line), the tcp keepalive can't dectect,
    //why?

    xxsocket::set_optval(this->s_.native_handle(), SOL_SOCKET, SO_SNDBUF, SZ(ar_server_config.nci_.send_buffer_size_,k));
    xxsocket::set_optval(this->s_.native_handle(), SOL_SOCKET, SO_RCVBUF, SZ(ar_server_config.nci_.recv_buffer_size_,k));
    this->s_.non_blocking(true);                                     
}

bool player_session::is_valid(void) const
{
    return this->s_.is_open();
}

bool player_session::is_kicked(void) const
{
    return this->is_kicked_;
}

void player_session::init(void)
{
    this->set_options();
    xxsocket temp(this->s_.native_handle());
    temp.get_peer_addr().to_string_full(this->detail_);
    temp.release_handle();
}

void player_session::start(void)
{
    this->message_handler_.on_connection();

    boost::asio::async_read(this->s_, 
        boost::asio::buffer(this->recv_buf_),
        boost::asio::transfer_exactly((sizeof(tgw_header0) - 1)),
        boost::bind(&player_session::handle_tgw_header_part1, 
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));

    // this->post_timeout_checking(); check by manager
}

void  player_session::logoff(void)
{
    scoped_lock<thread_mutex> guard(logoff_mutex_);

    if(!this->is_logged_off_)
    {
        this->is_logged_off_ = true;
        this->message_handler_.on_disconnect();
        this->timeout_check_timer_.cancel();
        this->manager_.unregister(this);
    }
}

bool player_session::trylogon(void)
{
    return this->manager_.tryregister(this);
}

player_session* player_session::relogon(void)
{
    return this->manager_.reregister(this);
}

void player_session::kick(const char* detail)
{
    this->is_kicked_ = true;
    try 
    {
        this->logoff();
        this->s_.shutdown(boost::asio::socket_base::shutdown_send);
    }
    catch(...)
    {
    }
}

void  player_session::send(const void* msg, int len)
{
#if 0
    this->post_send(msg, len);
#else
    timeval timeo;
    timeo.tv_sec = 0;
    timeo.tv_usec = 10000; // 10 milliseconds
    scoped_lock<thread_mutex> guard(this->send_lock_);
    // ignore send return, if error occured, the recv operation can always sense
    if(-1 == xxsocket::send_n(this->s_.native_handle(), msg, len, &timeo, 10))
    { // send timeout, close the connection directlys
        this->close();
    }
#endif
}

void player_session::handle_tgw_header_part1(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if(!ec) {
        switch(this->recv_buf_[bytes_transferred - 1])
        {
        case TGW_H1_CHAR:
            boost::asio::async_read(this->s_, 
                boost::asio::buffer(this->recv_buf_ + bytes_transferred, MAX_IOBUF_SIZE - bytes_transferred),
                boost::asio::transfer_exactly(sizeof(tgw_header1) - sizeof(tgw_header0)),
                boost::bind(&player_session::handle_tgw_header_part2, 
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            break;
        case TGW_H2_CHAR:
            boost::asio::async_read(this->s_, 
                boost::asio::buffer(this->recv_buf_ + bytes_transferred, MAX_IOBUF_SIZE - bytes_transferred),
                boost::asio::transfer_exactly(sizeof(tgw_header2) - sizeof(tgw_header0)),
                boost::bind(&player_session::handle_tgw_header_part2, 
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            break;
        case TGW_H3_CHAR:
            boost::asio::async_read(this->s_, 
                boost::asio::buffer(this->recv_buf_ + bytes_transferred, MAX_IOBUF_SIZE - bytes_transferred),
                boost::asio::transfer_exactly((sizeof(tgw_header3) - sizeof(tgw_header0))),
                boost::bind(&player_session::handle_tgw_header_part2, 
                this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
            break;
        default:
            this->manager_.cleanup(this);
        }
        return;
    }

    this->manager_.cleanup(this);
}

void player_session::handle_tgw_header_part2(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if(!ec)
    {
        return this->post_recv(this->recv_buf_, MAX_IOBUF_SIZE, PLAYER_MSG_HDR_SIZE);
    }

    this->manager_.cleanup(this);
}

void player_session::post_timeout_checking(void)
{
    this->timeout_check_timer_.expires_from_now(boost::posix_time::seconds(ar_server_config.heartbeat_check_interval_));
    this->timeout_check_timer_.async_wait(
        boost::bind(
        &player_session::on_timeout_checking,
        this,
        boost::asio::placeholders::error)
        );
}

void  player_session::post_recv(char* buffer, size_t size, size_t bytes_at_least)
{
    boost::asio::async_read(this->s_, 
        boost::asio::buffer(buffer, size),
        boost::asio::transfer_at_least(bytes_at_least),
        boost::bind(&player_session::on_recv_completed, 
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void player_session::post_send(const void* bytestream, size_t size)
{
    char* buffer = (char*)mpool_alloc(size);
    if(buffer != nullptr) 
    {
        memcpy(buffer, bytestream, size);
        boost::asio::async_write(this->s_,
            boost::asio::buffer(buffer, size),
            boost::bind(&player_session::on_send_completed, 
            this,
            buffer, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred ) );
    }
} 

void player_session::on_recv_completed(const boost::system::error_code& ec, size_t bytes_transferred)
{
    if(!ec)
    {
        char* ptr = this->recv_buf_;

        this->bytes_to_consume_ += bytes_transferred;
        do
        {
            size_t msg_size = this->message_handler_.decode_message_length(ptr, this->bytes_to_consume_);
            if(msg_size <= (size_t)MAX_IOBUF_SIZE)
            {
                if(0 == msg_size)
                { // header not sufficiently
                    //LOG_TRACE("(header)tcp packages reconstruction for the session: %s\n", this->detail_);

                    ::memmove(this->recv_buf_, ptr, this->bytes_to_consume_);
                    return this->post_recv(this->recv_buf_ + this->bytes_to_consume_, MAX_IOBUF_SIZE - this->bytes_to_consume_, PLAYER_MSG_HDR_SIZE - this->bytes_to_consume_);
                }
                else if(msg_size <= this->bytes_to_consume_) 
                {
                    if(this->message_handler_.on_message(ptr, msg_size)) 
                    {
                        ptr += msg_size;
                        this->bytes_to_consume_ -= msg_size;
                    }
                    else 
                    {
                        LOG_TRACE("message handle exception(MT:%d,TSOM:%d) for the session: %s\n", ptr[0], msg_size, this->detail_);
                        goto _L_Error;
                    }
                }
                else { // body not sufficiently
                    //LOG_TRACE("(body)tcp packages reconstruction occured for the session: %s\n", this->detail_);

                    ::memmove(this->recv_buf_, ptr, this->bytes_to_consume_);
                    return this->post_recv(this->recv_buf_ + this->bytes_to_consume_, MAX_IOBUF_SIZE - this->bytes_to_consume_, msg_size - this->bytes_to_consume_);
                }
            }
            else {
                LOG_ERROR("message length exception(MT:%d,TSOM:%d) for the session: %s, offset(%d), bytes_transferred(%d)\n", ptr[0], msg_size, this->detail_, ptr - this->recv_buf_, bytes_transferred);
                goto _L_Error;
            }
        } while(this->bytes_to_consume_ > 0);

        // consume all the data perfectly, so post next receive-request
        return this->post_recv(this->recv_buf_, MAX_IOBUF_SIZE, PLAYER_MSG_HDR_SIZE);
    }

    // error occured, socket error or handle message error
_L_Error:
    LOG_TRACE("connection error for the session: %s, reason: %s\n", this->detail_, ec.message().c_str());
    this->logoff();
    this->manager_.cleanup(this);
}

void player_session::on_send_completed(char* buffer, const boost::system::error_code& ec, size_t bytes_transffer)
{
    mpool_free(buffer);
    if(ec) 
    {
        LOG_TRACE_ALL("on_send_completed, error occured, detail: %s\n", ec.message().c_str());
    }
}

void player_session::on_timeout_checking(const boost::system::error_code& ec)
{
    if(!ec) 
    {
        if(!this->execute_timeout_check())
        {
            this->post_timeout_checking();
        }
    }
}

bool  player_session::execute_timeout_check(void)
{
    if(this->message_handler_.is_heartbeat_timeout())
    {
        LOG_TRACE_ALL("heartbeat of the session: %s is timeout, now shutting down the session.\n", this->detail_);
        this->logoff();
        this->close();
        return true;
    }
    return false;
}


