#ifndef _CRYPTO_WRAPPER_H_
#define _CRYPTO_WRAPPER_H_
#include <string>
#include "unreal_string.h"
#include "aes.h"
using namespace thelib;

namespace crypto {

    // Attention: all api you need to use managed_cstring to save output unmanaged_string
    // to free memory correctly
    namespace aes {
        static const char* DEFAULT_KEY = "ZQnNQmA1iIQ3z3ukoPoATdE88OJ0qsMm";
        
        thelib::unmanaged_string encrypt(const thelib::unmanaged_string& in, const char* key = DEFAULT_KEY, bool compressed = false);
        thelib::unmanaged_string decrypt(const thelib::unmanaged_string& in, const char* key = DEFAULT_KEY, bool compressed = false);
        
        /*std::string encrypt_v2(const thelib::unmanaged_string& in, const char* key = DEFAULT_KEY, bool compressed = false);
        std::string decrypt_v2(const thelib::unmanaged_string& in, const char* key = DEFAULT_KEY, bool compressed = false);

        std::string& encrypt_v2(const thelib::unmanaged_string& in, std::string& output, const char* key = DEFAULT_KEY, bool compressed = false);
        std::string& decrypt_v2(const thelib::unmanaged_string& in, std::string& output, const char* key = DEFAULT_KEY, bool compressed = false);
        */
        template<typename _Ty> inline
            void pkcs(_Ty& plaintext, size_t aes_blocksize = AES_BLOCK_SIZE)
        {
            size_t padding_size = aes_blocksize - plaintext.size() % aes_blocksize;
            for(size_t offst = 0; offst < padding_size; ++offst)
            {
                plaintext.push_back((char)padding_size);
            }
        }
    }

    namespace zlib {
        thelib::unmanaged_string compress(const thelib::unmanaged_string& in);
        thelib::unmanaged_string compress_for_aes(const thelib::unmanaged_string& in, size_t aes_blocksize = AES_BLOCK_SIZE);
        thelib::unmanaged_string uncompress(const thelib::unmanaged_string& in);
    };

    namespace http {
#define UC_KEY "W4J687g4QefaK7V0RfK9eaNfZ0Ich8ObY4wafdU4qb83vcO3n1l1f8Gc51k7O2W7"

        typedef enum {
            kAuthTypeEncode,
            kAuthTypeDecode,
        } AuthType;

        std::string encrypt(const unmanaged_string& plaintext, const char* key = UC_KEY);

        std::string decrypt(const unmanaged_string& ciphertext, const char* key = UC_KEY);

        std::string b64dec(const unmanaged_string& ciphertext);

        std::string b64enc(const unmanaged_string&  plaintext);

        std::string urldecode(const unmanaged_string& ciphertext);

        std::string urlencode(const unmanaged_string& input);

        std::string uc_authcode(const std::string& input_string, AuthType operation = kAuthTypeDecode, const char* key = "", unsigned int expiry = 0);

    };

    namespace hash {
        std::string md5(const unmanaged_string& string, bool rawoutput = false);
    };
};

#endif

