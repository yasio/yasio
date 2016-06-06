#ifndef _APR_MERGE_H_
#define _APR_MERGE_H_

/// arp.h
#include <string.h>
#include <limits.h>
#include <errno.h>
#define APR_DECLARE(t) t
#define APR_DECLARE_NONSTD APR_DECLARE
#define APR_HAVE_STDLIB_H 1
#define APR_INLINE
#define APR_UINT32_MAX UINT_MAX

/// apr_general.h
#define APR_ALIGN(size, boundary) \
    (((size) + ((boundary) - 1)) & ~((boundary) - 1))

#define APR_ALIGN_DEFAULT(size) APR_ALIGN(size, 8)

/// apr_errno.h
/**
 * Type for specifying an error or status code.
 */

typedef int apr_status_t;

/// apr base types
typedef unsigned int apr_uint32_t;

//#define APR_NOMEM ENOMEM
#define APR_OS_START_ERROR 20000
#define APR_ENOMEM ENOMEM
#define APR_SUCCESS 0
#define APR_ENOPOOL        (APR_OS_START_ERROR + 2)


/// apr_private_common.h
#define APR_INT_TRUNC_CAST    int
#define APR_UINT32_TRUNC_CAST apr_uint32_t

/// apr_lib.h


/// typedefs
typedef unsigned int apr_size_t;
typedef unsigned char apr_byte_t;


#endif

