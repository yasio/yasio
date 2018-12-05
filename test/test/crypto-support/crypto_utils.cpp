#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <algorithm>


#include "crypto_utils.h"

#if _HAS_INTEL_AES_IN
#include "iaes_asm_interface.h"
#ifdef _WIN32
#pragma comment(lib, "x86/intel_aes86.lib")
#endif
#endif

// Only AES ecb no need ivec
static const unsigned char s_default_ivec[] = { \
      0x00, 0x23, 0x4b, 0x89, 0xaa, 0x96, 0xfe, 0xcd, \
      0xaf, 0x80, 0xfb, 0xf1, 0x78, 0xa2, 0x56, 0x21 \
};

#define local static
#define BAD_CAST (unsigned char*)
#define rrand(min,max) rand() % ((max) - (min)) + (min)
#define sz_align(d, a)     (((d) + ((a) - 1)) & ~((a) - 1))
#define align_size(size) align((unsigned int)(size), sizeof(void*))

namespace crypto {

uint8_t     hex2chr(const uint8_t hex)
{
	return hex > 9 ? (hex - 10 + 'a') : (hex + '0');
}

uint8_t     chr2hex(const uint8_t ch)
{
	return isdigit(ch) ? (ch - '0') : (ch - 'A' + 10);
}

uint8_t     hex2uchr(const uint8_t hex)
{
	return hex > 9 ? (hex - 10 + 'A') : (hex + '0');
}

uint8_t     uchr2hex(const uint8_t ch)
{
	return isdigit(ch) ? (ch - '0') : (ch - 'A' + 10);
}

char*       hex2chrp(const uint8_t hex, char charp[2])
{
	charp[0] = hex2chr(hex >> 4);
	charp[1] = hex2chr(hex & 0x0f);
	return charp;
}

void bin2hex(const void* source, unsigned int sourceLen,
	char* dest, unsigned int destLen)
{
	int i = 0, j = 0;
	int n = (std::min)(sourceLen, destLen);
	// assert( (sourceLen << 1) <= destLen );

	for (; i < n; ++i, j = i << 1)
	{
		(void)hex2chrp(((const uint8_t*)source)[i], dest + j);
	}
}

/* end of basic convertors */

/*
** aes utils
*/
namespace aes {
namespace detail {
namespace software_impl {
	////////////////////////// ecb encrypt/decrypt ///////////////////////////
	void ecb_encrypt(const void* in, size_t inlen,
		void* out, size_t outlen, const void* private_key, int keybits)
	{
		assert((inlen % AES_BLOCK_SIZE) == 0);

		static const int slice_size = AES_BLOCK_SIZE;

		AES_KEY aes_key;
		ossl_aes_set_encrypt_key((unsigned char *)private_key, keybits, &aes_key);

		// const size_t total_bytes = inlen;
		size_t remain_bytes = inlen;
		while (remain_bytes > 0)
		{
			/* if(remain_bytes > slice_size) {*/
			ossl_aes_encrypt((const unsigned char*)in, (unsigned char*)out, &aes_key);
			in = (const char*)in + slice_size;
			out = (char*)out + slice_size;
			remain_bytes -= slice_size;
			//}
			//else {
			//    // encrypt last less AES_BLOCK_SIZE bytes
			//    AES_encrypt((const unsigned char*)in, (unsigned char*)out, &aes_key); 
			//    remain_bytes = 0;
			//}
		}
	}

	void ecb_decrypt(const void* in, size_t inlen,
		void* out, size_t& outlen, const void* private_key, int keybits)
	{
		static const int slice_size = AES_BLOCK_SIZE;

		AES_KEY aes_key;
		ossl_aes_set_decrypt_key((unsigned char *)private_key, keybits, &aes_key);

		// const size_t total_bytes = inlen;
		size_t remain_bytes = inlen;
		outlen = 0;
		while (remain_bytes > 0)
		{
			ossl_aes_decrypt((const unsigned char*)in, (unsigned char*)out, &aes_key);
			if (remain_bytes > AES_BLOCK_SIZE)
			{
				outlen += (slice_size);
			}
			else {
				size_t padding_size = *((unsigned char*)out + slice_size - 1);
				outlen += (slice_size - padding_size);
			}

			in = (const char*)in + slice_size;
			out = (char*)out + slice_size;
			remain_bytes -= slice_size;
		}
	}

	////////////////////////// cbc encrypt/decrypt ///////////////////////////
	void cbc_encrypt(const void* in, size_t inlen,
		void* out, size_t outlen, const void* private_key, int keybits, const void* ivec)
	{
		assert(inlen == outlen);

		unsigned char iv[16];

		memcpy(iv, ivec != nullptr ? ivec : s_default_ivec, sizeof(iv));

		AES_KEY aes_key;
		ossl_aes_set_encrypt_key((unsigned char *)private_key, keybits, &aes_key);

		ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, outlen, &aes_key, iv, AES_ENCRYPT);
	}

