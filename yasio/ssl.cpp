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

#ifndef YASIO__SSL_CPP
#define YASIO__SSL_CPP
#if !defined(YASIO_HEADER_ONLY)
#  include "yasio/ssl.hpp"
#endif

#include "yasio/detail/socket.hpp"
#include "yasio/detail/logging.hpp"
#include "yasio/cxx17/string_view.hpp"

#define yasio__valid_str(str) (str && *str)

#if YASIO_SSL_BACKEND == 1 // OpenSSL
YASIO__DECL ssl_ctx_st* yasio___ssl_ctx_new(yasio__ssl_options& options, ssl_ctx_st* share)
{
  if (share)
    return share;
  auto ctx = ::SSL_CTX_new(::SSLv23_method());

#  if defined(SSL_MODE_RELEASE_BUFFERS)
  ::SSL_CTX_set_mode(ctx, SSL_MODE_RELEASE_BUFFERS);
#  endif

  ::SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
  if (yasio__valid_str(options.cafile_))
  {
    if (::SSL_CTX_load_verify_locations(ctx, options.cafile_, nullptr) == 1)
    {
      ::SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, ::SSL_CTX_get_verify_callback(ctx));
#  if OPENSSL_VERSION_NUMBER >= 0x10101000L
      ::SSL_CTX_set_post_handshake_auth(ctx, 1);
#  endif
#  if defined(X509_V_FLAG_PARTIAL_CHAIN)
      /* Have intermediate certificates in the trust store be treated as
         trust-anchors, in the same way as self-signed root CA certificates
         are. This allows users to verify servers using the intermediate cert
         only, instead of needing the whole chain. */
      X509_STORE_set_flags(SSL_CTX_get_cert_store(ctx), X509_V_FLAG_PARTIAL_CHAIN);
#  endif
    }
    else
      YASIO_LOG("[global] load ca certifaction file failed!");
  }
  else
    SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);

  if (yasio__valid_str(options.crtfile_) && ::SSL_CTX_use_certificate_file(ctx, options.crtfile_, SSL_FILETYPE_PEM) <= 0)
    YASIO_LOG("[gobal] load server cert file failed!");

  if (yasio__valid_str(options.keyfile_) && ::SSL_CTX_use_PrivateKey_file(ctx, options.keyfile_, SSL_FILETYPE_PEM) <= 0)
    YASIO_LOG("[gobal] load server private key file failed!");

  return ctx;
}

