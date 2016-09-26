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

enum { max_package_size = 16384 };

using namespace thelib::net;

#define thread_self GetCurrentThreadId

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
    : socket_(std::move(socket)),
    connection_manager_(manager),
    request_handler_(*this),
    total_bytes_received_(0)
{
    request_.expecting_size_ = 0;
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

    do_read_head();
}

void connection::stop()
{
    if(this->socket_.is_open() ) 
    {
        boost::system::error_code ignored_ec;
        socket_.close(ignored_ec);
        LOG_TRACE_ALL("the connection:%s:%u leave.", v4_address_, port_);
    }
}

void connection::do_read_head(size_t offset)
{
    auto self(shared_from_this());
    // bytes_transferred never grater than sizeof(buffer_)
    socket_.async_read_some(boost::asio::buffer(buffer_.data() + offset, buffer_.size() - offset),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if (!ec)
        {
            total_bytes_received_ += bytes_transferred;

            if(total_bytes_received_ <= max_package_size) {

                if(total_bytes_received_ >= sizeof(uint32_t)) 
                {
                    request_.expecting_size_ = ntohl( *(uint32_t*)buffer_.data() );
                    if(request_.expecting_size_ < max_package_size - sizeof(uint32_t) )
                    {
                        request_.content_.append(buffer_.data() + sizeof(uint32_t), total_bytes_received_ - sizeof(uint32_t));
                        if(request_.content_.size() == request_.expecting_size_)
                        {
                            if(request_handler_.handle_request(request_))
                            {
                                return do_write();
                            }
                        }
                        else {
                            return do_read_content();
                        }
                    }
                }
                else { // header insufficient
                    return do_read_head(total_bytes_received_);
                }
            }
        }

        LOG_TRACE("peer interrupt ahead of time, detail:%s", ec.message().c_str());

        connection_manager_.stop(shared_from_this());

        /*if(ec != boost::asio::error::operation_aborted) 
        {
        connection_manager_.stop(shared_from_this());
        }*/
    } );
}

void connection::do_read_content(void)
{
    auto self(shared_from_this());
    // bytes_transferred never grater than sizeof(buffer_)
    socket_.async_read_some(boost::asio::buffer(buffer_), 
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        // LOG_TRACE_ALL("read data, thread id:%u", this_thread() ); TODO: continue read next package.
        if (!ec) {
            total_bytes_received_ += bytes_transferred;

            if(total_bytes_received_ <= max_package_size) {

                this->request_.content_.append(buffer_.data(), bytes_transferred);

                const size_t actual_size = request_.content_.size();
                if( actual_size == request_.expecting_size_)
                {
                    if(!request_handler_.handle_request(request_) )
                        close();
                }
                else if(actual_size < request_.expecting_size_)
                {
                    return do_read_content();
                }
            }
        }

        LOG_TRACE("peer interrupt ahead of time, detail:%s", ec.message().c_str());
        connection_manager_.stop(shared_from_this());
    } );
}


void connection::close()
{
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;

    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
        ignored_ec);

    stop();
}

void connection::do_write()
{
#if 0
    auto self(shared_from_this());

    boost::asio::async_write(socket_, boost::asio::buffer(reply_.content),
        [this, self](boost::system::error_code ec, std::size_t)
    {
        if (!ec)
        {
            // Initiate graceful connection closure.
            boost::system::error_code ignored_ec;

            socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both,
                ignored_ec);
        }

        connection_manager_.stop(shared_from_this());

    } );
#endif
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