	void cbc_decrypt(const void* in, size_t inlen,
		void* out, size_t& outlen, const void* private_key, int keybits, const void* ivec)
	{
		unsigned char iv[16];
		memcpy(iv, ivec != nullptr ? ivec : s_default_ivec, sizeof(iv));

		AES_KEY aes_key;
		ossl_aes_set_decrypt_key((unsigned char *)private_key, keybits, &aes_key);

		ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, inlen, &aes_key, iv, AES_DECRYPT);

		size_t padding_size = ((unsigned char*)out)[inlen - 1];
		if (inlen > padding_size)
			outlen = inlen - padding_size;
	}

	/// AES cbc partial encrypt/decrypt
	void cbc_encrypt_init(cbc_block_state* state, const void* private_key, int keybits, const void* ivec)
	{
		memcpy(state->iv, ivec, sizeof(state->iv));
		ossl_aes_set_encrypt_key((const unsigned char*)private_key, keybits, &state->key);
	}

	void cbc_decrypt_init(cbc_block_state* state, const void* private_key, int keybits, const void* ivec)
	{
        memcpy(state->iv, ivec, sizeof(state->iv));
		ossl_aes_set_decrypt_key((const unsigned char*)private_key, keybits, &state->key);
	}

	void cbc_encrypt_block(cbc_block_state* state, const void* in, size_t inlen,
		void* out, size_t outlen)
	{
		assert(inlen == outlen);

		ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, outlen, &state->key, state->iv, AES_ENCRYPT);
	}

	void cbc_decrypt_block(cbc_block_state* state, const void* in, size_t inlen, void* out, size_t outlen)
	{
		ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, inlen, &state->key, state->iv, AES_DECRYPT);
	}
};

#if _HAS_INTEL_AES_IN
#define INDEX_BITS(bits) (((bits) >> 6) - 2)
#define INDEX_ROUNDS(rounds) (((rounds) >> 1) - 5)
namespace hardware_impl {
	typedef void(*ecb_encdec_t)(_AES_IN UCHAR *, _AES_OUT UCHAR *, _AES_IN UCHAR *, _AES_IN size_t numBlocks);
	typedef void (*cbc_encdec_t)(_AES_IN UCHAR *, _AES_OUT UCHAR *, _AES_IN UCHAR *, _AES_IN size_t numBlocks, _AES_IN UCHAR *iv);
	typedef void (MYSTDCALL*cbc_block_encdec_t)(sAesData *data);

	static ecb_encdec_t ecb_encrypt_bits[]{
		intel_AES_enc128,
		intel_AES_enc192,
		intel_AES_enc256,
	};

	static ecb_encdec_t ecb_decrypt_bits[]{
		intel_AES_enc128,
		intel_AES_enc192,
		intel_AES_enc256,
	};
	
	static cbc_encdec_t cbc_encrypt_bits[] = {
		intel_AES_enc128_CBC, /* 128 >> 6 - 2 = 0 */
		intel_AES_enc192_CBC, /* 192 >> 6 - 2 = 1 */
		intel_AES_enc256_CBC, /* 256 >> 6 - 2 = 2 */
	};

	static cbc_encdec_t cbc_decrypt_bits[] = {
		intel_AES_dec128_CBC, /* 128 >> 6 - 2 = 0 */
		intel_AES_dec192_CBC, /* 192 >> 6 - 2 = 1 */
		intel_AES_dec256_CBC, /* 256 >> 6 - 2 = 2 */
	};

	static cbc_block_encdec_t cbc_block_encrypt_bits[] = {
		iEnc128_CBC, /* 10 >> 1 - 5 = 0 */
		iEnc192_CBC, /* 12 >> 1 - 5 = 1 */
		iEnc256_CBC, /* 14 >> 1 - 5 = 2 */
	};

	static cbc_block_encdec_t cbc_block_decrypt_bits[] = {
		iDec128_CBC, /* 128 >> 6 - 2 = 0 */
		iDec192_CBC, /* 192 >> 6 - 2 = 1 */
		iDec256_CBC, /* 256 >> 6 - 2 = 3 */
	};

	////////////////////////// ecb encrypt/decrypt ///////////////////////////
	void ecb_encrypt(const void* in, size_t inlen,
		void* out, size_t outlen, const void* private_key, int keybits)
	{
		assert((inlen % AES_BLOCK_SIZE) == 0);

		size_t numBlocks = inlen / AES_BLOCK_SIZE;

		assert(INDEX_BITS(keybits) <= 2);
		ecb_encrypt_bits[INDEX_BITS(keybits)]((UCHAR*)in, (UCHAR*)out, (UCHAR*)private_key, numBlocks);
	}

