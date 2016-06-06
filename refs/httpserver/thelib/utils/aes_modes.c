/* ====================================================================
 * Copyright (c) 2008 The OpenSSL Project. All rights reserved.
 *
 * Rights for redistribution and usage in source and binary
 * forms are granted according to the OpenSSL license.
 */
// aes modes.h
#include <stddef.h>
typedef void (*block128_f)(const unsigned char in[16],
			unsigned char out[16],
			const void *key);

typedef void (*cbc128_f)(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], int enc);

typedef void (*ctr128_f)(const unsigned char *in, unsigned char *out,
			size_t blocks, const void *key,
			const unsigned char ivec[16]);

typedef void (*ccm128_f)(const unsigned char *in, unsigned char *out,
			size_t blocks, const void *key,
			const unsigned char ivec[16],unsigned char cmac[16]);

void ossl_crypto_cbc128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);
void ossl_crypto_cbc128_decrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);

void ossl_crypto_ctr128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], unsigned char ecount_buf[16],
			unsigned int *num, block128_f block);

void ossl_crypto_ctr128_encrypt_ctr32(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], unsigned char ecount_buf[16],
			unsigned int *num, ctr128_f ctr);

void ossl_crypto_ofb128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], int *num,
			block128_f block);

void ossl_crypto_cfb128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block);
void ossl_crypto_cfb128_8_encrypt(const unsigned char *in, unsigned char *out,
			size_t length, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block);
void ossl_crypto_cfb128_1_encrypt(const unsigned char *in, unsigned char *out,
			size_t bits, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block);

size_t ossl_crypto_cts128_encrypt_block(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);
size_t ossl_crypto_cts128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], cbc128_f cbc);
size_t ossl_crypto_cts128_decrypt_block(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);
size_t ossl_crypto_cts128_decrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], cbc128_f cbc);

size_t ossl_crypto_nistcts128_encrypt_block(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);
size_t ossl_crypto_nistcts128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], cbc128_f cbc);
size_t ossl_crypto_nistcts128_decrypt_block(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block);
size_t ossl_crypto_nistcts128_decrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], cbc128_f cbc);

typedef struct gcm128_context GCM128_CONTEXT;

GCM128_CONTEXT *ossl_crypto_gcm128_new(void *key, block128_f block);
void ossl_crypto_gcm128_init(GCM128_CONTEXT *ctx,void *key,block128_f block);
void ossl_crypto_gcm128_setiv(GCM128_CONTEXT *ctx, const unsigned char *iv,
			size_t len);
int ossl_crypto_gcm128_aad(GCM128_CONTEXT *ctx, const unsigned char *aad,
			size_t len);
int ossl_crypto_gcm128_encrypt(GCM128_CONTEXT *ctx,
			const unsigned char *in, unsigned char *out,
			size_t len);
int ossl_crypto_gcm128_decrypt(GCM128_CONTEXT *ctx,
			const unsigned char *in, unsigned char *out,
			size_t len);
int ossl_crypto_gcm128_encrypt_ctr32(GCM128_CONTEXT *ctx,
			const unsigned char *in, unsigned char *out,
			size_t len, ctr128_f stream);
int ossl_crypto_gcm128_decrypt_ctr32(GCM128_CONTEXT *ctx,
			const unsigned char *in, unsigned char *out,
			size_t len, ctr128_f stream);
int ossl_crypto_gcm128_finish(GCM128_CONTEXT *ctx,const unsigned char *tag,
			size_t len);
void ossl_crypto_gcm128_tag(GCM128_CONTEXT *ctx, unsigned char *tag, size_t len);
void ossl_crypto_gcm128_release(GCM128_CONTEXT *ctx);

typedef struct ccm128_context CCM128_CONTEXT;

void ossl_crypto_ccm128_init(CCM128_CONTEXT *ctx,
	unsigned int M, unsigned int L, void *key,block128_f block);
int ossl_crypto_ccm128_setiv(CCM128_CONTEXT *ctx,
	const unsigned char *nonce, size_t nlen, size_t mlen);
void ossl_crypto_ccm128_aad(CCM128_CONTEXT *ctx,
	const unsigned char *aad, size_t alen);
int ossl_crypto_ccm128_encrypt(CCM128_CONTEXT *ctx,
	const unsigned char *inp, unsigned char *out, size_t len);
int ossl_crypto_ccm128_decrypt(CCM128_CONTEXT *ctx,
	const unsigned char *inp, unsigned char *out, size_t len);
