//
// async_client.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/ssl.hpp>
#include "thelib/utils/xxsocket.h"
using namespace thelib::net;

using boost::asio::ip::tcp;

static boost::asio::io_service io_service; // local io_service on as client

static void parse_uri(const std::string& uri, std::string& service, std::string& host, u_short& port, std::string& path)
{
    port = 0;

    // Parse host and path from URI
    size_t colon = uri.find_first_of(':');
    size_t slash = colon + 3;
    size_t port_colon = uri.find_first_of(':', slash);
    size_t path_slash = uri.find_first_of('/', slash);

    service = uri.substr(0, colon );

    if(port_colon == uri.npos) {
        host = uri.substr( slash, uri.find_first_of('/',slash) - slash);
    }
    else {
        std::string portS = uri.substr(port_colon + 1);
        port = atoi(portS.c_str());
        host = uri.substr( slash, port_colon - slash);
    }

    if(path_slash != std::string::npos) {
        path = uri.substr( path_slash );
    }
    else {
        path = "/";
    }
}

static std::string send_http_req_i(const std::string& host,
                                   u_short port,
                                   const std::string& path = "/",
                                   const std::string& method = "GET",
                                   const std::string& content = ""
                                   )
{
    try
    {
        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        // boost::asio::ip::tcp::resolver::query query("", port); 
        tcp::resolver::query query(host, "http");

        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        auto endpoint = endpoint_iterator->endpoint();
        if(port != 0) {
            endpoint.port(port);
        }
        // Try each endpoint until we successfully establish a connection.
        // tcp::socket socket(io_service);
        boost::asio::ip::tcp::socket socket(io_service);

        socket.open(boost::asio::ip::tcp::v4());  


        // ÉèÎª·Ç×èÈû  
        // socket.io_control(boost::asio::ip::tcp::socket::non_blocking_io(true));
        //xxsocket::set_nonblocking(socket.native_handle(), true);
        //socket.set_verify_callback(boost::asio::ssl::rfc2818_verification("sandbox.itunes.apple.com"));
        // socket.connect(endpoint);
        timeval tv = {10};
        if(xxsocket::connect_n(socket.native_handle(), endpoint.address().to_string().c_str(), 
            endpoint.port(), tv) != 0)
        {
            return "";
        }

        // boost::asio::connect(socket.lowest_layer(), endpoint_iterator);
        // socket.handshake(boost::asio::ssl::stream_base::client);
        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "Accept: */*\r\n";
        if(method == "POST") {
            request_stream << "Content-Type: application/json" << "\r\n";
            request_stream << "Content-Length: " << content.size() << "\r\n";
            request_stream << "Connection: Close\r\n\r\n";
            request_stream << content;
        }
        else {
            request_stream << "Connection: Close\r\n\r\n";
        }

        // Send the request.
        size_t bytes_transferred = boost::asio::write(socket, request);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, "\r\n");

        // Check that response is OK.
        std::istream response_stream(&response);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            std::cout << "Invalid response\n";
            return "";
        }
        if (status_code != 200)
        {
            std::cout << "Response returned with status code " << status_code << "\n";
            return "";
        }

        // Read the response headers, which are terminated by a blank line.
        boost::asio::read_until(socket, response, "\r\n\r\n");

        // Process the response headers.
        std::string header;
        while (std::getline(response_stream, header) && header != "\r")
            std::cout << header << "\n";
        std::cout << "\n";

        // Write whatever content we already have to output.
        // std::size_t n = boost::asio::read_until(sock, sb, '\n');
        boost::asio::streambuf::const_buffers_type bufs = response.data();
        std::string response_data(
            boost::asio::buffers_begin(bufs),
            boost::asio::buffers_begin(bufs) + response.size());

        // Read until EOF, writing data to output as we go.
        boost::system::error_code error;

        char buffer[512];
        memset(buffer, 0, sizeof(buffer));
        bytes_transferred = 0;

        while((bytes_transferred = 
            boost::asio::read(socket, boost::asio::buffer(buffer),
            boost::asio::transfer_at_least(1), error)) )
        {
            response_data.append(buffer, bytes_transferred);
        }

        /* while (boost::asio::read(socket, response,
        boost::asio::transfer_at_least(1), error))
        ss << &response;*/
        // std::string contentr = ss.str();
        if (error != boost::asio::error::eof)
        { // short read, peer already close the connect
            // throw boost::system::system_error(error);
            std::clog << "Warnning: peer already close the connection.\n";
        }

        return std::move(response_data);
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
        return "";
    }
}