	void ecb_decrypt(const void* in, size_t inlen,
		void* out, size_t& outlen, const void* private_key, int keybits)
	{
		size_t numBlocks = inlen / AES_BLOCK_SIZE;

		assert(INDEX_BITS(keybits) <= 2);
		ecb_decrypt_bits[INDEX_BITS(keybits)]((UCHAR*)in, (UCHAR*)out, (UCHAR*)private_key, numBlocks);

		size_t padding_size = ((unsigned char*)out)[inlen - 1];
		if (inlen > padding_size)
			outlen = inlen - padding_size;
	}

	////////////////////////// cbc encrypt/decrypt ///////////////////////////
	void cbc_encrypt(const void* in, size_t inlen,
		void* out, size_t outlen, const void* private_key, int keybits, const void* ivec)
	{
		assert(inlen == outlen);

		unsigned char iv[16];
		memcpy(iv, ivec != nullptr ? ivec : s_default_ivec, sizeof(iv));

		size_t numBlocks = inlen / AES_BLOCK_SIZE;

		assert(INDEX_BITS(keybits) <= 2);
		cbc_encrypt_bits[INDEX_BITS(keybits)]((UCHAR*)in, (UCHAR*)out, (UCHAR*)private_key, numBlocks, iv);
	}

	void cbc_decrypt(const void* in, size_t inlen,
		void* out, size_t& outlen, const void* private_key, int keybits, const void* ivec)
	{
		unsigned char iv[16];
		memcpy(iv, ivec != nullptr ? ivec : s_default_ivec, sizeof(iv));

		size_t numBlocks = inlen / AES_BLOCK_SIZE;

		assert(INDEX_BITS(keybits) <= 2);

		cbc_decrypt_bits[INDEX_BITS(keybits)]((UCHAR*)in, (UCHAR*)out, (UCHAR*)private_key, numBlocks, iv);

		size_t padding_size = ((unsigned char*)out)[inlen - 1];
		if (inlen > padding_size)
			outlen = inlen - padding_size;
	}

	/// AES cbc partial
	void cbc_encrypt_init(cbc_block_state* state, const void* private_key, int keybits, const void* ivec)
	{
		memcpy(state->iv, ivec, sizeof(state->iv));

		if (keybits == 128) {
			state->key.rounds = 10;
			iEncExpandKey128((UCHAR*)private_key, (UCHAR*)&state->key);
		}
		else if (keybits == 192) {
			state->key.rounds = 12;
			iEncExpandKey192((UCHAR*)private_key, (UCHAR*)&state->key);
		}
		else {
			state->key.rounds = 14;
			iEncExpandKey256((UCHAR*)private_key, (UCHAR*)&state->key);
		}
	}

	void cbc_decrypt_init(cbc_block_state* state, const void* private_key, int keybits, const void* ivec)
	{
		memcpy(state->iv, ivec, sizeof(state->iv));

		// DEFINE_ROUND_KEYS;
		if (keybits == 128) {
			state->key.rounds = 10;
			iDecExpandKey128((UCHAR*)private_key, (UCHAR*)&state->key);
		}
		else if (keybits == 192) {
			state->key.rounds = 12;
			iDecExpandKey192((UCHAR*)private_key, (UCHAR*)&state->key);
		}
		else {
			state->key.rounds = 14;
			iDecExpandKey256((UCHAR*)private_key, (UCHAR*)&state->key);
		}
	}

	void cbc_encrypt_block(cbc_block_state* state, const void* in, size_t inlen,
		void* out, size_t outlen)
	{
		assert(inlen == outlen);
		size_t numBlocks = inlen / AES_BLOCK_SIZE;
		sAesData aesData;
		aesData.in_block = (UCHAR*)in;
		aesData.out_block = (UCHAR*)out;
		aesData.expanded_key = (UCHAR*)&state->key;
		aesData.num_blocks = numBlocks;
		aesData.iv = state->iv;

		assert(INDEX_ROUNDS(state->key.rounds) <= 2);
		cbc_block_encrypt_bits[INDEX_ROUNDS(state->key.rounds)](&aesData);
	}

	void cbc_decrypt_block(cbc_block_state* state, const void* in, size_t inlen, void* out, size_t outlen)
	{
		size_t numBlocks = inlen / AES_BLOCK_SIZE;

		sAesData aesData;
		aesData.in_block = (UCHAR*)in;
		aesData.out_block = (UCHAR*)out;
		aesData.expanded_key = (UCHAR*)&state->key;
		aesData.num_blocks = numBlocks;
		aesData.iv = state->iv;

		assert(INDEX_ROUNDS(state->key.rounds) <= 2);
		cbc_block_decrypt_bits[INDEX_ROUNDS(state->key.rounds)](&aesData);
	}
} /* hardware impl */
#endif

