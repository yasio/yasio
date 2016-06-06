#include "apr_allocator.h"

#include <stddef.h> /* for offsetof */

#if APR_HAVE_STDLIB_H
#include <stdlib.h>     /* for malloc, free and abort */
#endif

#if APR_HAVE_UNISTD_H
#include <unistd.h>     /* for getpid and sysconf */
#endif

#if APR_ALLOCATOR_USES_MMAP
#include <sys/mman.h>
#endif

/*
 * Magic numbers
 */

/*
 * XXX: This is not optimal when using --enable-allocator-uses-mmap on
 * XXX: machines with large pagesize, but currently the sink is assumed
 * XXX: to be index 0, so MIN_ALLOC must be at least two pages.
 */
#define MIN_ALLOC (2 * BOUNDARY_SIZE)
/// apr #define BOUNDARY_INDEX 20
#define MAX_INDEX   20

#if APR_ALLOCATOR_USES_MMAP && defined(_SC_PAGESIZE)
static unsigned int boundary_index;
static unsigned int boundary_size;
#define BOUNDARY_INDEX  boundary_index
#define BOUNDARY_SIZE   boundary_size
#else
/// apr #define BOUNDARY_INDEX 12
#define BOUNDARY_INDEX 12 // sizeof(void*) 
#define BOUNDARY_SIZE (1 << BOUNDARY_INDEX)
#endif

/* 
 * Timing constants for killing subprocesses
 * There is a total 3-second delay between sending a SIGINT 
 * and sending of the final SIGKILL.
 * TIMEOUT_INTERVAL should be set to TIMEOUT_USECS / 64
 * for the exponetial timeout alogrithm.
 */
#define TIMEOUT_USECS    3000000
#define TIMEOUT_INTERVAL   46875

/*
 * Allocator
 *
 * @note The max_free_index and current_free_index fields are not really
 * indices, but quantities of BOUNDARY_SIZE big memory blocks.
 */

struct apr_allocator_t {
    /** largest used index into free[], always < MAX_INDEX */
    apr_uint32_t        max_index;
    /** Total size (in BOUNDARY_SIZE multiples) of unused memory before
     * blocks are given back. @see apr_allocator_max_free_set().
     * @note Initialized to APR_ALLOCATOR_MAX_FREE_UNLIMITED,
     * which means to never give back blocks.
     */
    apr_uint32_t        max_free_index;
    /**
     * Memory size (in BOUNDARY_SIZE multiples) that currently must be freed
     * before blocks are given back. Range: 0..max_free_index
     */
    apr_uint32_t        current_free_index;
#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;
#endif /* APR_HAS_THREADS */
    // apr_pool_t         *owner;
    /**
     * Lists of free nodes. Slot 0 is used for oversized nodes,
     * and the slots 1..MAX_INDEX-1 contain nodes of sizes
     * (i+1) * BOUNDARY_SIZE. Example for BOUNDARY_INDEX == 12:
     * slot  0: nodes larger than 81920
     * slot  1: size  8192
     * slot  2: size 12288
     * ...
     * slot 19: size 81920
     */
    apr_memnode_t      *free[MAX_INDEX];
};

#define SIZEOF_ALLOCATOR_T  APR_ALIGN_DEFAULT(sizeof(apr_allocator_t))


/*
 * Allocator
 */

APR_DECLARE(apr_status_t) apr_allocator_create(apr_allocator_t **allocator)
{
    apr_allocator_t *new_allocator;

    *allocator = NULL;

    if ((new_allocator = (apr_allocator_t*)malloc(SIZEOF_ALLOCATOR_T)) == NULL)
        return APR_ENOMEM;

    memset(new_allocator, 0, SIZEOF_ALLOCATOR_T);
    new_allocator->max_free_index = APR_ALLOCATOR_MAX_FREE_UNLIMITED;

    *allocator = new_allocator;

    return APR_SUCCESS;
}

APR_DECLARE(void) apr_allocator_destroy(apr_allocator_t *allocator)
{
    apr_uint32_t index;
    apr_memnode_t *node, **ref;

    for (index = 0; index < MAX_INDEX; index++) {
        ref = &allocator->free[index];
        while ((node = *ref) != NULL) {
            *ref = node->next;
#if APR_ALLOCATOR_USES_MMAP
            munmap(node, (node->index+1) << BOUNDARY_INDEX);
#else
            free(node);
#endif
        }
    }

    free(allocator);
}

