#ifndef _AES_WRAPPER_H_
#define _AES_WRAPPER_H_

#include <assert.h>
#include <string.h>
#include <string>
#include "aes.h"

/// output and input can overlapped

extern void aes_ecb_encrypt(const char* in, size_t inlen, 
                 char* out, size_t outlen, const char* private_key, int keybits = 256);

extern void aes_ecb_decrypt(const char* in, size_t inlen, 
                 char* out, size_t& outlen, const char* private_key, int keybits = 256);

extern void aes_cbc_encrypt(const char* in, size_t inlen, 
                 char* out, size_t outlen, const char* private_key, int keybits = 256);

extern void aes_cbc_decrypt(const char* in, size_t inlen, 
                 char* out, size_t& outlen, const char* private_key, int keybits = 256);

extern void aes_cbc_encrypt_v2(const char* in, size_t inlen, std::string&, const char* private_key, int keybits = 256);

extern void aes_cbc_decrypt_v2(const char* in, size_t inlen, std::string&, const char* private_key, int keybits = 256);

extern void aes_cbc128_encrypt_v2(const unsigned char *in, size_t len, std::string&, const AES_KEY *key, unsigned char ivec[16]);

extern void aes_cbc128_decrypt_v2(const unsigned char *in, size_t len, std::string&, const AES_KEY *key, unsigned char ivec[16]);

#endif

