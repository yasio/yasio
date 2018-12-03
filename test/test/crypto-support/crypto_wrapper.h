//
// Copyright (c) 2014-2018 halx99 - All Rights Reserved
//
#ifndef _CRYPTO_WRAPPER_H_
#define _CRYPTO_WRAPPER_H_
#include <string>
#include <vector>
#include <assert.h>
#include <string_view>
#include "crypto_utils.h"

namespace crypto {

    namespace aes {

        /// padding
        namespace privacy {
            template<PaddingMode = PaddingMode::PKCS7>
            struct padding_spec
            {
                template<typename _ByteSeqCont>
                inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::PKCS7(plaintext, blocksize); }
            };

            template<>
            struct padding_spec < PaddingMode::ISO10126>
            {
                template<typename _ByteSeqCont>
                inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ISO10126(plaintext, blocksize); }
            };

            template<>
            struct padding_spec < PaddingMode::ANSIX923>
            {
                template<typename _ByteSeqCont>
                inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ANSIX923(plaintext, blocksize); }
            };

            template<>
            struct padding_spec < PaddingMode::Zeros>
            {
                template<typename _ByteSeqCont>
                inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ZEROS(plaintext, blocksize); }
            };

            template<>
            struct padding_spec < PaddingMode::None>
            {
                template<typename _ByteSeqCont>
                inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return 0; }
            };

            template<CipherMode = CipherMode::CBC>
            struct mode_spec
            {
                static void encrypt(const void* in, size_t inlen,
                    void* out, size_t outlen, const void* private_key, int keybits = 256, const void* ivec = nullptr)
                {
                    detail::cbc_encrypt(in, inlen, out, outlen, private_key, keybits, ivec);
                }

                static void decrypt(const void* in, size_t inlen,
                    void* out, size_t& outlen, const void* private_key, int keybits = 256, const void* ivec = nullptr)
                {
                    detail::cbc_decrypt(in, inlen, out, outlen, private_key, keybits, ivec);
                }
            };

            template<>
            struct mode_spec<CipherMode::ECB>
            {
                static void encrypt(const void* in, size_t inlen,
                    void* out, size_t outlen, const void* private_key, int keybits = 256, const void* /*ivec*/ = nullptr)
                {
                    detail::ecb_encrypt(in, inlen, out, outlen, private_key, keybits);
                }

                static void decrypt(const void* in, size_t inlen,
                    void* out, size_t& outlen, const void* private_key, int keybits = 256, const void* /*ivec*/ = nullptr)
                {
                    detail::ecb_decrypt(in, inlen, out, outlen, private_key, keybits);
                }
            };
        };

        /// encrypt decrypt APIs 

        template<CipherMode cipherMode = CipherMode::CBC, PaddingMode paddingMode = PaddingMode::PKCS7, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT encrypt(std::string_view in, const void* key = DEFAULT_KEY, int keybits = 256, const void* ivec = nullptr)
        {
            _BYTE_SEQ_CONT out = in;
            privacy::padding_spec<paddingMode>::perform(out);

            privacy::mode_spec<cipherMode>::encrypt(out.data(),
                out.size(),
                &out.front(),
                out.size(),
                key,
                keybits,
                ivec);
            return out;
        }

        template<CipherMode cipherMode = CipherMode::CBC, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT decrypt(std::string_view in, const void* key = DEFAULT_KEY, int keybits = 256, const void* ivec = nullptr)
        {
            size_t outlen = in.size();
            _BYTE_SEQ_CONT out(outlen, 0x0);
            privacy::mode_spec<cipherMode>::decrypt(in.data(),
                in.size(),
                &out.front(),
                outlen,
                key,
                keybits,
                ivec);
            out.resize(outlen);
            return out;
        }

        /// wrappers, strongly encrypt, low speed; false: use ECB: fast speed,  light-weighted encrypt
        namespace overlapped {

            template<CipherMode cipherMode = CipherMode::CBC, PaddingMode paddingMode = PaddingMode::PKCS7, typename _ByteSeqCont = std::string>
            inline void encrypt(_BYTE_SEQ_CONT& inout, const void* key = DEFAULT_KEY, int keybits = 256, const void* ivec = nullptr, size_t offset = 0)
            {
                privacy::padding_spec<paddingMode>::perform(inout);

                privacy::mode_spec<cipherMode>::encrypt(inout.data() + offset,
                    inout.size() - offset,
                    &inout.front() + offset,
                    inout.size() - offset,
                    key,
                    keybits,
                    ivec);
            }

            template<CipherMode cipherMode = CipherMode::CBC, typename _ByteSeqCont = std::string>
            inline void decrypt(_BYTE_SEQ_CONT& inout, const void* key = DEFAULT_KEY, int keybits = 256, const void* ivec = nullptr, size_t offset = 0)
            {
                size_t outlen = 0;
                privacy::mode_spec<cipherMode>::decrypt(inout.data() + offset,
                    inout.size() - offset,
                    &inout.front() + offset,
                    outlen,
                    key,
                    keybits,
                    ivec);

                inout.resize(outlen + offset);
            }
        }
    } /* end of namespace crypto::aes */