void(*ecb_encrypt)(const void* in, size_t inlen,
	void* out, size_t outlen, const void* private_key, int keybits/*128,192,256*/);
void(*ecb_decrypt)(const void* in, size_t inlen,
	void* out, size_t& outlen, const void* private_key, int keybits/*128,192,256*/);

void(*cbc_encrypt)(const void* in, size_t inlen,
	void* out, size_t outlen, const void* private_key, int keybits/*128,192,256*/, const void* ivec);

void(*cbc_decrypt)(const void* in, size_t inlen,
	void* out, size_t& outlen, const void* private_key, int keybits/*128,192,256*/, const void* ivec);

void(*cbc_encrypt_init)(cbc_block_state* state, const void *private_key,
    int keybits /*128,192,256*/, const void* ivec);

void(*cbc_decrypt_init)(cbc_block_state* state, const void *private_key,
    int keybits /*128,192,256*/, const void* ivec);

void(*cbc_encrypt_block)(cbc_block_state* state, const void *in, size_t inlen, void *out,
    size_t outlen);

void(*cbc_decrypt_block)(cbc_block_state* state, const void *in, size_t inlen, void *out,
    size_t outlen);
}
}
}

/// initialize aes enc/dec
namespace {
	struct autoinit {
		autoinit()
		{
#if _HAS_INTEL_AES_IN
			if (check_for_aes_instructions()) {
				crypto::aes::detail::ecb_encrypt       = crypto::aes::detail::hardware_impl::ecb_encrypt;
                crypto::aes::detail::ecb_decrypt       = crypto::aes::detail::hardware_impl::ecb_decrypt;
                crypto::aes::detail::cbc_encrypt       = crypto::aes::detail::hardware_impl::cbc_encrypt;
                crypto::aes::detail::cbc_decrypt       = crypto::aes::detail::hardware_impl::cbc_decrypt;
                crypto::aes::detail::cbc_encrypt_init  = crypto::aes::detail::hardware_impl::cbc_encrypt_init;
                crypto::aes::detail::cbc_decrypt_init  = crypto::aes::detail::hardware_impl::cbc_decrypt_init;
                crypto::aes::detail::cbc_encrypt_block = crypto::aes::detail::hardware_impl::cbc_encrypt_block;
                crypto::aes::detail::cbc_decrypt_block = crypto::aes::detail::hardware_impl::cbc_decrypt_block;
			}
			else {
				crypto::aes::detail::ecb_encrypt       = crypto::aes::detail::software_impl::ecb_encrypt;
                crypto::aes::detail::ecb_decrypt       = crypto::aes::detail::software_impl::ecb_decrypt;
                crypto::aes::detail::cbc_encrypt       = crypto::aes::detail::software_impl::cbc_encrypt;
                crypto::aes::detail::cbc_decrypt       = crypto::aes::detail::software_impl::cbc_decrypt;
                crypto::aes::detail::cbc_encrypt_init  = crypto::aes::detail::software_impl::cbc_encrypt_init;
                crypto::aes::detail::cbc_decrypt_init  = crypto::aes::detail::software_impl::cbc_decrypt_init;
                crypto::aes::detail::cbc_encrypt_block = crypto::aes::detail::software_impl::cbc_encrypt_block;
                crypto::aes::detail::cbc_decrypt_block = crypto::aes::detail::software_impl::cbc_decrypt_block;
			}
#else
			crypto::aes::detail::ecb_encrypt       = crypto::aes::detail::software_impl::ecb_encrypt;
			crypto::aes::detail::ecb_decrypt       = crypto::aes::detail::software_impl::ecb_decrypt;
			crypto::aes::detail::cbc_encrypt       = crypto::aes::detail::software_impl::cbc_encrypt;
			crypto::aes::detail::cbc_decrypt       = crypto::aes::detail::software_impl::cbc_decrypt;
            crypto::aes::detail::cbc_encrypt_init  = crypto::aes::detail::software_impl::cbc_encrypt_init;
            crypto::aes::detail::cbc_decrypt_init  = crypto::aes::detail::software_impl::cbc_decrypt_init;
			crypto::aes::detail::cbc_encrypt_block = crypto::aes::detail::software_impl::cbc_encrypt_block;
			crypto::aes::detail::cbc_decrypt_block = crypto::aes::detail::software_impl::cbc_decrypt_block;
#endif
		}
	} ;

	static autoinit __;
}
