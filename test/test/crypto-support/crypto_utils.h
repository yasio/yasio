//
// Copyright (c) 2014-2015 purelib - All Rights Reserved
//
#ifndef _CRYPTO_UTILS_H_
#define _CRYPTO_UTILS_H_
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define _HAS_MD5 1
#define _HAS_MD6 1
#define _HAS_OPENSSL 1
#define _HAS_ZLIB 1
#define _HAS_LIBB64 1
#define _HAS_INTEL_AES 0

#include "aes.h"
#include "libb64.h"
#include "md5.h"

//#ifdef __cplusplus
// extern "C"
//{
//#endif

/*
 * This package supports both compile-time and run-time determination of CPU
 * byte order.  If ARCH_IS_BIG_ENDIAN is defined as 0, the code will be
 * compiled to run only on little-endian CPUs; if ARCH_IS_BIG_ENDIAN is
 * defined as non-zero, the code will be compiled to run only on big-endian
 * CPUs; if ARCH_IS_BIG_ENDIAN is not defined, the code will be compiled to
 * run on either big- or little-endian CPUs, but will run slightly less
 * efficiently on either one than if ARCH_IS_BIG_ENDIAN is defined.
 */

#define CODEC_OK 0
#define CODEC_BAD_ALLOC -2


///
/// all follow api regardless input as alignment BLOCK_SIZE: 16
/// overlapped api
///

#include <string>
#include <vector>

