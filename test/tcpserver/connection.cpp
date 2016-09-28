//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// network
// header: 8 bytes total
//   - version: 4 bytes, version of protocol
//   - length: 4 bytes, length of body
// body:

#include "connection.hpp"
#include <utility>
#include <vector>
#include "utils/crypto_wrapper.h"
#include "utils/nsconv.h"
#include "utils/xxsocket.h"
#include "utils/xxbswap.h"
#include "server_logger.hpp"
#include "http_client.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

#include "utils/object_pool.h"

#include "pcode_autog_server_messages.h"

#include "utils/xxsocket.h"

#define HWP_MAGIC_NUMBER 0x6102
#define HWP_PACKET_HEADER_LENGTH 16
#define HWP_AES_KEYBIT 128


#define HWP_PACKET_HEADER_LENGTH 16
#define _DEFINE_KEYS \
const unsigned char information_key[] = { 0xe8, 0xef, 0xd7, 0x24, 0x20, 0x2d, 0xd3, 0x19, 0x28, 0x02, 0x7c, 0x07, 0xed, 0x69, 0x2a, 0xfa };

// 消息包头, 必须实现, 字段可选,但至少应该有表示协议包长度的字段
obinarystream pcode_autog_begin_encode(uint16_t command_id, uint16_t reserved = 0x6102)
{
    messages::MsgHeader hdr;
    hdr.command_id = command_id;
    hdr.length = 0;
    hdr.reserved = reserved;
    hdr.reserved2 = microtime();
    hdr.version = 1;
    return hdr.encode();
}

#define MAX_PDU_LEN SZ(16, k)

using namespace purelib::net;

#define thread_self GetCurrentThreadId

static bool decode_pdu_length(const char* data, size_t datalen, int& len, messages::MsgHeader* header)
{
    if (datalen >= HWP_PACKET_HEADER_LENGTH) {

        char hdrs[HWP_PACKET_HEADER_LENGTH];

        _DEFINE_KEYS;

        crypto::aes::detail::cbc_decrypt_init(data, HWP_PACKET_HEADER_LENGTH,
            hdrs, HWP_PACKET_HEADER_LENGTH,
            information_key, HWP_AES_KEYBIT);

        header->decode(hdrs, sizeof(hdrs));

        len = header->length + HWP_PACKET_HEADER_LENGTH;
        return HWP_MAGIC_NUMBER == header->reserved;
    }
    else {
        len = -1;
        return true;
    }
}


static const time_t connection_timeout_millisec = 7000; // 心跳7秒钟超时

namespace current {

    boost::posix_time::time_duration::tick_type milliseconds_since_epoch()
    {
        using boost::gregorian::date;
        using boost::posix_time::ptime;
        using boost::posix_time::microsec_clock;

        static ptime const epoch(date(1970, 1, 1));
        return (microsec_clock::universal_time() - epoch).total_milliseconds();
    }

}