APR_DECLARE(void) apr_allocator_max_free_set(apr_allocator_t *allocator,
                                             apr_size_t in_size)
{
    apr_uint32_t max_free_index;
    apr_uint32_t size = (APR_UINT32_TRUNC_CAST)in_size;

#if APR_HAS_THREADS
    apr_thread_mutex_t *mutex;

    mutex = apr_allocator_mutex_get(allocator);
    if (mutex != NULL)
        apr_thread_mutex_lock(mutex);
#endif /* APR_HAS_THREADS */

    max_free_index = APR_ALIGN(size, BOUNDARY_SIZE) >> BOUNDARY_INDEX;
    allocator->current_free_index += max_free_index;
    allocator->current_free_index -= allocator->max_free_index;
    allocator->max_free_index = max_free_index;
    if (allocator->current_free_index > max_free_index)
        allocator->current_free_index = max_free_index;

#if APR_HAS_THREADS
    if (mutex != NULL)
        apr_thread_mutex_unlock(mutex);
#endif
}

static APR_INLINE
apr_memnode_t *allocator_alloc(apr_allocator_t *allocator, apr_size_t in_size)
{
    apr_memnode_t *node, **ref;
    apr_uint32_t max_index;
    apr_size_t size, i, index;

    /* Round up the block size to the next boundary, but always
     * allocate at least a certain size (MIN_ALLOC).
     */
    size = APR_ALIGN(in_size + APR_MEMNODE_T_SIZE, BOUNDARY_SIZE);
    if (size < in_size) {
        return NULL;
    }
    if (size < MIN_ALLOC)
        size = MIN_ALLOC;

    /* Find the index for this node size by
     * dividing its size by the boundary size
     */
    index = (size >> BOUNDARY_INDEX) - 1;
    
    if (index > APR_UINT32_MAX) {
        return NULL;
    }

    /* First see if there are any nodes in the area we know
     * our node will fit into.
     */
    if (index <= allocator->max_index) {
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_lock(allocator->mutex);
#endif /* APR_HAS_THREADS */

        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         *
         * NOTE: an optimization would be to check
         * allocator->free[index] first and if no
         * node is present, directly use
         * allocator->free[max_index].  This seems
         * like overkill though and could cause
         * memory waste.
         */
        max_index = allocator->max_index;
        ref = &allocator->free[index];
        i = index;
        while (*ref == NULL && i < max_index) {
           ref++;
           i++;
        }

        if ((node = *ref) != NULL) {
            /* If we have found a node and it doesn't have any
             * nodes waiting in line behind it _and_ we are on
             * the highest available index, find the new highest
             * available index
             */
            if ((*ref = node->next) == NULL && i >= max_index) {
                do {
                    ref--;
                    max_index--;
                }
                while (*ref == NULL && max_index > 0);

                allocator->max_index = max_index;
            }

            allocator->current_free_index += node->index + 1;
            if (allocator->current_free_index > allocator->max_free_index)
                allocator->current_free_index = allocator->max_free_index;

#if APR_HAS_THREADS
            if (allocator->mutex)
                apr_thread_mutex_unlock(allocator->mutex);
#endif /* APR_HAS_THREADS */

            node->next = NULL;
            node->first_avail = (char *)node + APR_MEMNODE_T_SIZE;

            return node;
        }

#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_unlock(allocator->mutex);
#endif /* APR_HAS_THREADS */
    }

    /* If we found nothing, seek the sink (at index 0), if
     * it is not empty.
     */
    else if (allocator->free[0]) {
#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_lock(allocator->mutex);
#endif /* APR_HAS_THREADS */

        /* Walk the free list to see if there are
         * any nodes on it of the requested size
         */
        ref = &allocator->free[0];
        while ((node = *ref) != NULL && index > node->index)
            ref = &node->next;

        if (node) {
            *ref = node->next;

            allocator->current_free_index += node->index + 1;
            if (allocator->current_free_index > allocator->max_free_index)
                allocator->current_free_index = allocator->max_free_index;

#if APR_HAS_THREADS
            if (allocator->mutex)
                apr_thread_mutex_unlock(allocator->mutex);
#endif /* APR_HAS_THREADS */

            node->next = NULL;
            node->first_avail = (char *)node + APR_MEMNODE_T_SIZE;

            return node;
        }

#if APR_HAS_THREADS
        if (allocator->mutex)
            apr_thread_mutex_unlock(allocator->mutex);
#endif /* APR_HAS_THREADS */
    }

    /* If we haven't got a suitable node, malloc a new one
     * and initialize it.
     */
#if APR_ALLOCATOR_USES_MMAP
    if ((node = mmap(NULL, size, PROT_READ|PROT_WRITE,
                     MAP_PRIVATE|MAP_ANON, -1, 0)) == MAP_FAILED)
#else
    if ((node = (apr_memnode_t*)malloc(size)) == NULL)
#endif
        return NULL;

    node->next = NULL;
    node->index = (APR_UINT32_TRUNC_CAST)index;
    node->first_avail = (char *)node + APR_MEMNODE_T_SIZE;
    node->endp = (char *)node + size;

    return node;
}

