
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */
/// version 1.5.5
//#include <ngx_config.h>
//#include <ngx_core.h>
#include "ngx_slab.h"

/*
** getpagesize on Windows
**
*/
#ifdef _WIN32
#include <Windows.h>
static int getpagesize(void)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    return sys_info.dwPageSize;
}
#else
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#include "ngx_slab.h"

#define NGX_SLAB_PAGE_MASK   3
#define NGX_SLAB_PAGE        0
#define NGX_SLAB_BIG         1
#define NGX_SLAB_EXACT       2
#define NGX_SLAB_SMALL       3

#define NGX_PTR_SIZE 4

#if (NGX_PTR_SIZE == 4)

#define NGX_SLAB_PAGE_FREE   0
#define NGX_SLAB_PAGE_BUSY   0xffffffff
#define NGX_SLAB_PAGE_START  0x80000000

#define NGX_SLAB_SHIFT_MASK  0x0000000f
#define NGX_SLAB_MAP_MASK    0xffff0000
#define NGX_SLAB_MAP_SHIFT   16

#define NGX_SLAB_BUSY        0xffffffff

#else /* (ngx_PTR_SIZE == 8) */

#define NGX_SLAB_PAGE_FREE   0
#define NGX_SLAB_PAGE_BUSY   0xffffffffffffffff
#define NGX_SLAB_PAGE_START  0x8000000000000000

#define NGX_SLAB_SHIFT_MASK  0x000000000000000f
#define NGX_SLAB_MAP_MASK    0xffffffff00000000
#define NGX_SLAB_MAP_SHIFT   32

#define NGX_SLAB_BUSY        0xffffffffffffffff

#endif


#if (ngx_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)     ngx_memset(p, 0xA5, size)

#else

#define ngx_slab_junk(p, size)

#endif


static ngx_slab_page_t *ngx_slab_alloc_pages(ngx_slab_pool_t *pool,
    ngx_uint_t pages);
static void ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages);
//static bool ngx_slab_empty(ngx_slab_pool_t *pool, ngx_slab_page_t *page);

static ngx_uint_t  ngx_slab_max_size;
static ngx_uint_t  ngx_slab_exact_size;
static ngx_uint_t  ngx_slab_exact_shift;
static ngx_uint_t  ngx_pagesize;
static ngx_uint_t  ngx_pagesize_shift;
static ngx_uint_t  ngx_real_pages;

void
ngx_slab_init(ngx_slab_pool_t *pool)
{
    u_char           *p;
    size_t            size;
    ngx_uint_t        i, n, pages;
    ngx_slab_page_t  *slots;

	/*pagesize*/
	ngx_pagesize = getpagesize();
	for (n = ngx_pagesize, ngx_pagesize_shift = 0; 
			n >>= 1; ngx_pagesize_shift++) { /* void */ }

    /* STUB */
    if (ngx_slab_max_size == 0) {
        ngx_slab_max_size = ngx_pagesize / 2;
        ngx_slab_exact_size = ngx_pagesize / (8 * sizeof(uintptr_t));
        for (n = ngx_slab_exact_size; n >>= 1; ngx_slab_exact_shift++) {
            /* void */
        }
    }

    pool->min_size = 1 << pool->min_shift;

    p = (u_char *) pool + sizeof(ngx_slab_pool_t);
    slots = (ngx_slab_page_t *) p;

    n = ngx_pagesize_shift - pool->min_shift;
    for (i = 0; i < n; i++) {
        slots[i].slab = 0;
        slots[i].next = &slots[i];
        slots[i].prev = 0;
    }

    p += n * sizeof(ngx_slab_page_t);

    size = pool->end - p;
    ngx_slab_junk(p, size);

    pages = (ngx_uint_t) (size / (ngx_pagesize + sizeof(ngx_slab_page_t)));

    ngx_memzero(p, pages * sizeof(ngx_slab_page_t));

    pool->pages = (ngx_slab_page_t *) p;

    pool->free.prev = 0;
    pool->free.next = (ngx_slab_page_t *) p;

    pool->pages->slab = pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (uintptr_t) &pool->free;

    pool->start = (u_char *)
                  ngx_align_ptr((uintptr_t) p + pages * sizeof(ngx_slab_page_t),
                                 ngx_pagesize);

	ngx_real_pages = (pool->end - pool->start) / ngx_pagesize;
	pool->pages->slab = ngx_real_pages;
}


