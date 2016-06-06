
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 * version: 1.5.5
 */
#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

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
#define ngx_memalign memalign
#else
#define ngx_memalign(alignment, size/*, log*/)  ngx_alloc(size/*, log*/)
#endif

typedef unsigned char u_char;
typedef intptr_t ngx_int_t;
typedef uintptr_t ngx_uint_t;

typedef struct ngx_slab_page_s  ngx_slab_page_t;

struct ngx_slab_page_s {
    ngx_uint_t         slab;
    ngx_slab_page_t  *next;
    ngx_uint_t         prev;
};


typedef struct {
    //ngx_shmtx_sh_t    lock;

    size_t            min_size;
    size_t            min_shift;

    ngx_slab_page_t  *pages;
    ngx_slab_page_t   free;

    u_char           *start;
    u_char           *end;

    //ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    void             *data;
    void             *addr;
} ngx_slab_pool_t;

void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);

#ifdef __cplusplus
}
#endif

#endif /* _NGX_SLAB_H_INCLUDED_ */