static APR_INLINE
void allocator_free(apr_allocator_t *allocator, apr_memnode_t *node)
{
    apr_memnode_t *next, *freelist = NULL;
    apr_uint32_t index, max_index;
    apr_uint32_t max_free_index, current_free_index;

#if APR_HAS_THREADS
    if (allocator->mutex)
        apr_thread_mutex_lock(allocator->mutex);
#endif /* APR_HAS_THREADS */

    max_index = allocator->max_index;
    max_free_index = allocator->max_free_index;
    current_free_index = allocator->current_free_index;

    /* Walk the list of submitted nodes and free them one by one,
     * shoving them in the right 'size' buckets as we go.
     */
    do {
        next = node->next;
        index = node->index;

        if (max_free_index != APR_ALLOCATOR_MAX_FREE_UNLIMITED
            && index + 1 > current_free_index) {
            node->next = freelist;
            freelist = node;
        }
        else if (index < MAX_INDEX) {
            /* Add the node to the appropiate 'size' bucket.  Adjust
             * the max_index when appropiate.
             */
            if ((node->next = allocator->free[index]) == NULL
                && index > max_index) {
                max_index = index;
            }
            allocator->free[index] = node;
            if (current_free_index >= index + 1)
                current_free_index -= index + 1;
            else
                current_free_index = 0;
        }
        else {
            /* This node is too large to keep in a specific size bucket,
             * just add it to the sink (at index 0).
             */
            node->next = allocator->free[0];
            allocator->free[0] = node;
            if (current_free_index >= index + 1)
                current_free_index -= index + 1;
            else
                current_free_index = 0;
        }
    } while ((node = next) != NULL);

    allocator->max_index = max_index;
    allocator->current_free_index = current_free_index;

#if APR_HAS_THREADS
    if (allocator->mutex)
        apr_thread_mutex_unlock(allocator->mutex);
#endif /* APR_HAS_THREADS */

    while (freelist != NULL) {
        node = freelist;
        freelist = node->next;
#if APR_ALLOCATOR_USES_MMAP
        munmap(node, (node->index+1) << BOUNDARY_INDEX);
#else
        free(node);
#endif
    }
}

APR_DECLARE(void *) apr_allocator_alloc(apr_allocator_t *allocator,
                                                 apr_size_t size)
{
    return allocator_alloc(allocator, size)->first_avail;
}

APR_DECLARE(void) apr_allocator_free(apr_allocator_t *allocator,
                                     void *p)
{
    allocator_free(allocator, (apr_memnode_t*)( (char*)p - APR_MEMNODE_T_SIZE ));
}


#if APR_HAS_THREADS
APR_DECLARE(void) apr_allocator_mutex_set(apr_allocator_t *allocator,
                                          apr_thread_mutex_t *mutex)
{
    allocator->mutex = mutex;
}

APR_DECLARE(apr_thread_mutex_t *) apr_allocator_mutex_get(
                                      apr_allocator_t *allocator)
{
    return allocator->mutex;
}
#endif /* APR_HAS_THREADS */


apr_allocator_t* apr_global_allocator = nullptr;

namespace {
    struct initializer 
    {
        initializer(void)
        {
            apr_allocator_create(&apr_global_allocator);
        }
        ~initializer(void)
        {
            if(apr_global_allocator != nullptr)
                apr_allocator_destroy(apr_global_allocator);
        }
    };

    static initializer __internal_initializer__;
};