namespace crypto {
    
/* basic char convertors */
uint8_t hex2uchr(const uint8_t hex);

uint8_t uchr2hex(const uint8_t ch);

uint8_t hex2chr(const uint8_t hex);

uint8_t chr2hex(const uint8_t ch);

char *hex2chrp(const uint8_t hex, char charp[2]);

/* convert hexstream to charstream */
void bin2hex(const void *source, unsigned int sourceLen, char *dest,
             unsigned int destLen);
/* -- end of basic convertors -- */
    
namespace aes {

// 摘要:
//     指定用于加密的块密码模式。
enum CipherMode {
  // 摘要:
  //     密码块链 (CBC)
  //     模式引入了反馈。每个纯文本块在加密前，通过按位“异或”操作与前一个块的密码文本结合。这样确保了即使纯文本包含许多相同的块，这些块中的每一个也会加密为不同的密码文本块。在加密块之前，初始化向量通过按位“异或”操作与第一个纯文本块结合。如果密码文本块中有一个位出错，相应的纯文本块也将出错。此外，后面的块中与原出错位的位置相同的位也将出错。
  CBC = 1,
  //
  // 摘要:
  //     电子密码本 (ECB)
  //     模式分别加密每个块。这意味着任何纯文本块只要相同并且在同一消息中，或者在用相同的密钥加密的不同消息中，都将被转换成同样的密码文本块。如果要加密的纯文本包含大量重复的块，则逐块破解密码文本是可行的。另外，随时准备攻击的对手可能在您没有察觉的情况下替代和交换个别的块。如果密码文本块中有一个位出错，相应的整个纯文本块也将出错。
  ECB = 2,
  //
  // 摘要:
  //     输出反馈 (OFB)
  //     模式将少量递增的纯文本处理成密码文本，而不是一次处理整个块。此模式与
  //     CFB
  //     相似；这两种模式的唯一差别是移位寄存器的填充方式不同。如果密码文本中有一个位出错，纯文本中相应的位也将出错。但是，如果密码文本中有多余或者缺少的位，则那个位之后的纯文本都将出错。
  OFB = 3,
  //
  // 摘要:
  //     密码反馈 (CFB)
  //     模式将少量递增的纯文本处理成密码文本，而不是一次处理整个块。该模式使用在长度上为一个块且被分为几部分的移位寄存器。例如，如果块大小为
  //     8 个字节，并且每次处理一个字节，则移位寄存器被分为 8
  //     个部分。如果密码文本中有一个位出错，则一个纯文本位出错，并且移位寄存器损坏。这将导致接下来若干次递增的纯文本出错，直到出错位从移位寄存器中移出为止。
  CFB = 4,
  //
  // 摘要:
  //     密码文本窃用 (CTS)
  //     模式处理任何长度的纯文本并产生长度与纯文本长度匹配的密码文本。除了最后两个纯文本块外，对于所有其他块，此模式与
  //     CBC 模式的行为相同。
  CTS = 5,
};

// 摘要:
//     指定在消息数据块比加密操作所需的全部字节数短时应用的填充类型。
enum PaddingMode {
  // 摘要:
  //     不填充。
  None = 1,
  //
  // 摘要:
  //     PKCS #7 填充字符串由一个字节序列组成，每个字节填充该字节序列的长度。
  PKCS7 = 2,
  //
  // 摘要:
  //     填充字符串由设置为零的字节组成。
  Zeros = 3,
  //
  // 摘要:
  //     ANSIX923
  //     填充字符串由一个字节序列组成，此字节序列的最后一个字节填充字节序列的长度，其余字节均填充数字零。
  ANSIX923 = 4,
  //
  // 摘要:
  //     ISO10126
  //     填充字符串由一个字节序列组成，此字节序列的最后一个字节填充字节序列的长度，其余字节填充随机数据。
  ISO10126 = 5,
};

#define _BYTE_SEQ_CONT _ByteSeqCont

static const char *DEFAULT_KEY = "ZQnNQmA1iIQ3z3ukoPoATdE88OJ0qsMm";

namespace detail {

struct cbc_block_state
{
    AES_KEY key;
    unsigned char iv[16];
};

namespace padding {

template <typename _ByteSeqCont>
inline size_t PKCS7(_BYTE_SEQ_CONT &plaintext,
                    size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(typename _BYTE_SEQ_CONT::value_type) == 1,
                "PKCS7: only allow stl sequently byte conatiner!");
  size_t padding_size = blocksize - plaintext.size() % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    plaintext.push_back((char)padding_size);
  }
  return padding_size;
}

template <typename _ByteSeqCont>
inline size_t ZEROS(_BYTE_SEQ_CONT &plaintext,
                    size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ZEROS: only allow stl sequently byte conatiner!");
  size_t padding_size = blocksize - plaintext.size() % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    plaintext.push_back((char)0);
  }
  return padding_size;
}

template <typename _ByteSeqCont>
inline size_t ANSIX923(_BYTE_SEQ_CONT &plaintext,
                       size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ANSIX923: only allow stl sequently byte conatiner!");
  size_t padding_size = blocksize - plaintext.size() % blocksize;
  for (size_t offst = 0; offst < padding_size - 1; ++offst) {
    plaintext.push_back((char)0);
  }
  plaintext.push_back((char)padding_size);
  return padding_size;
}

template <typename _ByteSeqCont>
inline size_t ISO10126(_BYTE_SEQ_CONT &plaintext,
                       size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ISO10126: only allow stl sequently byte conatiner!");
  size_t padding_size = blocksize - plaintext.size() % blocksize;
  for (size_t offst = 0; offst < padding_size - 1; ++offst) {
    plaintext.push_back((char)(unsigned char)mathext::rrand(0, 256));
  }
  plaintext.push_back((char)padding_size);
  return padding_size;
}

template <typename _ByteSeqCont = std::string>
inline _BYTE_SEQ_CONT PKCS7(size_t datasize,
                            size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ISO10126: only allow stl sequently byte conatiner!");
  _BYTE_SEQ_CONT padding;
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    padding.push_back((char)padding_size);
  }
  return (padding);
}

template <typename _ByteSeqCont = std::string>
inline _BYTE_SEQ_CONT ZEROS(size_t datasize,
                            size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ISO10126: only allow stl sequently byte conatiner!");
  _BYTE_SEQ_CONT padding;
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    padding.push_back((char)0);
  }
  return (padding);
}

