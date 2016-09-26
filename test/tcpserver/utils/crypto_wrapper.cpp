#define _ZLIB_SUPPORT 1
#define MD6_SUPPORT 1

#if defined(_WIN32)
#ifdef _ZLIB_SUPPORT
#if !defined(WINRT)
#pragma comment(lib, "libzlib.lib")
#else
#pragma comment(lib, "zlib.lib")
#endif
#else
#include "zlib.h"
#endif
#endif
#include <assert.h>
#include "crypto_wrapper.h"
#if defined(MD6_SUPPORT)
#include "md6.h"
#endif
#include <algorithm>
#include <fstream>
#include "xxfsutility.h"

#define HASH_FILE_BUFF_SIZE 1024

#ifdef _ZLIB_SUPPORT
#include "win32-specific/zlib/include/zlib.h"
#endif

using namespace purelib;

template<typename _ByteSeqCont>
_ByteSeqCont zlib_compress(const unmanaged_string& in, int level)
{
	// calc destLen
    auto destLen = ::compressBound(in.size());
	_ByteSeqCont out(destLen, '\0');

    // do compress
    int ret = ::compress2((Bytef*)(&out.front()), &destLen, (const Bytef*)in.c_str(), in.size(), level);

    if (ret == Z_OK)
    {
        out.resize(destLen);
    }
    return std::move(out);
}

template<typename _ByteSeqCont>
_ByteSeqCont zlib_deflate(const unmanaged_string& in, int level)
{
    int err;
    Bytef buffer[128];
    z_stream d_stream; /* compression stream */

    // strcpy((char*)buffer, "garbage");

    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in = (Bytef*)in.c_str();
    d_stream.avail_in = in.size();
    d_stream.next_out = buffer;
    d_stream.avail_out = sizeof(buffer);

    _ByteSeqCont output;

    err = deflateInit(&d_stream, level);
    if (err != Z_OK) // TODO: log somthing
        return std::move(output);

    output.reserve(in.size());

    for (;;)
    {
        err = deflate(&d_stream, Z_FINISH);

        if (err == Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer) - d_stream.avail_out);
            break;
        }

        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            deflateEnd(&d_stream);
            output.clear();
            return std::move(output);
        }

        // not enough buffer ?
        if (err != Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer));

            d_stream.next_out = buffer;
            d_stream.avail_out = sizeof(buffer);
        }
    }

    deflateEnd(&d_stream);
    if (err != Z_STREAM_END)
    {
        output.clear();
    }

    return std::move(output);
}

template<typename _ByteSeqCont>
_ByteSeqCont zlib_inflate(const unmanaged_string& compr)
{ // inflate
    int err;
    Bytef buffer[128];
    z_stream d_stream; /* decompression stream */

    // strcpy((char*)buffer, "garbage");

    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in = (Bytef*)compr.c_str();
    d_stream.avail_in = compr.size();
    d_stream.next_out = buffer;
    d_stream.avail_out = sizeof(buffer);
    _ByteSeqCont output;
    err = inflateInit(&d_stream);
    if (err != Z_OK) // TODO: log somthing
        return std::move(output);
    // CHECK_ERR(err, "inflateInit");

    output.reserve(compr.size() << 2);
    for (;;)
    {
        err = inflate(&d_stream, Z_NO_FLUSH);

        if (err == Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer) - d_stream.avail_out);
            break;
        }

        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto  _L_end;
        }

        // not enough memory ?
        if (err != Z_STREAM_END)
        {
            // *out = (unsigned char*)realloc(*out, bufferSize * BUFFER_INC_FACTOR);
            output.insert(output.end(), buffer, buffer + sizeof(buffer));

            d_stream.next_out = buffer;
            d_stream.avail_out = sizeof(buffer);
        }
    }

_L_end:
    inflateEnd(&d_stream);
    if (err != Z_STREAM_END)
    {
        output.clear();
    }

    return std::move(output);
}

// inflate alias
template<typename _ByteSeqCont>
_ByteSeqCont zlib_uncompress(const unmanaged_string& in)
{
    return zlib_inflate<_ByteSeqCont>(in);
}

// gzip
template<typename _ByteSeqCont>
_ByteSeqCont zlib_gzcompr(const unmanaged_string& in, int level)
{
    int err;
    Bytef buffer[128];
    z_stream d_stream; /* compression stream */

    // strcpy((char*)buffer, "garbage");

    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in = (Bytef*)in.c_str();
    d_stream.avail_in = in.size();
    d_stream.next_out = buffer;
    d_stream.avail_out = sizeof(buffer);
    _ByteSeqCont output;
    err = deflateInit2(&d_stream, level, Z_DEFLATED, MAX_WBITS + 16, MAX_MEM_LEVEL - 1, Z_DEFAULT_STRATEGY);
    if (err != Z_OK) // TODO: log somthing
        return std::move(output);

    for (;;)
    {
        err = deflate(&d_stream, Z_FINISH);

        if (err == Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer) - d_stream.avail_out);
            break;
        }

        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto _L_end;
        }

        // not enough buffer ?
        if (err != Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer));

            d_stream.next_out = buffer;
            d_stream.avail_out = sizeof(buffer);
        }
    }

