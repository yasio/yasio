/*************************************************
* module:  nginx_pool                            *
* nginx source code version: 1.5.12              *
* comment: copy from nginx source code           *
* copyer:  xseekerj                              *
**************************************************/
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_
#include <memory>

/*
 * #include "ngx_core.h"
 * includes...
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <mutex>
#include "oslib.h"

/*
** copy from ngx_core.h
** 
*/
#define  NGX_OK          0
#define  NGX_ERROR      -1
#define  NGX_AGAIN      -2
#define  NGX_BUSY       -3
#define  NGX_DONE       -4
#define  NGX_DECLINED   -5
#define  NGX_ABORT      -6

typedef unsigned char u_char;
typedef intptr_t ngx_int_t;
typedef uintptr_t ngx_uint_t;


typedef struct ngx_pool_s        ngx_pool_t;
typedef struct ngx_chain_s       ngx_chain_t;

/*
** copy from ngx_core.h
** 
*/
#ifndef NGX_ALIGNMENT
#define NGX_ALIGNMENT   sizeof(void*/*unsigned long*/)    /* platform word */
#endif

#define ngx_align(d, a)     (((d) + (a - 1)) & ~(a - 1))
#define ngx_align_ptr(p, a)                                                   \
    (u_char *) (((uintptr_t) (p) + ((uintptr_t) a - 1)) & ~((uintptr_t) a - 1))


/*
**  copy from nginx_string.h
*/
/*
 * msvc and icc7 compile memset() to the inline "rep stos"
 * while ZeroMemory() and bzero() are the calls.
 * icc7 may also inline several mov's of a zeroed register for small blocks.
 */
#define ngx_memzero(buf, n)       (void) memset(buf, 0, n)
#define ngx_memset(buf, c, n)     (void) memset(buf, c, n)

/*
** copy from ngx_alloc.h
** 
*/
#define ngx_alloc malloc
#define ngx_free free

/*
 * Linux has memalign() or posix_memalign()
 * Solaris has memalign()
 * FreeBSD 7.0 has posix_memalign(), besides, early version's malloc()
 * aligns allocations bigger than page size at the page boundary
 */

//#if (NGX_HAVE_POSIX_MEMALIGN || NGX_HAVE_MEMALIGN)
#ifdef __linux__

void *ngx_memalign(size_t alignment, size_t size/*, ngx_log_t *log*/);

#else

#define ngx_memalign(alignment, size/*, log*/)  ngx_alloc(size/*, log*/)

#endif

extern ngx_uint_t  ngx_pagesize;


/*
**  copy from ngx_palloc.h
** 
*/
/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)


typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

struct ngx_pool_cleanup_s {
    ngx_pool_cleanup_pt   handler;
    void                 *data;
    ngx_pool_cleanup_t   *next;
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

struct ngx_pool_large_s {
    ngx_pool_large_t     *next;
    void                 *alloc;
};


typedef struct {
    u_char               *last;
    u_char               *end;
    ngx_pool_t           *next;
    ngx_uint_t            failed;
} ngx_pool_data_t;


struct ngx_pool_s {
    ngx_pool_data_t       d;
    size_t                max;
    ngx_pool_t           *current;
    ngx_chain_t          *chain;
    ngx_pool_large_t     *large;
    ngx_pool_cleanup_t   *cleanup;
    // ngx_log_t            *log;
};


/*typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;*/


//void *ngx_alloc(size_t size/*, ngx_log_t *log*/);
//void *ngx_calloc(size_t size/*, ngx_log_t *log*/);

ngx_pool_t *ngx_create_pool(size_t size/*, ngx_log_t *log*/);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
//void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
//void ngx_pool_cleanup_file(void *data);
//void ngx_pool_delete_file(void *data);
//void* ngx_pool_alloc(ngx_pool_t *pool, size_t size);

#endif /* _NGX_PALLOC_H_INCLUDED_ */
