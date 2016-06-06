#include "aes_wrapper.h"

enum { max_data_len = 65536 }; // Avoid network attack

void aes_ecb_encrypt(const char* in, size_t inlen, 
                 char* out, size_t outlen, const char* private_key, int keybits)
{
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú
    assert( (inlen % AES_BLOCK_SIZE) == 0 );

    static const int slice_size = AES_BLOCK_SIZE;

    AES_KEY aes_key;
    ossl_aes_set_encrypt_key((unsigned char *)private_key, keybits, &aes_key);

    // const size_t total_bytes = inlen;
    size_t remain_bytes = inlen;
    while(remain_bytes > 0)
    {
        /* if(remain_bytes > slice_size) {*/
        ossl_aes_encrypt((const unsigned char*)in, (unsigned char*)out, &aes_key); 
        in += slice_size;
        out += slice_size;
        remain_bytes -= slice_size;
        //}
        //else {
        //    // encrypt last less AES_BLOCK_SIZE bytes
        //    AES_encrypt((const unsigned char*)in, (unsigned char*)out, &aes_key); 
        //    remain_bytes = 0;
        //}
    }
}

void aes_ecb_decrypt(const char* in, size_t inlen, 
                 char* out, size_t& outlen, const char* private_key, int keybits)
{
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú

    static const int slice_size = AES_BLOCK_SIZE;

    AES_KEY aes_key;
    ossl_aes_set_decrypt_key((unsigned char *)private_key,keybits,&aes_key);

    // const size_t total_bytes = inlen;
    size_t remain_bytes = inlen;
    outlen = 0;
    while(remain_bytes > 0)
    {
        ossl_aes_decrypt((const unsigned char*)in, (unsigned char*)out, &aes_key); 
        if(remain_bytes > AES_BLOCK_SIZE)
        {
            outlen += (slice_size);
        }
        else {
            size_t padding_size = *(out + slice_size - 1);
            outlen += (slice_size - padding_size);
        }

        in += slice_size;
        out += slice_size;
        remain_bytes -= slice_size; 
    }
}

void aes_cbc_encrypt(const char* in, size_t inlen, 
                 char* out, size_t outlen, const char* private_key, int keybits)
{
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú
    assert( inlen == outlen );

    unsigned char ivec[] = {
        0x00, 0x23, 0x4b, 0x89, 0xaa, 0x96, 0xfe, 0xcd,
        0xaf, 0x80, 0xfb, 0xf1, 0x78, 0xa2, 0x56, 0x21
    };

    //static const int slice_size = AES_BLOCK_SIZE;

    AES_KEY aes_key;
    ossl_aes_set_encrypt_key((unsigned char *)private_key, keybits, &aes_key);

    ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, outlen, &aes_key, ivec, AES_ENCRYPT);
}

void aes_cbc_decrypt(const char* in, size_t inlen, 
                 char* out, size_t& outlen, const char* private_key, int keybits)
{
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú

    //static const int slice_size = AES_BLOCK_SIZE;

    AES_KEY aes_key;
    ossl_aes_set_decrypt_key((unsigned char *)private_key,keybits,&aes_key);

    unsigned char ivec[] = {
        0x00, 0x23, 0x4b, 0x89, 0xaa, 0x96, 0xfe, 0xcd,
        0xaf, 0x80, 0xfb, 0xf1, 0x78, 0xa2, 0x56, 0x21
    };

    ossl_aes_cbc_encrypt((const unsigned char*)in, (unsigned char*)out, inlen, &aes_key, ivec, AES_DECRYPT);

    size_t padding_size = out[inlen - 1];
    if(inlen > padding_size)
        outlen = inlen - padding_size;
}

#ifndef STRICT_ALIGNMENT
#  define STRICT_ALIGNMENT 0
#endif

void aes_cbc_encrypt_v2(const char* in, size_t len, 
                 std::string& output, const char* private_key, int keybits)
{
    /// check max data len
    if(len > max_data_len)
    {
        return;
    }
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú
    // assert( inlen == outlen );
    output.reserve(len);

    unsigned char ivec[] = {
        0x00, 0x23, 0x4b, 0x89, 0xaa, 0x96, 0xfe, 0xcd,
        0xaf, 0x80, 0xfb, 0xf1, 0x78, 0xa2, 0x56, 0x21
    };

    //static const int slice_size = AES_BLOCK_SIZE;

    AES_KEY key;
    ossl_aes_set_encrypt_key((unsigned char *)private_key, keybits, &key);

    /// encrypt
    aes_cbc128_encrypt_v2((const unsigned char*)in, len, output, &key, ivec);
}