_L_end:
    deflateEnd(&d_stream);
    if (err != Z_STREAM_END)
    {
        output.clear();
    }

    return std::move(output);
}

template<typename _ByteSeqCont>
_ByteSeqCont zlib_gzuncompr(const unmanaged_string& compr)
{ // inflate
    int err;
    Bytef buffer[128];
    z_stream d_stream; /* decompression stream */

    // strcpy((char*)buffer, "garbage");

    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in = (Bytef*)compr.c_str();
    d_stream.avail_in = compr.size();
    d_stream.next_out = buffer;
    d_stream.avail_out = sizeof(buffer);
    _ByteSeqCont output;
    err = inflateInit2(&d_stream, MAX_WBITS + 16);
    if (err != Z_OK) // TODO: log somthing
        return std::move(output);
    // CHECK_ERR(err, "inflateInit");
    output.reserve(compr.size() << 2);

    for (;;)
    {
        err = inflate(&d_stream, Z_NO_FLUSH);

        if (err == Z_STREAM_END)
        {
            output.insert(output.end(), buffer, buffer + sizeof(buffer) - d_stream.avail_out);
            break;
        }

        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto _L_end; 
        }

        // not enough memory ?
        if (err != Z_STREAM_END)
        {
            // *out = (unsigned char*)realloc(*out, bufferSize * BUFFER_INC_FACTOR);
            output.insert(output.end(), buffer, buffer + sizeof(buffer));

            d_stream.next_out = buffer;
            d_stream.avail_out = sizeof(buffer);
        }
    }

_L_end:
    inflateEnd(&d_stream);
    if (err != Z_STREAM_END)
    {
        output.clear();
    }

    return std::move(output);
}

std::string crypto::zlib::compress(const unmanaged_string& in, int level)
{
    return zlib_compress<std::string>(in, level);
}
std::string crypto::zlib::uncompress(const unmanaged_string& in)
{
    return zlib_uncompress<std::string>(in);
}
std::string crypto::zlib::deflate(const unmanaged_string& in, int level)
{
    return zlib_deflate<std::string>(in, level);
}
std::string crypto::zlib::inflate(const unmanaged_string& in)
{
    return zlib_inflate<std::string>(in);
}
std::string crypto::zlib::gzcompr(const unmanaged_string& in, int level)
{
    return zlib_gzcompr<std::string>(in, level);
}
std::string crypto::zlib::gzuncompr(const unmanaged_string& in)
{
    return zlib_gzuncompr<std::string>(in);
}

std::vector<char> crypto::zlib::abi::compress(const unmanaged_string& in, int level)
{
    return zlib_compress<std::vector<char>>(in, level);
}
std::vector<char> crypto::zlib::abi::uncompress(const unmanaged_string& in)
{
    return zlib_uncompress<std::vector<char>>(in);
}
std::vector<char> crypto::zlib::abi::deflate(const unmanaged_string& in, int level)
{
    return zlib_deflate<std::vector<char>>(in, level);
}
std::vector<char> crypto::zlib::abi::inflate(const unmanaged_string& in)
{
    return zlib_inflate<std::vector<char>>(in);
}
std::vector<char> crypto::zlib::abi::gzcompr(const unmanaged_string& in, int level)
{
    return zlib_gzcompr<std::vector<char>>(in, level);
}
std::vector<char> crypto::zlib::abi::gzuncompr(const unmanaged_string& in)
{
    return zlib_gzuncompr<std::vector<char>>(in);
}

std::string crypto::hash::md5(const unmanaged_string& plaintext)
{
    char    ciphertext[32];

    md5chars(plaintext.c_str(), plaintext.length(), ciphertext);

    return std::string(ciphertext, sizeof(ciphertext));
}

std::string crypto::hash::md5raw(const unmanaged_string& plaintext)
{
    char    ciphertext[16];

    ::md5(plaintext.c_str(), plaintext.length(), ciphertext);

    return std::string(ciphertext, sizeof(ciphertext));
}

std::string crypto::hash::fmd5(const char* filename)
{
    FILE* fp = fopen(filename, "rb");
    // std::ifstream fin(filename, std::ios_base::binary);
    if (fp == NULL)
    {
        // std::cout << "Transmission Client: open failed!\n";
        return "";
    }

    //fin.seekg(0, std::ios_base::end);
    //std::streamsize bytes_left = fin.tellg(); // filesize initialized
    //fin.seekg(0, std::ios_base::beg);
    auto bytes_left = fsutil::get_file_size(fp);

    char buffer[HASH_FILE_BUFF_SIZE];

    md5_state_t state;

    char hash[16] = { 0 };
    std::string result(32, '\0');

    md5_init(&state);

    while (bytes_left > 0) {
        auto n = (std::min)((std::streamsize)sizeof(buffer), (std::streamsize)bytes_left);
        // fin.read(buffer, bytes_read);
        auto bytes_read = fread(buffer, 1, n, fp);
        md5_append(&state, (unsigned char*)buffer, bytes_read);
        bytes_left -= bytes_read;
    }

    fclose(fp);
    md5_finish(&state, (unsigned char*)hash);

    hexs2chars(hash, sizeof(hash), &result.front(), result.size());

    return std::move(result);
}

