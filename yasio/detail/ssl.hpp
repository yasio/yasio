//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store universal app
//
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

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

#if YASIO_SSL_BACKEND == 1 // OpenSSL
#  include <openssl/bio.h>
#  include <openssl/ssl.h>
#  include <openssl/err.h>
#elif YASIO_SSL_BACKEND == 2 // mbedtls
#  include "mbedtls/net_sockets.h"
#  include "mbedtls/debug.h"
#  include "mbedtls/ssl.h"
#  include "mbedtls/entropy.h"
#  include "mbedtls/ctr_drbg.h"
#  include "mbedtls/error.h"
#  include "mbedtls/certs.h"
struct ssl_ctx_st {
  mbedtls_ctr_drbg_context ctr_drbg;
  mbedtls_entropy_context entropy;
  mbedtls_x509_crt cacert;
  mbedtls_ssl_config conf;
};
struct ssl_st {
  mbedtls_ssl_context impl;
  mbedtls_net_context bio;
};
#else
#  error "yasio - Unsupported ssl backend provided!"
#endif

#endif
