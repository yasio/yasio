#include "aes_wrapper.h"
#include "endian.h"
#ifdef _ZLIB_SUPPORT
#ifdef _WIN32
#include <zlib/zlib.h> /* from cocos2dx */
#pragma comment(lib, "libzlib.lib")
#else
#include <zlib.h>
#endif
#endif
#include <assert.h>
#include "crypto_wrapper.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

using namespace thelib;

unmanaged_string crypto::aes::encrypt(const thelib::unmanaged_string& in, const char* key, bool compressed)
{
    size_t outlen = 0;
    char* out = nullptr;
    // std::string output;
    // output.c_str
    if(!compressed) {
        outlen = in.length();
        assert(outlen % AES_BLOCK_SIZE == 0);
        out = (char*)calloc(1, outlen);
        aes_cbc_encrypt(in.c_str(), in.length(), out, outlen, key);
    }
    else {
#ifdef _ZLIB_SUPPORT
        managed_cstring compressed_data(zlib::compress_for_aes(in));
        outlen = compressed_data.length();
        assert(outlen % AES_BLOCK_SIZE == 0);
        out = (char*)calloc(1, outlen);
        aes_cbc_encrypt(compressed_data.c_str(), compressed_data.size(), out, outlen, key);
#endif
    }
    return unmanaged_string(out, outlen);
}

unmanaged_string crypto::aes::decrypt(const thelib::unmanaged_string& in, const char* key, bool compressed)
{
    unmanaged_string out;

    size_t bufferlen = 0;
    char* buffer = nullptr;
    bufferlen = in.length();
    buffer = (char*)calloc(1, bufferlen);
    aes_cbc_decrypt(in.c_str(), in.length(), buffer, bufferlen, key);

    // size_t padding_size = 
    if(!compressed) {
        out.assign(buffer, bufferlen);
    }
    else {
#ifdef _ZLIB_SUPPORT
        out = zlib::uncompress(unmanaged_string(buffer, bufferlen));
#endif
        free(buffer);
    }
  
    return out;
}
//
//std::string crypto::aes::encrypt_v2(const thelib::unmanaged_string& in, const char* key, bool compressed)
//{
//    std::string output;
//
//    aes_cbc_encrypt_v2(in.c_str(), in.size(), output, key);
//  
//    return std::move(output);
//}
//
//std::string crypto::aes::decrypt_v2(const thelib::unmanaged_string& in, const char* key, bool compressed)
//{
//    std::string output;
//
//    aes_cbc_decrypt_v2(in.c_str(), in.size(), output, key);
//  
//    return std::move(output);
//}
//
//std::string& crypto::aes::encrypt_v2(const thelib::unmanaged_string& in, std::string& output, const char* key, bool compressed)
//{
//    aes_cbc_encrypt_v2(in.c_str(), in.size(), output, key);
//    return output;
//}
//
//std::string& crypto::aes::decrypt_v2(const thelib::unmanaged_string& in, std::string& output, const char* key, bool compressed)
//{
//    aes_cbc_decrypt_v2(in.c_str(), in.size(), output, key);
//    return output;
//}

//void crypto::aes::pkcs(std::string& plaintext, size_t aes_blocksize)
//{
//    size_t padding_size = aes_blocksize - plaintext.size() % aes_blocksize;
//    for(size_t offst = 0; offst < padding_size; ++offst)
//    {
//        plaintext.push_back((char)padding_size);
//    }
//}

#ifdef _ZLIB_SUPPORT

unmanaged_string crypto::zlib::compress(const thelib::unmanaged_string& in)
{
    // calc destLen
    uLongf destLen = compressBound(in.size());
    char* out((char*)malloc(destLen + 4));

    // do compress
    int ret = ::compress((Bytef*)(out + 4), &destLen, (const Bytef*)in.c_str(), in.size()); 
    
    // write size
    thelib::endian::to_netval_i(in.size(), out);

    return unmanaged_string(out, destLen + sizeof(destLen));
}

unmanaged_string crypto::zlib::compress_for_aes(const thelib::unmanaged_string& in, size_t aes_blocksize)
{
    // calc destLen
    uLongf destLen = compressBound(in.size());

    char* out((char*)malloc(destLen + 4 + aes_blocksize));

     // write size
    thelib::endian::to_netval_i(in.size(), out);

    // do compress
    int ret = ::compress((Bytef*)(out + 4), &destLen, (const Bytef*)in.c_str(), in.size()); 
    
    size_t padding_size = aes_blocksize - ( (4 + destLen ) % aes_blocksize );

    for(size_t offst = 0; offst < padding_size; ++offst) 
    {
        *(out + 4 + destLen + offst) = padding_size;
    }

    return unmanaged_string(out, 4 + destLen + padding_size);
}

unmanaged_string crypto::zlib::uncompress(const thelib::unmanaged_string& in)
{
    // read
    const char* iptr = in.c_str();
    uLongf destLen = 0;

    thelib::endian::to_hostval(destLen, iptr);

    // malloc buffer
    char* out((char*)malloc(destLen));

    // do compress
    int ret = ::uncompress((Bytef*)out, &destLen, (Bytef*)iptr, in.size() - sizeof(destLen)); 
    
    return unmanaged_string(out, destLen);
}

#endif

/// http
#include "random.h"
#include "crypto_utils.h"
#include "nsconv.h"