#if defined(MD6_SUPPORT)
std::string crypto::hash::md6(const std::string& plaintext)
{
    // char    ciphertext[128];
    std::string result(128, '\0');

    ::md6chars(plaintext.c_str(), plaintext.length(), &result.front(), 64);

    return std::move(result); // std::string(ciphertext, sizeof(ciphertext));
}

std::string crypto::hash::md6raw(const std::string& plaintext)
{
    std::string result(64, '\0');

    ::md6(plaintext.c_str(), plaintext.length(), &result.front(), 64);

    return std::move(result);
}



std::string crypto::hash::fmd6(const char* filename, int hashByteLen)
{
    std::ifstream fin;
    fin.open(filename, std::ios_base::binary);
    if (!fin.is_open())
    {
        // std::cout << "Transmission Client: open failed!\n";
        return "";
    }

    fin.seekg(0, std::ios_base::end);
    std::streamsize bytes_left = fin.tellg(); // filesize initialized
    fin.seekg(0, std::ios_base::beg);

    char buffer[HASH_FILE_BUFF_SIZE];

    md6_state state;

    if (hashByteLen > 64)
        hashByteLen = 64;
    // int hashByteLen = 64;

    // assert(hashByteLen <= 64);

    char hash[64] = { 0 };
    std::string result(hashByteLen << 1, '\0');

    md6_init(&state, hashByteLen << 3);

    while (bytes_left > 0) {
        int bytes_read = (std::min)((std::streamsize)sizeof(buffer), bytes_left);
        fin.read(buffer, bytes_read);
        md6_update(&state, (unsigned char*)buffer, bytes_read << 3);
        bytes_left -= bytes_read;
    }

    fin.close();
    md6_final(&state, (unsigned char*)hash);

    hexs2chars(hash, hashByteLen, &result.front(), result.size());

    return std::move(result);
}
#endif /* MD6_SUPPORT */

std::string crypto::http::b64dec(const unmanaged_string& ciphertext)
{
    std::string plaintext( ciphertext.length(), '\0' );

    base64_decodestate state;
    base64_init_decodestate(&state);
    int r1 = base64_decode_block(ciphertext.c_str(), ciphertext.length(), &plaintext.front(), &state);

    plaintext.resize(r1);
    return std::move(plaintext);
}

std::string crypto::http::b64enc(const unmanaged_string&  plaintext)
{
    std::string ciphertext( (plaintext.length() * 2), '\0' );
    char* wrptr = &ciphertext.front();
    base64_encodestate state;
    base64_init_encodestate(&state);
    int r1 = base64_encode_block(plaintext.c_str(), plaintext.length(), wrptr, &state);
    int r2 = base64_encode_blockend(wrptr + r1, &state);

    ciphertext.resize(r1 + r2);
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
            buf[1] = ::hex2uchr( (uint8_t)input[ix] >> 4 );
            buf[2] = ::hex2uchr( (uint8_t)input[ix] % 16);
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
            ch = (::uchr2hex(ciphertext[ix+1])<<4);
            ch |= ::uchr2hex(ciphertext[ix+2]);
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
// appended zlib_inflate func
managed_cstring crypto::zlib::abi::_inflate(const unmanaged_string& compr)
{ // inflate
    int err;
    Bytef buffer[128];
    z_stream d_stream; /* decompression stream */

    // strcpy((char*)buffer, "garbage");

    d_stream.zalloc = nullptr;
    d_stream.zfree = nullptr;
    d_stream.opaque = (voidpf)0;

    d_stream.next_in = (Bytef*)compr.c_str();
    d_stream.avail_in = compr.size();
    d_stream.next_out = buffer;
    d_stream.avail_out = sizeof(buffer);
    managed_cstring output;
    err = inflateInit(&d_stream);
    if (err != Z_OK) // TODO: log somthing
        return std::move(output);
    // CHECK_ERR(err, "inflateInit");

    output.reserve(compr.size() << 2);
    for (;;)
    {
        err = inflate(&d_stream, Z_NO_FLUSH);

        if (err == Z_STREAM_END)
        {
            output.cappend((const char*)buffer, sizeof(buffer) - d_stream.avail_out);
            break;
        }

        switch (err)
        {
        case Z_NEED_DICT:
            err = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            goto  _L_end;
        }

        // not enough memory ?
        if (err != Z_STREAM_END)
        {
            // *out = (unsigned char*)realloc(*out, bufferSize * BUFFER_INC_FACTOR);
            output.cappend((const char*)buffer, sizeof(buffer));

            d_stream.next_out = buffer;
            d_stream.avail_out = sizeof(buffer);
        }
    }

_L_end:
    inflateEnd(&d_stream);
    if (err != Z_STREAM_END)
    {
        output.clear();
    }

    return std::move(output);
}
