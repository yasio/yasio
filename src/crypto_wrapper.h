#ifndef _CRYPTO_WRAPPER_H_
#define _CRYPTO_WRAPPER_H_
#include <string>
#include <vector>
#include <assert.h>
#include "unreal_string.h"
#include "crypto_utils.h"

using namespace purelib;

namespace crypto {

	/// IV is defined intrin
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
			struct padding_spec < PaddingMode::ISO10126 >
			{
				template<typename _ByteSeqCont>
				inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ISO10126(plaintext, blocksize); }
			};

			template<>
			struct padding_spec < PaddingMode::ANSIX923 >
			{
				template<typename _ByteSeqCont>
				inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ANSIX923(plaintext, blocksize); }
			};

			template<>
			struct padding_spec < PaddingMode::Zeros >
			{
				template<typename _ByteSeqCont>
				inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return detail::padding::ZEROS(plaintext, blocksize); }
			};

			template<>
			struct padding_spec < PaddingMode::None >
			{
				template<typename _ByteSeqCont>
				inline static size_t perform(_BYTE_SEQ_CONT& plaintext, size_t blocksize = AES_BLOCK_SIZE) { return 0; }
			};

			template<CipherMode = CipherMode::CBC>
			struct mode_spec
			{
				static void encrypt(const void* in, size_t inlen,
                    char* out, size_t outlen, const void* private_key, int keybits = 256)
				{
					detail::cbc_encrypt(in, inlen, out, outlen, private_key, keybits);
				}

				static void decrypt(const void* in, size_t inlen,
                    void* out, size_t& outlen, const void* private_key, int keybits = 256)
				{
					detail::cbc_decrypt(in, inlen, out, outlen, private_key, keybits);
				}
			};

			template<>
			struct mode_spec < CipherMode::ECB >
			{
				static void encrypt(const void* in, size_t inlen,
                    void* out, size_t outlen, const void* private_key, int keybits = 256)
				{
					detail::ecb_encrypt(in, inlen, out, outlen, private_key, keybits);
				}

				static void decrypt(const void* in, size_t inlen,
                    void* out, size_t& outlen, const void* private_key, int keybits = 256)
				{
					detail::ecb_decrypt(in, inlen, out, outlen, private_key, keybits);
				}
			};
		};

		/// encrypt decrypt APIs 

		template<CipherMode cipherMode = CipherMode::CBC, PaddingMode paddingMode = PaddingMode::PKCS7, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT encrypt(const _BYTE_SEQ_CONT& in, const void* key = DEFAULT_KEY, int keybits = 256)
		{
			auto out = in;
			privacy::padding_spec<paddingMode>::perform(out);

			privacy::mode_spec<cipherMode>::encrypt(out.data(),
				out.size(),
				&out.front(),
				out.size(),
				key,
				keybits);
			return out;
		}

		template<CipherMode cipherMode = CipherMode::CBC, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT decrypt(const _BYTE_SEQ_CONT& in, const void* key = DEFAULT_KEY, int keybits = 256)
		{
			size_t outlen = in.size();
			_BYTE_SEQ_CONT out(outlen, 0x0);
			privacy::mode_spec<cipherMode>::decrypt(in.data(),
				in.size(),
				&out.front(),
				outlen,
				key,
				keybits);
			out.resize(outlen);
			return out;
		}

		template<CipherMode cipherMode = CipherMode::CBC, PaddingMode paddingMode = PaddingMode::PKCS7, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT encrypt(_BYTE_SEQ_CONT&& in, const void* key = DEFAULT_KEY, int keybits = 256)
		{
			privacy::padding_spec<paddingMode>::perform(in);

			privacy::mode_spec<cipherMode>::encrypt(in.data(),
				in.size(),
				&in.front(),
				in.size(),
				key,
				keybits);
			return in;
		}

		template<CipherMode cipherMode = CipherMode::CBC, typename _ByteSeqCont = std::string>
        inline _BYTE_SEQ_CONT decrypt(_BYTE_SEQ_CONT&& in, const void* key = DEFAULT_KEY, int keybits = 256)
		{
			size_t outlen = in.size();
			privacy::mode_spec<cipherMode>::decrypt(in.data(),
				in.size(),
				&in.front(),
				outlen,
				key,
				keybits);
			in.resize(outlen);
			return in;
		}

		/// wrappers, strongly encrypt, low speed; false: use ECB: fast speed,  light-weighted encrypt
		namespace overlapped {

			template<CipherMode cipherMode = CipherMode::CBC, PaddingMode paddingMode = PaddingMode::PKCS7, typename _ByteSeqCont = std::string>
            inline void encrypt(_BYTE_SEQ_CONT& inout, const void* key = DEFAULT_KEY, int keybits = 256, size_t offset = 0)
			{
				privacy::padding_spec<paddingMode>::perform(inout);

				privacy::mode_spec<cipherMode>::encrypt(inout.data() + offset,
					inout.size() - offset,
					&inout.front() + offset,
					inout.size() - offset,
					key,
					keybits);
			}

			template<CipherMode cipherMode = CipherMode::CBC, typename _ByteSeqCont = std::string>
			inline void decrypt(_BYTE_SEQ_CONT& inout, const void* key = DEFAULT_KEY, int keybits = 256, size_t offset = 0)
			{
				size_t outlen = 0;
				privacy::mode_spec<cipherMode>::decrypt(inout.data() + offset,
					inout.size() - offset,
					&inout.front() + offset,
					outlen,
					key,
					keybits);

				inout.resize(outlen);
			}
		}

	} /* end of namespace crypto::aes */

	namespace zlib {
		/*
		** level values:
		** Z_NO_COMPRESSION         0
		** Z_BEST_SPEED             1
		** Z_BEST_COMPRESSION       9
		** Z_DEFAULT_COMPRESSION  (-1)
		**
		*/
		std::string compress(const unmanaged_string& in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
		std::string uncompress(const unmanaged_string& in);

		std::string deflate(const unmanaged_string& in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
		std::string inflate(const unmanaged_string& in);

		std::string gzcompr(const unmanaged_string& in, int level = -1);
		std::string gzuncompr(const unmanaged_string& in);

		namespace abi {
			std::vector<char> compress(const unmanaged_string& in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
			std::vector<char> uncompress(const unmanaged_string& in);

			std::vector<char> deflate(const unmanaged_string& in, int level = -1); // zlib 1.2.8 Z_DEFAULT_COMPRESSION is -1
			std::vector<char> inflate(const unmanaged_string& in);

			std::vector<char> gzcompr(const unmanaged_string& in, int level = -1);
			std::vector<char> gzuncompr(const unmanaged_string& in);

            managed_cstring   _inflate(const unmanaged_string& in);
		};
	};

	namespace http {
		std::string b64dec(const unmanaged_string& ciphertext);

		std::string b64enc(const unmanaged_string&  plaintext);

		std::string urldecode(const unmanaged_string& ciphertext);

		std::string urlencode(const unmanaged_string& input);
	};

    namespace hash {
        std::string md5(const unmanaged_string& string);
        std::string md5raw(const unmanaged_string& string);
        std::string fmd5(const char* filename);

        std::string md6(const std::string& data); // small data
        std::string md6raw(const std::string& data);
        std::string fmd6(const char* filename, int hashByteLen = 64);
    };
};

#endif

