//////////////////////////////////////////////////////////////////////////////////////////
// A multi-platform support c++11 library with focus on asynchronous socket I/O for any
// client application.
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2023 HALX99

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

#ifndef YASIO__SSL_HPP
#define YASIO__SSL_HPP

#include "yasio/detail/config.hpp"

#if YASIO_SSL_BACKEND == 1 // OpenSSL
#  include <openssl/bio.h>
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#elif YASIO_SSL_BACKEND == 2 // mbedtls
#  define MBEDTLS_ALLOW_PRIVATE_ACCESS
#  include "mbedtls/net_sockets.h"
#  include "mbedtls/debug.h"
#  include "mbedtls/ssl.h"
#  include "mbedtls/entropy.h"
#  include "mbedtls/ctr_drbg.h"
#  include "mbedtls/error.h"
struct ssl_ctx_st {
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_x509_crt cert;
  mbedtls_pk_context pkey;
  mbedtls_ssl_config conf;
};
struct ssl_st : public mbedtls_ssl_context {
  mbedtls_net_context bio;
};
#endif

#if defined(YASIO_SSL_BACKEND)
struct yssl_options {
  const char* crtfile_;
  const char* keyfile_;
  bool client;
};

YASIO__DECL ssl_ctx_st* yssl_ctx_new(const yssl_options& opts);
YASIO__DECL void yssl_ctx_free(ssl_ctx_st*& ctx);

YASIO__DECL ssl_st* yssl_new(ssl_ctx_st* ctx, int fd, const char* hostname, bool client);
YASIO__DECL void yssl_shutdown(ssl_st*&);

#endif

#if defined(YASIO_HEADER_ONLY)
#  include "yasio/ssl.cpp" // lgtm [cpp/include-non-header]
#endif

#endif