void *
ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size)
{
    void  *p;

    //ngx_shmtx_lock(&pool->mutex);

    p = ngx_slab_alloc_locked(pool, size);

    //ngx_shmtx_unlock(&pool->mutex);

    return p;
}


void *
ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size)
{
    size_t            s;
    uintptr_t         p, n, m, mask, *bitmap;
    ngx_uint_t        i, slot, shift, map;
    ngx_slab_page_t  *page, *prev, *slots;

    if (size >= ngx_slab_max_size) {

		// debug("slab alloc: %zu", size);

        page = ngx_slab_alloc_pages(pool, (size >> ngx_pagesize_shift)
                                          + ((size % ngx_pagesize) ? 1 : 0));
        if (page) {
            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

        } else {
            p = 0;
        }

        goto done;
    }

    if (size > pool->min_size) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }

    slots = (ngx_slab_page_t *) ((u_char *) pool + sizeof(ngx_slab_pool_t));
    page = slots[slot].next;

    if (page->next != page) {

        if (shift < ngx_slab_exact_shift) {

            do {
                p = (page - pool->pages) << ngx_pagesize_shift;
                bitmap = (uintptr_t *) (pool->start + p);

                map = (1 << (ngx_pagesize_shift - shift))
                          / (sizeof(uintptr_t) * 8);

                for (n = 0; n < map; n++) {

                    if (bitmap[n] != NGX_SLAB_BUSY) {

                        for (m = 1, i = 0; m; m <<= 1, i++) {
                            if ((bitmap[n] & m)) {
                                continue;
                            }

                            bitmap[n] |= m;

                            i = ((n * sizeof(uintptr_t) * 8) << shift)
                                + (i << shift);

                            if (bitmap[n] == NGX_SLAB_BUSY) {
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != NGX_SLAB_BUSY) {
                                         p = (uintptr_t) bitmap + i;

                                         goto done;
                                     }
                                }

                                prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = NGX_SLAB_SMALL;
                            }

                            p = (uintptr_t) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == ngx_slab_exact_shift) {

            do {
                if (page->slab != NGX_SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;
  
                        if (page->slab == NGX_SLAB_BUSY) {
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_EXACT;
                        }

                        p = (page - pool->pages) << ngx_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > ngx_slab_exact_shift */

            n = ngx_pagesize_shift - (page->slab & NGX_SLAB_SHIFT_MASK);
            n = 1 << n;
            n = ((uintptr_t) 1 << n) - 1;
            mask = n << NGX_SLAB_MAP_SHIFT;
 
            do {
                if ((page->slab & NGX_SLAB_MAP_MASK) != mask) {

                    for (m = (uintptr_t) 1 << NGX_SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & NGX_SLAB_MAP_MASK) == mask) {
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_BIG;
                        }

                        p = (page - pool->pages) << ngx_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }

    page = ngx_slab_alloc_pages(pool, 1);

    if (page) {
        if (shift < ngx_slab_exact_shift) {
            p = (page - pool->pages) << ngx_pagesize_shift;
            bitmap = (uintptr_t *) (pool->start + p);

            s = 1 << shift;
            n = (1 << (ngx_pagesize_shift - shift)) / 8 / s;

            if (n == 0) {
                n = 1;
            }

            bitmap[0] = (2 << n) - 1;

            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0;
            }

            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL;

            slots[slot].next = page;

            p = ((page - pool->pages) << ngx_pagesize_shift) + s * n;
            p += (uintptr_t) pool->start;

            goto done;

        } else if (shift == ngx_slab_exact_shift) {

            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;

        } else { /* shift > ngx_slab_exact_shift */

            page->slab = ((uintptr_t) 1 << NGX_SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    //debug("slab alloc: %p", (void *)p);

    return (void *) p;
}


void
ngx_slab_free(ngx_slab_pool_t *pool, void *p)
{
    //ngx_shmtx_lock(&pool->mutex);

    ngx_slab_free_locked(pool, p);

    //ngx_shmtx_unlock(&pool->mutex);
}


void
ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p)
{
    size_t            size;
    uintptr_t         slab, m, *bitmap;
    ngx_uint_t        n, type, slot, shift, map;
    ngx_slab_page_t  *slots, *page;

    //debug("slab free: %p", p);

    if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
        //error("ngx_slab_free(): outside of pool");
        goto fail;
    }

    n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & NGX_SLAB_PAGE_MASK;

    switch (type) {

    case NGX_SLAB_SMALL:

        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((uintptr_t) p & (ngx_pagesize - 1)) >> shift;
        m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
        n /= (sizeof(uintptr_t) * 8);
        bitmap = (uintptr_t *) ((uintptr_t) p & ~(ngx_pagesize - 1));

        if (bitmap[n] & m) {

            if (page->next == NULL) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL;
                page->next->prev = (uintptr_t) page | NGX_SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (1 << (ngx_pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {
                n = 1;
            }

            if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
                goto done;
            }

            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (n = 1; n < map; n++) {
                if (bitmap[n]) {
                    goto done;
                }
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_EXACT:

        m = (uintptr_t) 1 <<
                (((uintptr_t) p & (ngx_pagesize - 1)) >> ngx_slab_exact_shift);
        size = ngx_slab_exact_size;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            if (slab == NGX_SLAB_BUSY) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = ngx_slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;
                page->next->prev = (uintptr_t) page | NGX_SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab) {
                goto done;
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_BIG:

        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (uintptr_t) 1 << ((((uintptr_t) p & (ngx_pagesize - 1)) >> shift)
                              + NGX_SLAB_MAP_SHIFT);

        if (slab & m) {

            if (page->next == NULL) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;
                page->next->prev = (uintptr_t) page | NGX_SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & NGX_SLAB_MAP_MASK) {
                goto done;
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_PAGE:

        if ((uintptr_t) p & (ngx_pagesize - 1)) {
            goto wrong_chunk;
        }

		if (slab == NGX_SLAB_PAGE_FREE) {
			//alert("ngx_slab_free(): page is already free");
			goto fail;
        }

		if (slab == NGX_SLAB_PAGE_BUSY) {
			//alert("ngx_slab_free(): pointer to wrong page");
			goto fail;
        }

        n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
        size = slab & ~NGX_SLAB_PAGE_START;

        ngx_slab_free_pages(pool, &pool->pages[n], size);

        ngx_slab_junk(p, size << ngx_pagesize_shift);

        return;
    }

    /* not reached */

    return;

done:

    ngx_slab_junk(p, size);

    return;

wrong_chunk:

	//error("ngx_slab_free(): pointer to wrong chunk");

    goto fail;

chunk_already_free:

	//error("ngx_slab_free(): chunk is already free");

fail:

    return;
}


static ngx_slab_page_t *
ngx_slab_alloc_pages(ngx_slab_pool_t *pool, ngx_uint_t pages)
{
    ngx_slab_page_t  *page, *p;

    for (page = pool->free.next; page != &pool->free; page = page->next) {

        if (page->slab >= pages) {

            if (page->slab > pages) {
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (ngx_slab_page_t *) page->prev;
                p->next = &page[pages];
                page->next->prev = (uintptr_t) &page[pages];

            } else {
                p = (ngx_slab_page_t *) page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | NGX_SLAB_PAGE_START;
            page->next = NULL;
            page->prev = NGX_SLAB_PAGE;

            if (--pages == 0) {
                return page;
            }

            for (p = page + 1; pages; pages--) {
                p->slab = NGX_SLAB_PAGE_BUSY;
                p->next = NULL;
                p->prev = NGX_SLAB_PAGE;
                p++;
            }

            return page;
        }
	}

    ///error("ngx_slab_alloc() failed: no memory");

    return NULL;
}

static void
ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages)
{
    ngx_slab_page_t  *prev, *next;

	if (pages > 1) {
		ngx_memzero(&page[1], (pages - 1)* sizeof(ngx_slab_page_t));
	}  

    if (page->next) {
        prev = (ngx_slab_page_t *) (page->prev & ~NGX_SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }

	page->slab = pages;
	page->prev = (uintptr_t) &pool->free;  
	page->next = pool->free.next;
	page->next->prev = (uintptr_t) page;

	pool->free.next = page;

#ifdef PAGE_MERGE
	if (pool->pages != page) {
		prev = page - 1;
		if (ngx_slab_empty(pool, prev)) {
			for (; prev >= pool->pages; prev--) {
				if (prev->slab != 0) 
				{
					pool->free.next = page->next;
					page->next->prev = (uintptr_t) &pool->free;

					prev->slab += pages;
					ngx_memzero(page, sizeof(ngx_slab_page_t));

					page = prev;

					break;
				}
			}
		}
	}

	if ((page - pool->pages + page->slab) < ngx_real_pages) {
		next = page + page->slab;
		if (ngx_slab_empty(pool, next)) 
		{
			prev = (ngx_slab_page_t *) (next->prev);
			prev->next = next->next;
			next->next->prev = next->prev;

			page->slab += next->slab;
			ngx_memzero(next, sizeof(ngx_slab_page_t));
		}	
	}

#endif
}

void
ngx_slab_dummy_init(ngx_slab_pool_t *pool)
{
    ngx_uint_t n;

	ngx_pagesize = getpagesize();
	for (n = ngx_pagesize, ngx_pagesize_shift = 0; 
			n >>= 1; ngx_pagesize_shift++) { /* void */ }

    if (ngx_slab_max_size == 0) {
        ngx_slab_max_size = ngx_pagesize / 2;
        ngx_slab_exact_size = ngx_pagesize / (8 * sizeof(uintptr_t));
        for (n = ngx_slab_exact_size; n >>= 1; ngx_slab_exact_shift++) {
            /* void */
        }
    }
}

//void
//ngx_slab_stat(ngx_slab_pool_t *pool, ngx_slab_stat_t *stat)
//{
//	uintptr_t 			m, n, mask, slab;
//	uintptr_t 			*bitmap;
//	ngx_uint_t 			i, j, map, type, obj_size;
//	ngx_slab_page_t 	*page;
//
//	ngx_memzero(stat, sizeof(ngx_slab_stat_t));
//
//	page = pool->pages;
// 	stat->pages = (pool->end - pool->start) / ngx_pagesize;;
//
//	for (i = 0; i < stat->pages; i++)
//	{
//		slab = page->slab;
//		type = page->prev & NGX_SLAB_PAGE_MASK;
//
//		switch (type) {
//
//			case NGX_SLAB_SMALL:
//
//				n = (page - pool->pages) << ngx_pagesize_shift;
//                bitmap = (uintptr_t *) (pool->start + n);
//
//				obj_size = 1 << slab;
//                map = (1 << (ngx_pagesize_shift - slab))
//                          / (sizeof(uintptr_t) * 8);
//
//				for (j = 0; j < map; j++) {
//					for (m = 1 ; m; m <<= 1) {
//						if ((bitmap[j] & m)) {
//							stat->used_size += obj_size;
//							stat->b_small   += obj_size;
//						}
//
//					}		
//				}
//
//				stat->p_small++;
//
//				break;
//
//			case NGX_SLAB_EXACT:
//
//				if (slab == NGX_SLAB_BUSY) {
//					stat->used_size += sizeof(uintptr_t) * 8 * ngx_slab_exact_size;
//					stat->b_exact   += sizeof(uintptr_t) * 8 * ngx_slab_exact_size;
//				}
//				else {
//					for (m = 1; m; m <<= 1) {
//						if (slab & m) {
//							stat->used_size += ngx_slab_exact_size;
//							stat->b_exact    += ngx_slab_exact_size;
//						}
//					}
//				}
//
//				stat->p_exact++;
//
//				break;
//
//			case NGX_SLAB_BIG:
//
//				j = ngx_pagesize_shift - (slab & NGX_SLAB_SHIFT_MASK);
//				j = 1 << j;
//				j = ((uintptr_t) 1 << j) - 1;
//				mask = j << NGX_SLAB_MAP_SHIFT;
//				obj_size = 1 << (slab & NGX_SLAB_SHIFT_MASK);
//
//				for (m = (uintptr_t) 1 << NGX_SLAB_MAP_SHIFT; m & mask; m <<= 1)
//				{
//					if ((page->slab & m)) {
//						stat->used_size += obj_size;
//						stat->b_big     += obj_size;
//					}
//				}
//
//				stat->p_big++;
//
//				break;
//
//			case NGX_SLAB_PAGE:
//
//				if (page->prev == NGX_SLAB_PAGE) {		
//					slab 			=  slab & ~NGX_SLAB_PAGE_START;
//					stat->used_size += slab * ngx_pagesize;
//					stat->b_page    += slab * ngx_pagesize;
//					stat->p_page    += slab;
//
//					i += (slab - 1);
//
//					break;
//				}
//
//			default:
//
//				if (slab  > stat->max_free_pages) {
//					stat->max_free_pages = page->slab;
//				}
//
//				stat->free_page += slab;
//
//				i += (slab - 1);
//
//				break;
//		}
//
//		page = pool->pages + i + 1;
//	}
//
//	stat->pool_size = pool->end - pool->start;
//	stat->used_pct = stat->used_size * 100 / stat->pool_size;
//
//	info("pool_size : %zu bytes",	stat->pool_size);
//	info("used_size : %zu bytes",	stat->used_size);
//	info("used_pct  : %zu%%\n",		stat->used_pct);
//
//	info("total page count : %zu",	stat->pages);
//	info("free page count  : %zu\n",	stat->free_page);
//
//	info("small slab use page : %zu,\tbytes : %zu",	stat->p_small, stat->b_small);	
//	info("exact slab use page : %zu,\tbytes : %zu",	stat->p_exact, stat->b_exact);
//	info("big   slab use page : %zu,\tbytes : %zu",	stat->p_big,   stat->b_big);	
//	info("page slab use page  : %zu,\tbytes : %zu\n",	stat->p_page,  stat->b_page);				
//
//	info("max free pages : %zu\n",		stat->max_free_pages);
//}

//static bool 
//ngx_slab_empty(ngx_slab_pool_t *pool, ngx_slab_page_t *page)
//{
//	ngx_slab_page_t *prev;
//
//	if (page->slab == 0) {
//		return true;
//	}
//
//	//page->prev == PAGE | SMALL | EXACT | BIG
//	if (page->next == NULL ) {
//		return false;
//	}
//
//	prev = (ngx_slab_page_t *)(page->prev & ~NGX_SLAB_PAGE_MASK);   
//	while (prev >= pool->pages) { 
//		prev = (ngx_slab_page_t *)(prev->prev & ~NGX_SLAB_PAGE_MASK);   
//	};
//
//	if (prev == &pool->free) {
//		return true;
//	}
//
//	return false;
//}