int ossl_crypto_ccm128_encrypt_ccm64(CCM128_CONTEXT *ctx,
	const unsigned char *inp, unsigned char *out, size_t len,
	ccm128_f stream);
int ossl_crypto_ccm128_decrypt_ccm64(CCM128_CONTEXT *ctx,
	const unsigned char *inp, unsigned char *out, size_t len,
	ccm128_f stream);
size_t ossl_crypto_ccm128_tag(CCM128_CONTEXT *ctx, unsigned char *tag, size_t len);

typedef struct xts128_context XTS128_CONTEXT;

int ossl_crypto_xts128_encrypt(const XTS128_CONTEXT *ctx, const unsigned char iv[16],
	const unsigned char *inp, unsigned char *out, size_t len, int enc);


/* crypto/aes/aes_ecb.c -*- mode:C; c-file-style: "eay" -*- */
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#ifndef AES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

#include "aes.h"
#include "aes_locl.h"

// ECB copy from Aes_ecb.c
void osl_aes_ecb_encrypt(const unsigned char *in, unsigned char *out,
		     const AES_KEY *key, const int enc) 
{

    assert(in && out && key);
    assert((AES_ENCRYPT == enc)||(AES_DECRYPT == enc));

	if (AES_ENCRYPT == enc)
        ossl_aes_encrypt(in, out, key);
	else
        ossl_aes_decrypt(in, out, key);
}

// CBC copy from Aes_cbc.c
void ossl_aes_cbc_encrypt(const unsigned char *in, unsigned char *out,
		     size_t len, const AES_KEY *key,
		     unsigned char *ivec, const int enc) {

	if (enc)
		ossl_crypto_cbc128_encrypt(in,out,len,key,ivec,(block128_f)ossl_aes_encrypt);
	else
		ossl_crypto_cbc128_decrypt(in,out,len,key,ivec,(block128_f)ossl_aes_decrypt);
}

// CFB copy from Aes_cfb.c
/* The input and output encrypted as though 128bit cfb mode is being
 * used.  The extra state information to record how much of the
 * 128bit block we have used is contained in *num;
 */

void ossl_aes_cfb128_encrypt(const unsigned char *in, unsigned char *out,
	size_t length, const AES_KEY *key,
	unsigned char *ivec, int *num, const int enc) {

	ossl_crypto_cfb128_encrypt(in,out,length,key,ivec,num,enc,(block128_f)ossl_aes_encrypt);
}

/* N.B. This expects the input to be packed, MS bit first */
void ossl_aes_cfb1_encrypt(const unsigned char *in, unsigned char *out,
		      size_t length, const AES_KEY *key,
		      unsigned char *ivec, int *num, const int enc)
    {
    ossl_crypto_cfb128_1_encrypt(in,out,length,key,ivec,num,enc,(block128_f)ossl_aes_encrypt);
    }

void ossl_aes_cfb8_encrypt(const unsigned char *in, unsigned char *out,
		      size_t length, const AES_KEY *key,
		      unsigned char *ivec, int *num, const int enc)
    {
    ossl_crypto_cfb128_8_encrypt(in,out,length,key,ivec,num,enc,(block128_f)ossl_aes_encrypt);
    }

// IGE copy from Aes_ige.c

#define N_WORDS (AES_BLOCK_SIZE / sizeof(unsigned long))
typedef struct {
        unsigned long data[N_WORDS];
} aes_block_t;

/* XXX: probably some better way to do this */
#if defined(__i386__) || defined(__x86_64__)
#define UNALIGNED_MEMOPS_ARE_FAST 1
#else
#define UNALIGNED_MEMOPS_ARE_FAST 0
#endif

#if UNALIGNED_MEMOPS_ARE_FAST
#define load_block(d, s)        (d) = *(const aes_block_t *)(s)
#define store_block(d, s)       *(aes_block_t *)(d) = (s)
#else
#define load_block(d, s)        memcpy((d).data, (s), AES_BLOCK_SIZE)
#define store_block(d, s)       memcpy((d), (s).data, AES_BLOCK_SIZE)
#endif

/* N.B. The IV for this mode is _twice_ the block size */