namespace tcp {
namespace server {

//static std::string to_string(boost::asio::streambuf& buffer)
//{
//    boost::asio::streambuf::const_buffers_type bufs = buffer.data();
//    std::string data(
//        boost::asio::buffers_begin(bufs),
//        boost::asio::buffers_begin(bufs) + buffer.size());
//    return std::move(data);
//}

connection::connection(boost::asio::ip::tcp::socket socket,
    connection_manager& manager/*, request_handler& handler*/)
    : 
    timeout_check_timer_(socket.get_io_service()),
    socket_(std::move(socket)),
    connection_manager_(manager),
    request_handler_(*this),
    total_bytes_received_(0)
{
    ctx_.reset();
    memset(this->v4_address_, 0x0, sizeof(this->v4_address_));
    //tid_ = this_thread();
    //LOG_TRACE_ALL("construct a connection resource ok, thread id:%u", tid_);
}

connection::~connection(void)
{
    LOG_TRACE_ALL("destruct a connection resource ok, thread id:%u\n", thread_self());
}


void connection::start()
{ 
    boost::system::error_code ec;
    auto endpoint = socket_.remote_endpoint(ec);
    if(ec)
    { // pitfall: if no check, server will down by: Transport endpoint is not connected
        connection_manager_.stop_locked(shared_from_this());
        LOG_TRACE("a exception connection appear, stop it immediately, detail:%s", ec.message().c_str());
        return;
    }

    linger l;
    l.l_onoff = 1;
    l.l_linger = 0;
    xxsocket::set_optval(socket_.native_handle(), SOL_SOCKET, SO_LINGER, l);
    xxsocket::set_optval(socket_.native_handle(), IPPROTO_TCP, TCP_NODELAY, 1);
    xxsocket::set_optval(socket_.native_handle(), SOL_SOCKET, SO_SNDTIMEO, 10);
    xxsocket::set_optval(socket_.native_handle(), SOL_SOCKET, SO_RCVTIMEO, 10);

    sprintf(v4_address_, "%s", endpoint.address().to_string().c_str());
    port_ = endpoint.port();
    LOG_TRACE_ALL("a connect income, detail: %s:%u", v4_address_, port_);

    do_read_packet();
}

void  connection::cancel_timeout_wait() // 取消超时检查
{
    boost::system::error_code ignored_ec;
    this->timeout_check_timer_.cancel(ignored_ec);
}

void connection::expire_from_now()
{
    expire_time(current::milliseconds_since_epoch() + connection_timeout_millisec);
}

void  connection::do_timeout_wait() // 执行超时检查
{
    this->timeout_check_timer_.expires_from_now(boost::posix_time::seconds(7)); // 7秒超时
    auto self = shared_from_this();
    this->timeout_check_timer_.async_wait([this, self](const boost::system::error_code& ec) {
        if (!ec) { // 定时器到期
            LOG_ERROR("The connection %s is timeout.", v4_address_);
            shutdown();
        }
        else if (ec == boost::asio::error::operation_aborted) { // 继续检查
            if (!stopped_)
                do_timeout_wait();
        }
    });
}

void connection::shutdown()
{
    boost::system::error_code ignored_ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
        ignored_ec);
}

void connection::do_read_packet(size_t offset)
{
    auto self(shared_from_this());
    // bytes_transferred never grater than sizeof(buffer_)
    socket_.async_read_some(boost::asio::buffer(ctx_.buffer_ + offset, sizeof(ctx_.buffer_) - offset),
        [this, self, offset](boost::system::error_code ec, std::size_t n)
    {
        if (!ec)
        {
            total_bytes_received_ += n; // ONLY 统计
            
            if (!unpack(n))
            {
                goto _L_error;
            }

            while (ctx_.offset_ > 0) {
                if (!unpack(0))
                {
                    goto _L_error;
                }
            }

            // post next read.
            do_read_packet(ctx_.offset_);

            return;
        }

  _L_error:
        LOG_WARN_ALL("peer interrupt ahead of time, detail:%s", ec.message().c_str());

        handle_close(1);
    } );
}

bool connection::unpack(int n)
{
    if (ctx_.expected_size_ == -1) { // decode length
        if (decode_pdu_length(ctx_.buffer_, ctx_.offset_ + n, ctx_.expected_size_, &ctx_.header_))
        {
            if (ctx_.expected_size_ < 0) { // header insuff
                ctx_.offset_ += n;
            }
            else { // ok
                ctx_.packet_.reserve(ctx_.expected_size_); // #perfomance, avoid memory reallocte.

                auto bytes_transferred = n + ctx_.offset_;
                ctx_.packet_.insert(ctx_.packet_.end(), ctx_.buffer_, ctx_.buffer_ + (std::min)(ctx_.expected_size_, bytes_transferred));
                if (bytes_transferred >= ctx_.expected_size_)
                { // pdu received properly
                    ctx_.offset_ = bytes_transferred - ctx_.expected_size_; // set offset to bytes of remain buffer
                    if (ctx_.offset_ > 0)  // move remain data to head of buffer and hold offset.
                        ::memmove(ctx_.buffer_, ctx_.buffer_ + ctx_.expected_size_, ctx_.offset_);

                    // move properly pdu to ready queue, GL thread will retrieve it.
                    // move_received_pdu(ctx); // TODO: ok, handle it.
                    handle_request();
                }
                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                    ctx_.offset_ = 0;
                }
            }
        }
        else {
            // error_ = error_number::ERR_DPL_ILLEGAL_PDU;
            return false;
        }
    }
    else { // recv remain pdu data
        auto bytes_transferred = n + ctx_.offset_;// bytes transferred at this time
        if ((ctx_.packet_.size() + bytes_transferred) > MAX_PDU_LEN) // TODO: config MAX_PDU_LEN, now is 16384
        {
            // error_ = error_number::ERR_PDU_TOO_LONG;
            // break;
            return false;
        }
        else {
            auto bytes_needed = ctx_.expected_size_ - static_cast<int>(ctx_.packet_.size()); // never equal zero or less than zero
            if (bytes_needed > 0) {
                ctx_.packet_.insert(ctx_.packet_.end(), ctx_.buffer_, ctx_.buffer_ + (std::min)(bytes_transferred, (bytes_needed)));

                ctx_.offset_ = bytes_transferred - bytes_needed; // set offset to bytes of remain buffer
                if (ctx_.offset_ >= 0)
                { // pdu received properly
                    if (ctx_.offset_ > 0)  // move remain data to head of buffer and hold offset.
                        ::memmove(ctx_.buffer_, ctx_.buffer_ + bytes_needed, ctx_.offset_);

                    // move properly pdu to ready queue, GL thread will retrieve it.
                    // move_received_pdu(ctx); // ok;
                    handle_request();
                }
                else { // all buffer consumed, set offset to ZERO, pdu incomplete, continue recv remain data.
                    ctx_.offset_ = 0;
                }
            }
            else { // Never go here.
                   //assert(false);
            }
        }
    }

