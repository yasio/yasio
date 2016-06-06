//
// main.cpp
// ~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include "thelib/utils/mysqlcc.h"
#include "mysql_client.hpp"
#include "server.hpp"
#include "initd.h"
#include "thelib/utils/random.h"

#include "thelib/utils/xxsocket.h"
#include "http_client.hpp"
//#include <thelib/process_manager.h>
#include "thelib/utils/aes_wrapper.h"
#include "thelib/utils/crypto_wrapper.h"
#include "thelib/utils/xxfsutility.h"
#include "server_logger.hpp"
#include <openssl/objects.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <unordered_map>


using namespace thelib;
using namespace thelib::net;

static char rand_table[] = {
    'a','b','c','d','e','f','g','h',
    'i','j','k','l','m','n','o','p',
    'q','r','s','t','u','v','w','x',
    'y','z','1','2','3','4','5','6',
    '7','8','9','0','A','B','C','D',
    'E','F','G','H','I','J','K','L',
    'M','N','O','P','Q','R','S','T',
    'U','V','W','X','Y','Z'
};

void random_cipher(char buf[32])
{
    for(int i = 0; i < 32; ++i)
    {
        buf[i] = rand_table[rrand(0, sizeof(rand_table))];
    }
}

//#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))

http::server::server* the_server;

static const char* rsa_private_key = "MIICXAIBAAKBgQC9hZyBktXJGlFREJ6M8vJqtAe3hnpr04Nh2l4GMk3pZvijTTojyY7mMhMm5OsD/R8bW7joC7WhRik12wLRwOJjG/sNtyjg0hk05BzRnetxZp0oFggppH/HmCSGYBNm085OPROsk6PDzA/Sq45d531yAZd3s/PrQnYKq4aTmrXylQIDAQABAoGAP20w1Jh8ncIGBthGB6vi/1zi3EPQQrfV13DKWM6LDprciWJ2G7X/8gA+Mp0YHgyfVXub5WTN03x0nEaAqgwgdzFAN3c+rv/iqNNk9JudD5voY6+rZoMDghAjKhk3iiZ2EgX+YETUFaroznDhWVR0k6dwCmw9PXq6Fb643N3gTyECQQDlL7uIjLTWgZPHMhY2kjBgi8OA4FW/JVl3mV+xhfcoGgbQSIw7U5CXvfsbuM7h/0AZmXgl/6Rj1vw3W6uNmzrZAkEA07HnJTBm90oSqBtkaASPXa1kiQcHDI8Xy00KyBJOETlSSnA48mzQUe0BGxgfAifnOqr6Q4y6GFgP+S9to6WIHQJAKLrt6huPe9u1Zp45ImOio1XTXdEAjCLYHpAsWIFFZmQRt+xct6JnPQBvYwLaCYHyY1pJ5v7iuTeYxUHOYDEpKQJAdQdWZyK46WBTrAdonHBY6Uqf13jBFtpMJyGyIiSsb60mpuwfLzWkfIXvJJFTIxf4JmC69Xjor+iO/AySKfOqqQJBAIQYV0/dpvY9VOGJVLW7PVD4V+XRuvWYN8w2zW8VwwPUgqFEqngGZ/9jwTFbgcYECHEKBGwCcXSoSwlFlpsNLdg=";

static void parse_post_params(const std::string& input, std::unordered_map<std::string, std::string>& output)
{
    std::string name;
    std::string value;
    enum {
        param_start,
        param_name,
        param_value,
    } state;

    state = param_start;
    for(std::string::size_type i = 0; i < input.size(); ++i)
    {
        switch(state)
        {
        case param_start:
            if(input[i] == '=' || input[i] == '&')
            {
                return; // error
            }
            else {
                state = param_name;
            }
        case param_name:
            if(input[i] != '=' && input[i] != '&')
            {
                name.push_back(input[i]);
            }
            else if(input[i] == '=')
            {
                state = param_value;
            }
            else if(input[i] == '&') {
                return; // error
            }
            break;
        case param_value:
            if(input[i] != '=' && input[i] != '&')
            {
                value.push_back(input[i]);
            }
            else if(input[i] == '=')
            {
                return; // error
            }
            else if(input[i] == '&')
            {
                output[name] = crypto::http::urldecode(value);
                name.clear();
                value.clear();
                state = param_name;
            }
            
            break;
        }
    }

    if(!name.empty() && !value.empty())
        output[name] = value;
}