void ossl_aes_ige_encrypt(const unsigned char *in, unsigned char *out,
					 size_t length, const AES_KEY *key,
					 unsigned char *ivec, const int enc)
	{
	size_t n;
	size_t len = length;

	assert(in && out && key && ivec);
	assert((AES_ENCRYPT == enc)||(AES_DECRYPT == enc));
	assert((length%AES_BLOCK_SIZE) == 0);

	len = length / AES_BLOCK_SIZE;

	if (AES_ENCRYPT == enc)
		{
		if (in != out &&
		    (UNALIGNED_MEMOPS_ARE_FAST || ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(long)==0))
			{
			aes_block_t *ivp = (aes_block_t *)ivec;
			aes_block_t *iv2p = (aes_block_t *)(ivec + AES_BLOCK_SIZE);

			while (len)
				{
				aes_block_t *inp = (aes_block_t *)in;
				aes_block_t *outp = (aes_block_t *)out;

				for(n=0 ; n < N_WORDS; ++n)
					outp->data[n] = inp->data[n] ^ ivp->data[n];
                ossl_aes_encrypt((unsigned char *)outp->data, (unsigned char *)outp->data, key);
				for(n=0 ; n < N_WORDS; ++n)
					outp->data[n] ^= iv2p->data[n];
				ivp = outp;
				iv2p = inp;
				--len;
				in += AES_BLOCK_SIZE;
				out += AES_BLOCK_SIZE;
				}
			memcpy(ivec, ivp->data, AES_BLOCK_SIZE);
			memcpy(ivec + AES_BLOCK_SIZE, iv2p->data, AES_BLOCK_SIZE);
			}
		else
			{
			aes_block_t tmp, tmp2;
			aes_block_t iv;
			aes_block_t iv2;

			load_block(iv, ivec);
			load_block(iv2, ivec + AES_BLOCK_SIZE);

			while (len)
				{
				load_block(tmp, in);
				for(n=0 ; n < N_WORDS; ++n)
					tmp2.data[n] = tmp.data[n] ^ iv.data[n];
				ossl_aes_encrypt((unsigned char *)tmp2.data, (unsigned char *)tmp2.data, key);
				for(n=0 ; n < N_WORDS; ++n)
					tmp2.data[n] ^= iv2.data[n];
				store_block(out, tmp2);
				iv = tmp2;
				iv2 = tmp;
				--len;
				in += AES_BLOCK_SIZE;
				out += AES_BLOCK_SIZE;
				}
			memcpy(ivec, iv.data, AES_BLOCK_SIZE);
			memcpy(ivec + AES_BLOCK_SIZE, iv2.data, AES_BLOCK_SIZE);
			}
		}
	else
		{
		if (in != out &&
		    (UNALIGNED_MEMOPS_ARE_FAST || ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(long)==0))
			{
			aes_block_t *ivp = (aes_block_t *)ivec;
			aes_block_t *iv2p = (aes_block_t *)(ivec + AES_BLOCK_SIZE);

			while (len)
				{
				aes_block_t tmp;
				aes_block_t *inp = (aes_block_t *)in;
				aes_block_t *outp = (aes_block_t *)out;

				for(n=0 ; n < N_WORDS; ++n)
					tmp.data[n] = inp->data[n] ^ iv2p->data[n];
                ossl_aes_decrypt((unsigned char *)tmp.data, (unsigned char *)outp->data, key);
				for(n=0 ; n < N_WORDS; ++n)
					outp->data[n] ^= ivp->data[n];
				ivp = inp;
				iv2p = outp;
				--len;
				in += AES_BLOCK_SIZE;
				out += AES_BLOCK_SIZE;
				}
			memcpy(ivec, ivp->data, AES_BLOCK_SIZE);
			memcpy(ivec + AES_BLOCK_SIZE, iv2p->data, AES_BLOCK_SIZE);
			}
		else
			{
			aes_block_t tmp, tmp2;
			aes_block_t iv;
			aes_block_t iv2;

			load_block(iv, ivec);
			load_block(iv2, ivec + AES_BLOCK_SIZE);

			while (len)
				{
				load_block(tmp, in);
				tmp2 = tmp;
				for(n=0 ; n < N_WORDS; ++n)
					tmp.data[n] ^= iv2.data[n];
				ossl_aes_decrypt((unsigned char *)tmp.data, (unsigned char *)tmp.data, key);
				for(n=0 ; n < N_WORDS; ++n)
					tmp.data[n] ^= iv.data[n];
				store_block(out, tmp);
				iv = tmp2;
				iv2 = tmp;
				--len;
				in += AES_BLOCK_SIZE;
				out += AES_BLOCK_SIZE;
				}
			memcpy(ivec, iv.data, AES_BLOCK_SIZE);
			memcpy(ivec + AES_BLOCK_SIZE, iv2.data, AES_BLOCK_SIZE);
			}
		}
	}