    return true;
}

void connection::send(buffer_type&& buffer)
{
    boost::asio::async_write(socket_, boost::asio::buffer(std::move(buffer)),
        [this/*, self*/](boost::system::error_code ec, std::size_t)
    {
        if (ec)
        {
            // Initiate graceful connection closure.
            handle_close(2);

            LOG_TRACE("The connection:%s shoud be closed, bacause:%s", ec.message().c_str(), this->v4_address_);
        }
    });
}

void connection::send_to_peer(const buffer_type& buffer)
{
    if (!this->peer_.expired())
    {
        auto peer_self = this->peer_.lock();

        std::string copied = buffer;
        
        timeval timeout = { 0, 100 };
        if (-1 == xxsocket::send_n(peer_self->get_socket().native_handle(), buffer.c_str(), buffer.size(), &timeout))
        {
            shutdown();
        }

        //boost::asio::async_write(peer_self->socket_, boost::asio::buffer(copied),
        //    [peer_self](boost::system::error_code ec, std::size_t)
        //{
        //    if (ec)
        //    {
        //        // Initiate graceful connection closure.
        //        peer_self->handle_close(2);

        //        LOG_TRACE("The connection:%s shoud be closed, bacause:%s", ec.message().c_str(), peer_self->v4_address_);
        //    }
        //});
    }
}

void connection::handle_request()
{
    if (!this->request_handler_.handle_request(this->ctx_))
    {
        LOG_ERROR("handle request failed: detail:%s", this->v4_address_);
        handle_close();
    }
}

void connection::handle_close(int op)
{
    if (!stopped_) {

        LOG_TRACE_ALL("The connection:%s is closed", this->v4_address_);

        stopped_ = true;

        if (this->socket_.is_open()) {
            // Initiate graceful connection closure.
            boost::system::error_code ignored_ec;

            socket_.close(ignored_ec);
        }

        connection_manager_.stop(shared_from_this());
    }
}

boost::asio::ip::tcp::socket& connection::get_socket()
{
    return socket_;
}

const char* connection::get_address(void) const
{
    return v4_address_;
}

unsigned short connection::get_port(void) const
{
    return port_;
}

} // namespace server
} // namespace http