#if _HAS_ZLIB
    namespace zlib {
        /*
        ** level values:
        ** Z_NO_COMPRESSION         0
        ** Z_BEST_SPEED             1
        ** Z_BEST_COMPRESSION       9
        ** Z_DEFAULT_COMPRESSION  (-1)
        **
        */
        std::string compress(std::string_view in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
        std::string uncompress(std::string_view in);

        std::string deflate(std::string_view in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
        std::string inflate(std::string_view in);

        std::string gzcompr(std::string_view in, int level = -1);
        std::string gzuncompr(std::string_view in);

        namespace abi {
            std::vector<char> compress(std::string_view in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
            std::vector<char> uncompress(std::string_view in);

            std::vector<char> deflate(std::string_view in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
            std::vector<char> inflate(std::string_view in);

            std::vector<char> gzcompr(std::string_view in, int level = -1);
            std::vector<char> gzuncompr(std::string_view in);
            std::string   _inflate(std::string_view in);
        };
    };
#endif

    namespace http {
        std::string b64dec(std::string_view ciphertext);

        std::string b64enc(std::string_view  plaintext);

        std::string urldecode(std::string_view ciphertext);

        std::string urlencode(std::string_view input);
    };

    namespace hash {
#if _HAS_MD5
        std::string md5(std::string_view string);
        std::string md5raw(std::string_view string);
        std::string fmd5(const char* filename);
#endif

#if _HAS_MD5
        std::string md6(std::string_view data, size_t hashByteLen = 64); // small data
        std::string md6raw(std::string_view data, size_t hashByteLen = 64);
        std::string fmd6(const char* filename, int hashByteLen = 64);
#endif
    };

#if _HAS_OPENSSL
    namespace rsa {
        namespace rsa {
            enum PaddingMode {
                PKCS1_PADDING = 1,
                SSLV23_PADDING = 2,
                NO_PADDING = 3,
                PKCS1_OAEP_PADDING = 4,
                X931_PADDING = 5,
                /* EVP_PKEY_ only */
                PKCS1_PSS_PADDING = 6,
            };

            namespace pub {
                // supported PaddingModes : RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
                std::string encrypt(std::string_view plaintext, std::string_view keysteam, int paddingMode = PKCS1_OAEP_PADDING);
                std::string decrypt(std::string_view cipertext, std::string_view keysteam, int paddingMode = PKCS1_PADDING);

                // supported PaddingModes : RSA_PKCS1_PADDING, RSA_PKCS1_OAEP_PADDING, RSA_SSLV23_PADDING, RSA_NO_PADDING
                std::string encrypt2(std::string_view plaintext, std::string_view keyfile, int paddingMode = PKCS1_OAEP_PADDING);
                std::string decrypt2(std::string_view cipertext, std::string_view keyfile, int paddingMode = PKCS1_PADDING);
            }

            namespace pri {
                // supported PaddingModes : RSA_PKCS1_PADDING, RSA_X931_PADDING, RSA_NO_PADDING
                std::string encrypt(std::string_view plaintext, std::string_view keysteam, int paddingMode = PKCS1_PADDING);
                std::string decrypt(std::string_view cipertext, std::string_view keysteam, int paddingMode = PKCS1_OAEP_PADDING);

                // supported PaddingModes : RSA_PKCS1_PADDING, RSA_X931_PADDING, RSA_NO_PADDING
                std::string encrypt2(std::string_view plaintext, std::string_view keyfile, int paddingMode = PKCS1_PADDING);
                std::string decrypt2(std::string_view cipertext, std::string_view keyfile, int paddingMode = PKCS1_OAEP_PADDING);
            }
        }
    }
#endif

};

#endif