/*
 * Note that its effectively impossible to do biIGE in anything other
 * than a single pass, so no provision is made for chaining.
 */

/* N.B. The IV for this mode is _four times_ the block size */

void ossl_aes_bi_ige_encrypt(const unsigned char *in, unsigned char *out,
						size_t length, const AES_KEY *key,
						const AES_KEY *key2, const unsigned char *ivec,
						const int enc)
	{
	size_t n;
	size_t len = length;
	unsigned char tmp[AES_BLOCK_SIZE];
	unsigned char tmp2[AES_BLOCK_SIZE];
	unsigned char tmp3[AES_BLOCK_SIZE];
	unsigned char prev[AES_BLOCK_SIZE];
	const unsigned char *iv;
	const unsigned char *iv2;

	assert(in && out && key && ivec);
	assert((AES_ENCRYPT == enc)||(AES_DECRYPT == enc));
	assert((length%AES_BLOCK_SIZE) == 0);

	if (AES_ENCRYPT == enc)
		{
		/* XXX: Do a separate case for when in != out (strictly should
		   check for overlap, too) */

		/* First the forward pass */ 
		iv = ivec;
		iv2 = ivec + AES_BLOCK_SIZE;
		while (len >= AES_BLOCK_SIZE)
			{
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] = in[n] ^ iv[n];
            ossl_aes_encrypt(out, out, key);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] ^= iv2[n];
			iv = out;
			memcpy(prev, in, AES_BLOCK_SIZE);
			iv2 = prev;
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
			}

		/* And now backwards */
		iv = ivec + AES_BLOCK_SIZE*2;
		iv2 = ivec + AES_BLOCK_SIZE*3;
		len = length;
		while(len >= AES_BLOCK_SIZE)
			{
			out -= AES_BLOCK_SIZE;
			/* XXX: reduce copies by alternating between buffers */
			memcpy(tmp, out, AES_BLOCK_SIZE);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] ^= iv[n];
			/*			hexdump(stdout, "out ^ iv", out, AES_BLOCK_SIZE); */
			ossl_aes_encrypt(out, out, key);
			/*			hexdump(stdout,"enc", out, AES_BLOCK_SIZE); */
			/*			hexdump(stdout,"iv2", iv2, AES_BLOCK_SIZE); */
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] ^= iv2[n];
			/*			hexdump(stdout,"out", out, AES_BLOCK_SIZE); */
			iv = out;
			memcpy(prev, tmp, AES_BLOCK_SIZE);
			iv2 = prev;
			len -= AES_BLOCK_SIZE;
			}
		}
	else
		{
		/* First backwards */
		iv = ivec + AES_BLOCK_SIZE*2;
		iv2 = ivec + AES_BLOCK_SIZE*3;
		in += length;
		out += length;
		while (len >= AES_BLOCK_SIZE)
			{
			in -= AES_BLOCK_SIZE;
			out -= AES_BLOCK_SIZE;
			memcpy(tmp, in, AES_BLOCK_SIZE);
			memcpy(tmp2, in, AES_BLOCK_SIZE);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				tmp[n] ^= iv2[n];
			ossl_aes_decrypt(tmp, out, key);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] ^= iv[n];
			memcpy(tmp3, tmp2, AES_BLOCK_SIZE);
			iv = tmp3;
			iv2 = out;
			len -= AES_BLOCK_SIZE;
			}

		/* And now forwards */
		iv = ivec;
		iv2 = ivec + AES_BLOCK_SIZE;
		len = length;
		while (len >= AES_BLOCK_SIZE)
			{
			memcpy(tmp, out, AES_BLOCK_SIZE);
			memcpy(tmp2, out, AES_BLOCK_SIZE);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				tmp[n] ^= iv2[n];
			ossl_aes_decrypt(tmp, out, key);
			for(n=0 ; n < AES_BLOCK_SIZE ; ++n)
				out[n] ^= iv[n];
			memcpy(tmp3, tmp2, AES_BLOCK_SIZE);
			iv = tmp3;
			iv2 = out;
			len -= AES_BLOCK_SIZE;
			in += AES_BLOCK_SIZE;
			out += AES_BLOCK_SIZE;
			}
		}
	}


////////////////////////////// implementions ///////////////////////////
/// copy from cbc128.c
#ifndef MODES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

// define STRICT_ALIGNMENT 1