template <typename _ByteSeqCont = std::string>
inline _BYTE_SEQ_CONT ANSIX923(size_t datasize,
                               size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ISO10126: only allow stl sequently byte conatiner!");
  _BYTE_SEQ_CONT padding;
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size - 1; ++offst) {
    padding.push_back((char)0);
  }
  padding.push_back((char)padding_size);
  return (padding);
}

template <typename _ByteSeqCont = std::string>
inline _BYTE_SEQ_CONT ISO10126(size_t datasize,
                               size_t blocksize = AES_BLOCK_SIZE) {
  static_assert(sizeof(_BYTE_SEQ_CONT::value_type) == 1,
                "ISO10126: only allow stl sequently byte conatiner!");
  _BYTE_SEQ_CONT padding;
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size - 1; ++offst) {
    padding.push_back((char)(unsigned char)mathext::rrand(0, 256));
  }
  padding.push_back((char)padding_size);
  return (padding);
}

inline size_t PKCS7(size_t datasize, char padding[16],
                    size_t blocksize = AES_BLOCK_SIZE) {
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    padding[AES_BLOCK_SIZE - 1 - offst] = (unsigned char)padding_size;
  }
  return padding_size;
}

inline size_t ZEROS(size_t datasize, char padding[16],
                    size_t blocksize = AES_BLOCK_SIZE) {
  size_t padding_size = blocksize - datasize % blocksize;
  for (size_t offst = 0; offst < padding_size; ++offst) {
    padding[AES_BLOCK_SIZE - 1 - offst] = 0;
  }
  return padding_size;
}

inline size_t ANSIX923(size_t datasize, char padding[16],
                       size_t blocksize = AES_BLOCK_SIZE) {
  size_t padding_size = blocksize - datasize % blocksize;
  padding[AES_BLOCK_SIZE - 1] = (unsigned char)padding_size;
  for (size_t offst = 1; offst < padding_size; ++offst) {
    padding[AES_BLOCK_SIZE - 1 - offst] = 0;
  }
  return padding_size;
}

/* value range: [min, limit) half closed interval */
inline unsigned int rrand(unsigned int min, unsigned int limit)
{
    return (::rand() % ((limit)-(min)) + (min));
}
inline size_t ISO10126(size_t datasize, char padding[16],
                       size_t blocksize = AES_BLOCK_SIZE) {
  size_t padding_size = blocksize - datasize % blocksize;
  padding[AES_BLOCK_SIZE - 1] = (unsigned char)padding_size;
  for (size_t offst = 1; offst < padding_size; ++offst) {
    padding[AES_BLOCK_SIZE - 1 - offst] = (unsigned char)rrand(0, 256);
  }
  return padding_size;
}
} // namespace padding

/// AES ecb
extern void (*ecb_encrypt)(const void *in, size_t inlen, void *out,
                           size_t outlen, const void *private_key, int keybits);
extern void (*ecb_decrypt)(const void *in, size_t inlen, void *out,
                           size_t &outlen, const void *private_key,
                           int keybits);

/// AES CBC
extern void (*cbc_encrypt)(const void *in, size_t inlen, void *out,
                           size_t outlen, const void *private_key,
                           int keybits /*128,192,256*/, const void* ivec);

extern void (*cbc_decrypt)(const void *in, size_t inlen, void *out,
                           size_t &outlen, const void *private_key,
                           int keybits /*128,192,256*/, const void* ivec);

/// AES CBC block encrypt/decrypt
extern void (*cbc_encrypt_init)(cbc_block_state* state, const void *private_key,
                                int keybits /*128,192,256*/, const void* ivec);

extern void (*cbc_decrypt_init)(cbc_block_state* state, const void *private_key,
                                int keybits /*128,192,256*/, const void* ivec);

extern void (*cbc_encrypt_block)(cbc_block_state* state, const void *in, size_t inlen, void *out,
                                 size_t outlen);

extern void (*cbc_decrypt_block)(cbc_block_state* state, const void *in, size_t inlen, void *out,
                                 size_t outlen);
}; // namespace detail
}; // namespace aes
}; // namespace crypto

#endif /* _CRYPTO_UTILS_H_ */