YASIO__DECL void yasio__ssl_ctx_free(ssl_ctx_st*& ctx)
{
  ::SSL_CTX_free((SSL_CTX*)ctx);
  ctx = nullptr;
}
YASIO__DECL ssl_st* yasio__ssl_new(ssl_ctx_st* ctx, int fd, const char* hostname, bool client)
{
  auto ssl = ::SSL_new(ctx);
  ::SSL_set_fd(ssl, fd);
  if (client)
  {
    ::SSL_set_connect_state(ssl);
    ::SSL_set_tlsext_host_name(ssl, hostname);
  }
  else
    ::SSL_set_accept_state(ssl);
  return ssl;
}
YASIO__DECL void yasio__ssl_shutdown(ssl_st*& ssl)
{
  ::SSL_shutdown(ssl);
  ::SSL_free(ssl);
  ssl = nullptr;
}
#elif YASIO_SSL_BACKEND == 2 // mbedtls
YASIO__DECL ssl_ctx_st* yasio___ssl_ctx_new(yasio__ssl_options& options, ssl_ctx_st* /*share*/)
{
  auto ctx = new ssl_ctx_st();
  ::mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
  ::mbedtls_entropy_init(&ctx->entropy);
  ::mbedtls_ssl_config_init(&ctx->conf);
  ::mbedtls_x509_crt_init(&ctx->cacert);
  ::mbedtls_x509_crt_init(&ctx->cert);
  ::mbedtls_pk_init(&ctx->pkey);

  do
  {
    // drbg seed
    cxx17::string_view pers = options.client ? cxx17::string_view{YASIO_SSL_PIN, YASIO_SSL_PIN_LEN} : cxx17::string_view{YASIO_SSL_PON, YASIO_SSL_PON_LEN};
    int ret                 = ::mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, ::mbedtls_entropy_func, &ctx->entropy, (const unsigned char*)pers.data(), pers.length());
    if (ret != 0)
    {
      YASIO_LOG("mbedtls_ctr_drbg_seed fail with ret=%d", ret);
      break;
    }

    // rgn engine
    ::mbedtls_ssl_conf_rng(&ctx->conf, ::mbedtls_ctr_drbg_random, &ctx->ctr_drbg);

    // ssl role
    if ((ret = ::mbedtls_ssl_config_defaults(&ctx->conf, options.client ? MBEDTLS_SSL_IS_CLIENT : MBEDTLS_SSL_IS_SERVER, MBEDTLS_SSL_TRANSPORT_STREAM,
                                             MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
      YASIO_LOG("mbedtls_ssl_config_defaults fail with ret=%d", ret);

    // --- load client cert
    int authmode = MBEDTLS_SSL_VERIFY_OPTIONAL;
    if (yasio__valid_str(options.cafile_)) // the cafile_ must be full path
    {
      if ((ret = ::mbedtls_x509_crt_parse_file(&ctx->cacert, options.cafile_)) == 0)
        authmode = MBEDTLS_SSL_VERIFY_REQUIRED;
      else
      {
        YASIO_LOG("mbedtls_x509_crt_parse_file with ret=-0x%x", (unsigned int)-ret);
        break;
      }
    }

    ::mbedtls_ssl_conf_authmode(&ctx->conf, authmode);
    ::mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, nullptr);

    // --- load server cert and private key
    // server cert
    if (yasio__valid_str(options.crtfile_) && (ret = ::mbedtls_x509_crt_parse_file(&ctx->cert, options.crtfile_)) != 0)
    {
      YASIO_LOG("mbedtls_x509_crt_parse_file with ret=-0x%x", (unsigned int)-ret);
      break;
    }
    if (yasio__valid_str(options.keyfile_) &&
        (ret = ::mbedtls_pk_parse_keyfile(&ctx->pkey, options.keyfile_, nullptr, mbedtls_ctr_drbg_random, &ctx->ctr_drbg)) != 0)
    {
      YASIO_LOG("mbedtls_x509_crt_parse_file with ret=-0x%x", (unsigned int)-ret);
      break;
    }
    mbedtls_ssl_conf_ca_chain(&ctx->conf, ctx->cert.next, NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->cert, &ctx->pkey)) != 0)
    {
      YASIO_LOG("mbedtls_ssl_conf_own_cert with ret=-0x%x", (unsigned int)-ret);
      break;
    }

    return ctx;
  } while (false);

  yasio__ssl_ctx_free(ctx);

  return nullptr;
}

YASIO__DECL void yasio__ssl_ctx_free(ssl_ctx_st*& ctx)
{
  if (!ctx)
    return;
  ::mbedtls_pk_free(&ctx->pkey);
  ::mbedtls_x509_crt_free(&ctx->cert);
  ::mbedtls_x509_crt_free(&ctx->cacert);
  ::mbedtls_ssl_config_free(&ctx->conf);
  ::mbedtls_entropy_free(&ctx->entropy);
  ::mbedtls_ctr_drbg_free(&ctx->ctr_drbg);

  delete ctx;
  ctx = nullptr;
}

YASIO__DECL ssl_st* yasio__ssl_new(ssl_ctx_st* ctx, int fd, const char* hostname, bool client)
{
  auto ssl = new ssl_st();
  ::mbedtls_ssl_init(ssl);
  ::mbedtls_ssl_setup(ssl, &ctx->conf);

  // ssl_set_fd
  ssl->bio.fd = fd;
  ::mbedtls_ssl_set_bio(ssl, &ssl->bio, ::mbedtls_net_send, ::mbedtls_net_recv, nullptr /*  rev_timeout() */);
  ::mbedtls_ssl_set_hostname(ssl, hostname);
  return ssl;
}
YASIO__DECL void yasio__ssl_shutdown(ssl_st*& ssl)
{
  ::mbedtls_ssl_close_notify(ssl);
  ::mbedtls_ssl_free(ssl);
  delete ssl;
  ssl = nullptr;
}
#else
#  error "yasio - Unsupported ssl backend provided!"
#endif

#endif