#ifndef STRICT_ALIGNMENT
#  define STRICT_ALIGNMENT 0
#endif

void ossl_crypto_cbc128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block)
{
	size_t n;
	const unsigned char *iv = ivec;

	assert(in && out && key && ivec);

#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (STRICT_ALIGNMENT &&
	    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
		while (len>=16) {
			for(n=0; n<16; ++n)
				out[n] = in[n] ^ iv[n];
			(*block)(out, out, key);
			iv = out;
			len -= 16;
			in  += 16;
			out += 16;
		}
	} else {
		while (len>=16) {
			for(n=0; n<16; n+=sizeof(size_t))
				*(size_t*)(out+n) =
				*(size_t*)(in+n) ^ *(size_t*)(iv+n);
			(*block)(out, out, key);
			iv = out;
			len -= 16;
			in  += 16;
			out += 16;
		}
	}
#endif
	while (len) {
		for(n=0; n<16 && n<len; ++n)
			out[n] = in[n] ^ iv[n];
		for(; n<16; ++n)
			out[n] = iv[n];
		(*block)(out, out, key);
		iv = out;
		if (len<=16) break;
		len -= 16;
		in  += 16;
		out += 16;
	}
	memcpy(ivec,iv,16);
}

void ossl_crypto_cbc128_decrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], block128_f block)
{
	size_t n;
	union { size_t align; unsigned char c[16]; } tmp;

	assert(in && out && key && ivec);

#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (in != out) {
		const unsigned char *iv = ivec;

		if (STRICT_ALIGNMENT &&
		    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
			while (len>=16) {
				(*block)(in, out, key);
				for(n=0; n<16; ++n)
					out[n] ^= iv[n];
				iv = in;
				len -= 16;
				in  += 16;
				out += 16;
			}
		}
		else {
			while (len>=16) {
				(*block)(in, out, key);
				for(n=0; n<16; n+=sizeof(size_t))
					*(size_t *)(out+n) ^= *(size_t *)(iv+n);
				iv = in;
				len -= 16;
				in  += 16;
				out += 16;
			}
		}
		memcpy(ivec,iv,16);
	} else {
		if (STRICT_ALIGNMENT &&
		    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
			unsigned char c;
			while (len>=16) {
				(*block)(in, tmp.c, key);
				for(n=0; n<16; ++n) {
					c = in[n];
					out[n] = tmp.c[n] ^ ivec[n];
					ivec[n] = c;
				}
				len -= 16;
				in  += 16;
				out += 16;
			}
		}
		else {
			size_t c;
			while (len>=16) {
				(*block)(in, tmp.c, key);
				for(n=0; n<16; n+=sizeof(size_t)) {
					c = *(size_t *)(in+n);
					*(size_t *)(out+n) =
					*(size_t *)(tmp.c+n) ^ *(size_t *)(ivec+n);
					*(size_t *)(ivec+n) = c;
				}
				len -= 16;
				in  += 16;
				out += 16;
			}
		}
	}
#endif
	while (len) {
		unsigned char c;
		(*block)(in, tmp.c, key);
		for(n=0; n<16 && n<len; ++n) {
			c = in[n];
			out[n] = tmp.c[n] ^ ivec[n];
			ivec[n] = c;
		}
		if (len<=16) {
			for (; n<16; ++n)
				ivec[n] = in[n];
			break;
		}
		len -= 16;
		in  += 16;
		out += 16;
	}
}

/// copy from cfb128.c

#ifndef MODES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

/* The input and output encrypted as though 128bit cfb mode is being
 * used.  The extra state information to record how much of the
 * 128bit block we have used is contained in *num;
 */
