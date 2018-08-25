#ifndef _BITS_OPERATIONS_H_
#define _BITS_OPERATIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

int set_bits_value(unsigned char value, void* dest, unsigned int bits_count, unsigned int pos);

int get_bits_value(unsigned char* value, unsigned char source, unsigned int bits_count, unsigned int pos);

#ifdef __cplusplus
}
#endif

#endif
/*
* Copyright (c) 2012-2018 by HALX99  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