void aes_cbc_decrypt_v2(const char* in, size_t inlen, std::string& output, const char* private_key, int keybits)
{
    // aes ¼ÓÃÜÃÜÎÄ²¹ÆëAES_BLOCK_SIZE×Ö½Ú

    //static const int slice_size = AES_BLOCK_SIZE;
    if(inlen > max_data_len)  /// check max data len
    {
        return;
    }

    AES_KEY aes_key;
    ossl_aes_set_decrypt_key((unsigned char *)private_key,keybits,&aes_key);

    unsigned char ivec[] = {
        0x00, 0x23, 0x4b, 0x89, 0xaa, 0x96, 0xfe, 0xcd,
        0xaf, 0x80, 0xfb, 0xf1, 0x78, 0xa2, 0x56, 0x21
    };

    /// decrypt

    aes_cbc128_decrypt_v2((const unsigned char*)in, inlen, output, &aes_key, ivec);

    /// end of decrypt

    size_t padding_size = *output.rbegin();

    if(inlen > padding_size)
        output.resize( inlen - padding_size) ;
}


void aes_cbc128_encrypt_v2(const unsigned char *in, size_t len, std::string& output, const AES_KEY *key, unsigned char ivec[16])
{
    size_t n;
	const unsigned char *iv = ivec;

    assert(in  && ivec);

    unsigned char out[16];
    size_t offset = 0;

#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (STRICT_ALIGNMENT &&
	    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
		while (len>=16) {
			for(n=0; n<16; ++n)
				out[n] = in[n] ^ iv[n];
            ossl_aes_encrypt(out, out, key);
			iv = out;
			len -= 16;
			in  += 16;
            output.append((const char*)out, sizeof(out));
			offset += 16;
		}
	} else {
		while (len>=16) {
			for(n=0; n<16; n+=sizeof(size_t))
				*(size_t*)(out+n) =
				*(size_t*)(in+n) ^ *(size_t*)(iv+n);
			ossl_aes_encrypt(out, out, key);
			iv = out;
			len -= 16;
			in  += 16;
            output.append((const char*)out, sizeof(out));
			offset += 16;
		}
	}
#endif
	while (len) {
		for(n=0; n<16 && n<len; ++n)
			out[n] = in[n] ^ iv[n];
		for(; n<16; ++n)
			out[n] = iv[n];
		ossl_aes_encrypt(out, out, key);
		iv = out;
		if (len<=16) break;
		len -= 16;
		in  += 16;
        output.append((const char*)out, sizeof(out));
		offset += 16;
	}
	memcpy(ivec,iv,16);
}

void aes_cbc128_decrypt_v2(const unsigned char *in, size_t len, std::string& output, const AES_KEY *key, unsigned char ivec[16])
{
    size_t n;
	union { size_t align; unsigned char c[16]; } tmp;

	assert(in && key && ivec);

    unsigned char out[16];
    size_t offset = 0;
#if !defined(OPENSSL_SMALL_FOOTPRINT)
	if (in != (const unsigned char*)output.c_str()) {
		const unsigned char *iv = ivec;

		if (STRICT_ALIGNMENT &&
		    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
			while (len>=16) {
                ossl_aes_decrypt(in, out, key);
				for(n=0; n<16; ++n)
					out[n] ^= iv[n];
				iv = in;
				len -= 16;
				in  += 16;
                output.append((const char*)out, sizeof(out));
				offset += 16;
			}
		}
		else {
			while (len>=16) {
				ossl_aes_decrypt(in, out, key);
				for(n=0; n<16; n+=sizeof(size_t))
					*(size_t *)(out+n) ^= *(size_t *)(iv+n);
				iv = in;
				len -= 16;
				in  += 16;
				output.append((const char*)out, sizeof(out));
				offset += 16;
			}
		}
		memcpy(ivec,iv,16);
	} else {
		if (STRICT_ALIGNMENT &&
		    ((size_t)in|(size_t)out|(size_t)ivec)%sizeof(size_t) != 0) {
			unsigned char c;
			while (len>=16) {
				ossl_aes_decrypt(in, tmp.c, key);
				for(n=0; n<16; ++n) {
					c = in[n];
					out[n] = tmp.c[n] ^ ivec[n];
					ivec[n] = c;
				}
				len -= 16;
				in  += 16;
				output.append((const char*)out, sizeof(out));
				offset += 16;
			}
		}
		else {
			size_t c;
			while (len>=16) {
				ossl_aes_decrypt(in, tmp.c, key);
				for(n=0; n<16; n+=sizeof(size_t)) {
					c = *(size_t *)(in+n);
					*(size_t *)(out+n) =
					*(size_t *)(tmp.c+n) ^ *(size_t *)(ivec+n);
					*(size_t *)(ivec+n) = c;
				}
				len -= 16;
				in  += 16;
				output.append((const char*)out, sizeof(out));
				offset += 16;
			}
		}
	}
#endif
	while (len) {
		unsigned char c;
		ossl_aes_decrypt(in, tmp.c, key);
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
		output.append((const char*)out, sizeof(out));
		offset += 16;
	}
}