void ossl_crypto_cfb128_encrypt(const unsigned char *in, unsigned char *out,
			size_t len, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block)
{
    unsigned int n;
    size_t l = 0;

    assert(in && out && key && ivec && num);

    n = *num;

    if (enc) {
#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (16%sizeof(size_t) == 0) do {	/* always true actually */
		while (n && len) {
			*(out++) = ivec[n] ^= *(in++);
			--len;
			n = (n+1) % 16;
		}
#if defined(STRICT_ALIGNMENT)
		if (((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0)
			break;
#endif
		while (len>=16) {
			(*block)(ivec, ivec, key);
			for (; n<16; n+=sizeof(size_t)) {
				*(size_t*)(out+n) =
				*(size_t*)(ivec+n) ^= *(size_t*)(in+n);
			}
			len -= 16;
			out += 16;
			in  += 16;
			n = 0;
		}
		if (len) {
			(*block)(ivec, ivec, key);
			while (len--) {
				out[n] = ivec[n] ^= in[n];
				++n;
			}
		}
		*num = n;
		return;
	} while (0);
	/* the rest would be commonly eliminated by x86* compiler */
#endif
	while (l<len) {
		if (n == 0) {
			(*block)(ivec, ivec, key);
		}
		out[l] = ivec[n] ^= in[l];
		++l;
		n = (n+1) % 16;
	}
	*num = n;
    } else {
#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (16%sizeof(size_t) == 0) do {	/* always true actually */
		while (n && len) {
			unsigned char c;
			*(out++) = ivec[n] ^ (c = *(in++)); ivec[n] = c;
			--len;
			n = (n+1) % 16;
 		}
#if defined(STRICT_ALIGNMENT)
		if (((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0)
			break;
#endif
		while (len>=16) {
			(*block)(ivec, ivec, key);
			for (; n<16; n+=sizeof(size_t)) {
				size_t t = *(size_t*)(in+n);
				*(size_t*)(out+n) = *(size_t*)(ivec+n) ^ t;
				*(size_t*)(ivec+n) = t;
			}
			len -= 16;
			out += 16;
			in  += 16;
			n = 0;
		}
		if (len) {
			(*block)(ivec, ivec, key);
			while (len--) {
				unsigned char c;
				out[n] = ivec[n] ^ (c = in[n]); ivec[n] = c;
				++n;
			}
 		}
		*num = n;
		return;
	} while (0);
	/* the rest would be commonly eliminated by x86* compiler */
#endif
	while (l<len) {
		unsigned char c;
		if (n == 0) {
			(*block)(ivec, ivec, key);
		}
		out[l] = ivec[n] ^ (c = in[l]); ivec[n] = c;
		++l;
		n = (n+1) % 16;
	}
	*num=n;
    }
}

/* This expects a single block of size nbits for both in and out. Note that
   it corrupts any extra bits in the last byte of out */
static void cfbr_encrypt_block(const unsigned char *in,unsigned char *out,
			    int nbits,const void *key,
			    unsigned char ivec[16],int enc,
			    block128_f block)
{
    int n,rem,num;
    unsigned char ovec[16*2 + 1];  /* +1 because we dererefence (but don't use) one byte off the end */

    if (nbits<=0 || nbits>128) return;

	/* fill in the first half of the new IV with the current IV */
	memcpy(ovec,ivec,16);
	/* construct the new IV */
	(*block)(ivec,ivec,key);
	num = (nbits+7)/8;
	if (enc)	/* encrypt the input */
	    for(n=0 ; n < num ; ++n)
		out[n] = (ovec[16+n] = in[n] ^ ivec[n]);
	else		/* decrypt the input */
	    for(n=0 ; n < num ; ++n)
		out[n] = (ovec[16+n] = in[n]) ^ ivec[n];
	/* shift ovec left... */
	rem = nbits%8;
	num = nbits/8;
	if(rem==0)
	    memcpy(ivec,ovec+num,16);
	else
	    for(n=0 ; n < 16 ; ++n)
		ivec[n] = ovec[n+num]<<rem | ovec[n+num+1]>>(8-rem);

    /* it is not necessary to cleanse ovec, since the IV is not secret */
}

/* N.B. This expects the input to be packed, MS bit first */
void ossl_crypto_cfb128_1_encrypt(const unsigned char *in, unsigned char *out,
		 	size_t bits, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block)
{
    size_t n;
    unsigned char c[1],d[1];

    assert(in && out && key && ivec && num);
    assert(*num == 0);

    for(n=0 ; n<bits ; ++n)
	{
	c[0]=(in[n/8]&(1 << (7-n%8))) ? 0x80 : 0;
	cfbr_encrypt_block(c,d,1,key,ivec,enc,block);
	out[n/8]=(out[n/8]&~(1 << (unsigned int)(7-n%8))) |
		 ((d[0]&0x80) >> (unsigned int)(n%8));
	}
}

void ossl_crypto_cfb128_8_encrypt(const unsigned char *in, unsigned char *out,
			size_t length, const void *key,
			unsigned char ivec[16], int *num,
			int enc, block128_f block)
{
    size_t n;

    assert(in && out && key && ivec && num);
    assert(*num == 0);

    for(n=0 ; n<length ; ++n)
	cfbr_encrypt_block(&in[n],&out[n],8,key,ivec,enc,block);
}
