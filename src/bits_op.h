#ifndef _BITS_OPERATIONS_H_
#define _BITS_OPERATIONS_H_

#ifdef __cplusplus
extern "C" {
#endif
void set_bits_value(unsigned char value, unsigned int bits, unsigned int pos, void* dest);

unsigned char get_bits_value(unsigned char source, unsigned int bits, unsigned int pos);

#ifdef __cplusplus
}
#endif

#endif
/*
* Copyright (c) 2012-2018 by HALX99  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