std::string sha1(const std::string& content)
{
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, content.c_str(), content.length());
    unsigned char md[20];
    memset(md, 0, sizeof(md));
    SHA1_Final(md, &ctx);
    return std::string((char*)md, 20);
}

std::string sign(const std::string& message)
{
    managed_cstring privkey = fsutil::read_file_data("rsa_private_key.pem");
    
    bool succeed = false;
    RSA *priv_ctx;

    BIO *priv_bio;

    priv_bio = BIO_new_mem_buf((void*)privkey.c_str(), -1);
    priv_ctx = PEM_read_bio_RSAPrivateKey(priv_bio, NULL, NULL, NULL);
    unsigned char* signature = (unsigned char*)malloc(RSA_size(priv_ctx));
    unsigned int slen = 0;
    succeed = RSA_sign(NID_sha1WithRSA, (unsigned char*)message.c_str(), message.size(), signature, &slen, priv_ctx);

    std::string ret((char*)signature, slen);

    free(signature);
    return std::move(ret);
}

bool verify(const std::string& message, const std::string& signature)
{
    managed_cstring pubkey = fsutil::read_file_data("rsa_public_key.pem");
    BIO* pub_bio;
    RSA* pub_ctx;

    pub_bio = BIO_new_mem_buf((void*)pubkey.c_str(), -1);
    if(pub_bio == nullptr)
        return false;
    pub_ctx = PEM_read_bio_RSA_PUBKEY(pub_bio, NULL, NULL, NULL);
    if(pub_ctx == nullptr)
        return false;

    bool verified = RSA_verify(NID_sha1WithRSA, (unsigned char*)message.c_str(), message.size(), (unsigned char*)signature.c_str(), signature.size(), pub_ctx);

    free(pub_ctx);
    return verified;
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    enable_leak_check();
#else
    sinitd();
#endif

    LOG_WARN_ALL("program bit detect...");
    if(sizeof(void*) == 8) {
        LOG_WARN_ALL("this is a x86_64 program.");
    }
    else {
        LOG_WARN_ALL("this is a x86_32 program.");
    }

    cJSON_thread_init();

    /// 

    std::string stream = "a=13&b=19&trade_no=13334335&time=199838";
    std::unordered_map<std::string, std::string> post;
    parse_post_params(stream, post);


    ///////////////////
    ///////////////////

    //int ret = 0;

    //RSA* r = RSA_new();

    //managed_cstring privkey = fsutil::read_file_data("rsa_private_key.pem");
    //managed_cstring pubkey = fsutil::read_file_data("rsa_public_key.pem");
    //
    ////PEM_read_bio_RSA_PUBKEY(
    //// ret=RSA_generate_key_ex(r,bits,bne,NULL);
    //bool succeed = false;
    //RSA *priv_ctx;
    //RSA *pub_ctx;

    //BIO *priv_bio;
    //BIO *pub_bio;

    //priv_bio = BIO_new_mem_buf((void*)privkey.c_str(), -1);
    //priv_ctx = PEM_read_bio_RSAPrivateKey(priv_bio, NULL, NULL, NULL);
    //unsigned char* signature = (unsigned char*)malloc(RSA_size(priv_ctx));
    //unsigned int slen = 0;
    //succeed = RSA_sign(NID_sha1WithRSA, (unsigned char*)"hello world", sizeof("hello world") - 1, signature, &slen, priv_ctx);
    //

    //pub_bio = BIO_new_mem_buf((void*)pubkey.c_str(), -1);
    //pub_ctx = PEM_read_bio_RSA_PUBKEY(pub_bio, NULL, NULL, NULL);

    //succeed = RSA_verify(NID_sha1WithRSA, (unsigned char*)"hello world", sizeof("hello world") - 1, signature, slen, pub_ctx);

    
    ///////////////////
    ///////////////////
    
    try
    {
        // Check command line arguments.

        const char* address = "0.0.0.0";
        const char* port = "8888";

        if(argc >= 2) {
            address = argv[1];
        }

        if(argc >= 3) {
            port = argv[2];
        }

        // CR_SERVER_LOST(2013) CR_SERVER_GONE_AWAY(2006) CR_PRIMARY_CONFLICIT(1062)

        // Initialise the server.

        LOG_WARN_ALL("listen at port:[%s]", port);
        http::server::server s(address, port, "");

        the_server = &s;

        // Run the server until stopped.
        s.start(16);
    }
    catch (std::exception& e)
    {
        LOG_ERROR_ALL("exception:", e.what());
        // std::cerr << "exception: " << e.what() << "\n";
    }

    cJSON_thread_end();
    return 0;
}

