/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef APR_ALLOCATOR_H
#define APR_ALLOCATOR_H

/**
 * @file apr_allocator.h
 * @brief APR Internal Memory Allocation
 */

//#include "apr.h"
//#include "apr_errno.h"
//#define APR_WANT_MEMFUNC /**< For no good reason? */
//#include "apr_want.h"
/// arp.h
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <string>
#include <sstream>
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


//#ifdef __cplusplus
//extern "C" {
//#endif

/**
 * @defgroup apr_allocator Internal Memory Allocation
 * @ingroup APR 
 * @{
 */

/** the allocator structure */
typedef struct apr_allocator_t apr_allocator_t;
/** the structure which holds information about the allocation */
typedef struct apr_memnode_t apr_memnode_t;

/** basic memory node structure
 * @note The next, ref and first_avail fields are available for use by the
 *       caller of apr_allocator_alloc(), the remaining fields are read-only.
 *       The next field has to be used with caution and sensibly set when the
 *       memnode is passed back to apr_allocator_free().  See apr_allocator_free()
 *       for details.  
 *       The ref and first_avail fields will be properly restored by
 *       apr_allocator_free().
 */
struct apr_memnode_t {
    apr_memnode_t *next;            /**< next memnode */
    apr_memnode_t **ref;            /**< reference to self */
    apr_uint32_t   index;           /**< size */
    apr_uint32_t   free_index;      /**< how much free */
    char          *first_avail;     /**< pointer to first free memory */
    char          *endp;            /**< pointer to end of free memory */
};

/** The base size of a memory node - aligned.  */
#define APR_MEMNODE_T_SIZE APR_ALIGN_DEFAULT(sizeof(apr_memnode_t))

/** Symbolic constants */
#define APR_ALLOCATOR_MAX_FREE_UNLIMITED 0

/**
 * Create a new allocator
 * @param allocator The allocator we have just created.
 *
 */
APR_DECLARE(apr_status_t) apr_allocator_create(apr_allocator_t **allocator)
                          /*__attribute__((nonnull(1)))*/;

/**
 * Destroy an allocator
 * @param allocator The allocator to be destroyed
 * @remark Any memnodes not given back to the allocator prior to destroying
 *         will _not_ be free()d.
 */
APR_DECLARE(void) apr_allocator_destroy(apr_allocator_t *allocator)
                  /*__attribute__((nonnull(1)))*/;

/**
 * Allocate a block of mem from the allocator
 * @param allocator The allocator to allocate from
 * @param size The size of the mem to allocate (excluding the
 *        memnode structure)
 */
APR_DECLARE(void *) apr_allocator_alloc(apr_allocator_t *allocator,
                                                 apr_size_t size)
                             /*__attribute__((nonnull(1)))*/;

/**
 * Free a list of blocks of mem, giving them back to the allocator.
 * The list is typically terminated by a memnode with its next field
 * set to NULL.
 * @param allocator The allocator to give the mem back to
 * @param memnode The memory node to return
 */
APR_DECLARE(void) apr_allocator_free(apr_allocator_t *allocator,
                                     void * p)
                  /*__attribute__((nonnull(1,2)))*/;

/**
 * Set the current threshold at which the allocator should start
 * giving blocks back to the system.
 * @param allocator The allocator to set the threshold on
 * @param size The threshold.  0 == unlimited.
 */
APR_DECLARE(void) apr_allocator_max_free_set(apr_allocator_t *allocator,
                                             apr_size_t size)
                  /*__attribute__((nonnull(1)))*/;

// #include "apr_thread_mutex.h" cancel thread detect

#if APR_HAS_THREADS
/**
 * Set a mutex for the allocator to use
 * @param allocator The allocator to set the mutex for
 * @param mutex The mutex
 */
APR_DECLARE(void) apr_allocator_mutex_set(apr_allocator_t *allocator,
                                          apr_thread_mutex_t *mutex)
                  __attribute__((nonnull(1)));

/**
 * Get the mutex currently set for the allocator
 * @param allocator The allocator
 */
APR_DECLARE(apr_thread_mutex_t *) apr_allocator_mutex_get(
                                          apr_allocator_t *allocator)
                                  __attribute__((nonnull(1)));

#endif /* APR_HAS_THREADS */

/** @} */

//#ifdef __cplusplus
//}
//#endif

extern apr_allocator_t* apr_global_allocator;

//// allocator
namespace apr {
    // TEMPLATE CLASS object_pool_alloctor, can't used by std::vector
template<class _Ty>
class allocator
{	// generic allocator for objects of class _Ty
public:
    typedef _Ty value_type;

    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type* const_pointer;
    typedef const value_type& const_reference;

    typedef size_t size_type;
#ifdef _WIN32
    typedef ptrdiff_t difference_type;
#else
    typedef long  difference_type;
#endif

    template<class _Other>
    struct rebind
    {	// convert this type to _ALLOCATOR<_Other>
        typedef allocator<_Other> other;
    };

    pointer address(reference _Val) const
    {	// return address of mutable _Val
        return ((pointer) &(char&)_Val);
    }

    const_pointer address(const_reference _Val) const
    {	// return address of nonmutable _Val
        return ((const_pointer) &(char&)_Val);
    }

    allocator() throw()
    {	// construct default allocator (do nothing)
    }

    allocator(const allocator<_Ty>&) throw()
    {	// construct by copying (do nothing)
    }

    template<class _Other>
    allocator(const allocator<_Other>&) throw()
    {	// construct from a related allocator (do nothing)
    }

    template<class _Other>
    allocator<_Ty>& operator=(const allocator<_Other>&)
    {	// assign from a related allocator (do nothing)
        return (*this);
    }

    void deallocate(pointer _Ptr, size_type)
    {	// deallocate object at _Ptr, ignore size
        apr_allocator_free(apr_global_allocator, _Ptr);
    }

    pointer allocate(size_type count)
    {	// allocate array of _Count elements
        return static_cast<pointer>( apr_allocator_alloc(apr_global_allocator, (sizeof(_Ty) * count) ) );
    }

    pointer allocate(size_type count, const void*)
    {	// allocate array of _Count elements, not support, such as std::vector
        return allocate(count);
    }

    void construct(pointer _Ptr, const _Ty& _Val)
    {	// construct object at _Ptr with value _Val
        new (_Ptr) _Ty(_Val);
    }

#ifdef __cxx0x
    void construct(pointer _Ptr, _Ty&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std:: forward<_Ty>(_Val));
    }

    template<class _Other>
    void construct(pointer _Ptr, _Other&& _Val)
    {	// construct object at _Ptr with value _Val
        new ((void*)_Ptr) _Ty(std:: forward<_Other>(_Val));
    }
#endif

    template<class _Uty>
    void destroy(_Uty *_Ptr)
    {	// destroy object at _Ptr, do nothing, because destructor will called in _Mempool.release(_Ptr)
        _Ptr->~_Uty();
    }

    size_type max_size() const throw()
    {	// estimate maximum array size
        size_type _Count = (size_type)(-1) / sizeof (_Ty);
        return (0 < _Count ? _Count : 1);
    }
}; // class template apr::allocator

typedef std::basic_string<char, std::char_traits<char>, apr::allocator<char>> string;
typedef std::basic_stringstream<char, std::char_traits<char>, apr::allocator<char>> stringstream;
typedef std::basic_istringstream<char, std::char_traits<char>, apr::allocator<char>> istringstream;
typedef std::basic_ostringstream<char, std::char_traits<char>, apr::allocator<char>> ostringstream;

}; // namespace apr

#endif /* !APR_ALLOCATOR_H */