static std::string send_https_req_i(const std::string& host,
                                    u_short port,
                                    const std::string& path = "/",
                                    const std::string& method = "GET",
                                    const std::string& content = ""
                                    )
{
    try
    {

        boost::asio::ssl::context ctx(boost::asio::ssl::context::sslv3);

        // Get a list of endpoints corresponding to the server name.
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(host, "https");
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        // Try each endpoint until we successfully establish a connection.
        // tcp::socket socket(io_service);
        boost::asio::ssl::stream<boost::asio::ip::tcp::socket> socket(io_service, ctx);
        socket.set_verify_mode(boost::asio::ssl::verify_none);
        
        auto endpoint = endpoint_iterator->endpoint();
        if(port != 0) {
            endpoint.port(port);
        }

        boost::asio::connect(socket.lowest_layer(), endpoint_iterator);

        socket.handshake(boost::asio::ssl::stream_base::client);
        // Form the request. We specify the "Connection: close" header so that the
        // server will close the socket after transmitting the response. This will
        // allow us to treat all data up until the EOF as the content.
        boost::asio::streambuf request;
        std::ostream request_stream(&request);
        request_stream << method << " " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << host << "\r\n";
        request_stream << "Accept: */*\r\n";
        if(method == "POST") {
            request_stream << "Content-Length: " << content.size() << "\r\n";
            request_stream << "Connection: Close\r\n\r\n";
            request_stream << content;
        }
        else {
            request_stream << "Connection: Close\r\n\r\n";
        }

        // Send the request.
        boost::asio::write(socket, request);

        // Read the response status line. The response streambuf will automatically
        // grow to accommodate the entire line. The growth may be limited by passing
        // a maximum size to the streambuf constructor.
        boost::asio::streambuf response_buffer;
        boost::asio::read_until(socket, response_buffer, "\r\n");

        // Read header to check that response is OK.
        std::istream response_stream(&response_buffer);
        std::string http_version;
        response_stream >> http_version;
        unsigned int status_code;
        response_stream >> status_code;
        std::string status_message;
        std::getline(response_stream, status_message);
        if (!response_stream || http_version.substr(0, 5) != "HTTP/")
        {
            std::cout << "Invalid response\n";
            return "";
        }
        if (status_code != 200)
        {
            std::cout << "Response returned with status code " << status_code << "\n";
            return "";
        }

        /// Consume all header
        boost::asio::read_until(socket, response_buffer, "\r\n\r\n");
        response_buffer.consume(response_buffer.size());
        
        // Read body until EOF, writing data to output as we go.
        boost::system::error_code error;

        while( (boost::asio::read(socket, response_buffer, boost::asio::transfer_at_least(1), error)) )
        {
            ; // response_data.append(buffer, bytes_transferred);
        }

        boost::asio::streambuf::const_buffers_type bufs = response_buffer.data();
        return std::string(
            boost::asio::buffers_begin(bufs),
            boost::asio::buffers_begin(bufs) + response_buffer.size());

        //if (error != boost::asio::error::eof)
        //{ // short read, peer already close the connect
        //    // throw boost::system::system_error(error);
        //    std::clog << "Warnning: peer already close the connection.\n";
        //}
        //return std::move(response_data);
    }
    catch (std::exception& e)
    {
        std::cout << "Exception: " << e.what() << "\n";
        return "";
    }
}

std::string send_http_req(const std::string& uri, const std::string& method = "POST", const std::string& content = "")
{
    std::string host, service, path;
    u_short port = 0;
    parse_uri(uri, service, host, port, path);
    if(service == "http") {
        return send_http_req_i(host, port, path, method, content);
    }
    else if(service == "https") {
        return send_https_req_i(host, port, path, method, content);
    }
    else {
        std::cerr << "Invalid http protocol\n";
        return "";
    }
}

#ifdef _WIN32
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#endif
