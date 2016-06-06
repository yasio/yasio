//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "connection.hpp"
#include <utility>
#include <vector>
#include "thelib/utils/crypto_wrapper.h"
#include "thelib/utils/nsconv.h"
#include "thelib/utils/xxsocket.h"
#include "server_logger.hpp"
#include "http_client.hpp"
#include "connection_manager.hpp"
#include "request_handler.hpp"

#include "thelib/utils/object_pool_with_mutex.h"

enum { max_http_package_size = 16384 };

using namespace thelib::net;

namespace http {
namespace server {

static std::string to_string(boost::asio::streambuf& buffer)
{
    boost::asio::streambuf::const_buffers_type bufs = buffer.data();
    std::string data(
        boost::asio::buffers_begin(bufs),
        boost::asio::buffers_begin(bufs) + buffer.size());
    return std::move(data);
}

connection::connection(boost::asio::ip::tcp::socket socket,
    connection_manager& manager/*, request_handler& handler*/)
    : strand_(socket.get_io_service()),
    socket_(std::move(socket)),
    connection_manager_(manager),
    request_handler_(*this),
    total_bytes_received_(0),
    timestamp_(0)
{
    request_.header_length = 0;
    request_.content_length = 0;
    memset(this->v4_address_, 0x0, sizeof(this->v4_address_));
    //tid_ = this_thread();
    //LOG_TRACE_ALL("construct a connection resource ok, thread id:%u", tid_);
}

connection::~connection(void)
{
    LOG_TRACE_ALL("destruct a connection resource ok, thread id:%u\n", this_thread());
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
    
    timestamp_ = time(NULL);

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

void connection::do_read_head()
{
    auto self(shared_from_this());
    // bytes_transferred never grater than sizeof(buffer_)
    socket_.async_read_some(boost::asio::buffer(buffer_), strand_.wrap(
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        if (!ec)
        {
            timestamp_ = time(NULL);

            total_bytes_received_ += bytes_transferred;

            // check http package size
            if(total_bytes_received_ > max_http_package_size) {
                reply_ = reply::stock_reply(reply::bad_request);
                do_write();

                LOG_TRACE("too long http request from client: %s:u", this->get_address(), port_);
                return;
            }

            request_parser::result_type result;

            std::tie(result, std::ignore) = request_parser_.parse(
                request_, buffer_.data(), buffer_.data() + bytes_transferred);

            // check header
            switch(result) {
            case request_parser::good: // read header complete
                if(0 == stricmp(request_.method.c_str(), "POST"))
                { /// head read complete

                    // LOG_TRACE_ALL("header length:%d, content length:%d", request_.header_length, request_.content_length);
                    
                    size_t content_bytes = this->total_bytes_received_ - request_.header_length;
                    if(content_bytes > 0) 
                    { // put remain data to content
                        request_.content.append(buffer_.data() + (bytes_transferred - content_bytes), content_bytes); 
                    }

                    do_check();
                }
                else if(0 == stricmp(request_.method.c_str(), "GET"))
                {
                    request_handler_.handle_request(request_, reply_);
                    do_write();
                }
                else { // do not support other request
                    reply_ = reply::stock_reply(reply::bad_request);
                    do_write();
                }
                break;
            case request_parser::bad:
                reply_ = reply::stock_reply(reply::bad_request);
                do_write();
                break;
            default: // head insufficient
                do_read_head();
            }
        }
        else
        {
            LOG_TRACE("peer interrupt ahead of time, detail:%s", ec.message().c_str());
            
            connection_manager_.stop(shared_from_this());
            
            /*if(ec != boost::asio::error::operation_aborted) 
            {
                connection_manager_.stop(shared_from_this());
            }*/
        }
    }) );
}

void connection::do_read_content(void)
{
    auto self(shared_from_this());
    // bytes_transferred never grater than sizeof(buffer_)
    socket_.async_read_some(boost::asio::buffer(buffer_),strand_.wrap(
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred)
    {
        // LOG_TRACE_ALL("read data, thread id:%u", this_thread() );
        if (!ec)
        {
            timestamp_ = time(NULL);

            total_bytes_received_ += bytes_transferred;

            // check http package size
            if(total_bytes_received_ > max_http_package_size) {
                reply_ = reply::stock_reply(reply::bad_request);
                do_write();

                LOG_TRACE("too long http request from client: %s:u", this->get_address(), port_);
                return;
            }

            this->request_.content.append(buffer_.data(), bytes_transferred);

            do_check();
        }
        else
        {
            LOG_TRACE("peer interrupt ahead of time, detail:%s", ec.message().c_str());
            
            connection_manager_.stop(shared_from_this());
        }
    }) );
}

void connection::do_check(void)
{
    size_t size = request_.content.size();
    if( size == request_.content_length)
    {
        request_handler_.handle_request(request_, reply_);
        do_write();
    }
    else if(size < request_.content_length)
    {
        do_read_content();
    }
    else { // invalid http package
        reply_ = reply::stock_reply(reply::bad_request);
        do_write();
    }
}

void connection::do_write()
{
    auto self(shared_from_this());

    boost::asio::async_write(socket_, reply_.to_buffers(), strand_.wrap(
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

    } ) );
}

bool connection::is_timeout(void) const
{
    return (time(NULL) - timestamp_) >= 10; // data receive timeout: 10 seconds
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