std::string crypto::hash::md5(const unmanaged_string& plaintext, bool rawoutput)
{
    int     count = 32;
    char    ciphertext[32];
    if(!rawoutput) {
        md5chars(plaintext.c_str(), plaintext.length(), ciphertext);
    }
    else {
        count = 16;
        ::md5(plaintext.c_str(), plaintext.length(), ciphertext);
    }

    return std::string(ciphertext, count);
}

std::string crypto::http::b64dec(const unmanaged_string& ciphertext)
{
    std::string plaintext;
    plaintext.reserve( ciphertext.length() );

    base64_decodestate state;
    base64_init_decodestate(&state);
    int r1 = base64_decode_block(ciphertext.c_str(), ciphertext.length(), (char*)plaintext.c_str(), &state);

    plaintext.assign(plaintext.c_str(), r1);
    return std::move(plaintext);
}

std::string crypto::http::b64enc(const unmanaged_string&  plaintext)
{
    std::string ciphertext;
    ciphertext.reserve( (plaintext.length() * 2) );
    char* wrptr = (char*)ciphertext.c_str();
    base64_encodestate state;
    base64_init_encodestate(&state);
    int r1 = base64_encode_block(plaintext.c_str(), plaintext.length(), wrptr, &state);
    int r2 = base64_encode_blockend((char*)ciphertext.c_str() + r1, &state);

    wrptr[r1 + r2] = '\0';
    ciphertext.assign(wrptr, r1 + r2);
    return std::move(ciphertext);
}

std::string crypto::http::urlencode(const unmanaged_string& input)
{
    std::string output;
    for( size_t ix = 0; ix < input.size(); ix++ )
    {      
        uint8_t buf[4];
        memset( buf, 0, 4 );
        if( isalnum( (uint8_t)input[ix] ) )
        {      
            buf[0] = input[ix];
        }
        //else if ( isspace( (BYTE)sIn[ix] ) )
        //{
        //    buf[0] = '+';
        //}
        else
        {
            buf[0] = '%';
            buf[1] = hex2uchr( (uint8_t)input[ix] >> 4 );
            buf[2] = hex2uchr( (uint8_t)input[ix] % 16);
        }
        output += (char *)buf;
    }
    return std::move(output);
};

std::string crypto::http::urldecode(const unmanaged_string& ciphertext)
{
    std::string result = "";

    for( size_t ix = 0; ix < ciphertext.size(); ix++ )
    {
        uint8_t ch = 0;
        if(ciphertext[ix]=='%')
        {
            ch = (uchr2hex(ciphertext[ix+1])<<4);
            ch |= uchr2hex(ciphertext[ix+2]);
            ix += 2;
        }
        else if(ciphertext[ix] == '+')
        {
            ch = ' ';
        }
        else
        {
            ch = ciphertext[ix];
        }
        result += (char)ch;
    }

    return std::move(result);
}


/*
** UC_AUTHCODE implementation
**
*/
#define ord int
#define chr char
//
//std::string crypto::http::uc_authcode(const std::string& input_string, AuthType operation, const char* public_key, unsigned int expiry)
//{
//    int ckey_length = 4;
//
//    std::string key0 = crypto::hash::md5(public_key);
//    std::string key1 = crypto::hash::md5(key0.substr(0, 16));
//    std::string key2 = crypto::hash::md5(key0.substr(16, 16));
//    std::string key3 = ckey_length ? (operation == kAuthTypeDecode ? input_string.substr(0, ckey_length): nsc::rsubstr(md5(microtime()), ckey_length)) : "";
//
//    std::string cryptkey = key1 + md5(key1 + key3);
//    int key_length = cryptkey.length();
//
//    char times[16];
//    sprintf(times, "%010d", expiry ? expiry + time(NULL) : 0);
//    std::string judge_text = operation == kAuthTypeDecode ? b64dec(input_string.substr(ckey_length)) : times + md5(input_string + key2).substr(0, 16) + input_string;
//
//    int string_length = judge_text.length();
//
//    int i, j, k;
//    uint8_t tmp;
//    std::string result = "";
//    uint8_t box[256];
//    for(i = 0; i < 256; ++i)
//    {
//        box[i] = i;
//    }
//
//    uint8_t rndkey[256];
//    for(i = 0; i < 256; i++) {
//        rndkey[i] = ord(cryptkey[i % key_length]);
//    }
//
//    for(j = i = 0; i < 256; i++) {
//        j = (j + box[i] + rndkey[i]) % 256;
//        tmp = box[i];
//        box[i] = box[j];
//        box[j] = tmp;
//    }
//
//    for(k = j = i = 0; i < string_length; i++) {
//        k = (k + 1) % 256;
//        j = (j + box[k]) % 256;
//        tmp = box[k];
//        box[k] = box[j];
//        box[j] = tmp;
//        result += chr(ord(judge_text[i]) ^ (box[(box[k] + box[j]) % 256]));
//    }
//
//    if(operation == kAuthTypeDecode) {
//        if((nsc::to_numeric<time_t>(result.substr(0, 10)) == 0 || nsc::to_numeric<time_t>(result.substr(0, 10)) - time(NULL) > 0) && result.substr(10, 16) == md5(result.substr(26) + key2).substr(0, 16)) {
//            return result.substr(26);
//        } else {
//            return "";
//        }
//    } else {
//        return key3 + nsc::replace(b64enc(result), "=", "");
//    }
//}


