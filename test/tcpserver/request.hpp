//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2013-2014 xseekerj
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_REQUEST_HPP
#define HTTP_REQUEST_HPP

#include <string>
#include <vector>

#include "pcode_autog_server_messages.h"
#include "tls.hpp"

namespace tcp {
    namespace server {


        // TEMPLATE CLASS object_pool_allocator, can't used by std::vector
        template<class _Ty>
        class mempool2_allocator
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
                typedef mempool2_allocator<_Other> other;
            };

            pointer address(reference _Val) const
            {	// return address of mutable _Val
                return ((pointer) &(char&)_Val);
            }

            const_pointer address(const_reference _Val) const
            {	// return address of nonmutable _Val
                return ((const_pointer) &(char&)_Val);
            }

            mempool2_allocator() throw()
            {	// construct default allocator (do nothing)
            }

            mempool2_allocator(const mempool2_allocator<_Ty>&) throw()
            {	// construct by copying (do nothing)
            }

            template<class _Other>
            mempool2_allocator(const mempool2_allocator<_Other>&) throw()
            {	// construct from a related allocator (do nothing)
            }

            template<class _Other>
            mempool2_allocator<_Ty>& operator=(const mempool2_allocator<_Other>&)
            {	// assign from a related allocator (do nothing)
                return (*this);
            }

            void deallocate(pointer _Ptr, size_type)
            {	// deallocate object at _Ptr, ignore size
                gls_release_buffer(_Ptr);
            }

            pointer allocate(size_type count)
            {	// allocate array of _Count elements
                return static_cast<pointer>(gls_get_buffer(count));
            }

            pointer allocate(size_type count, const void*)
            {	// allocate array of _Count elements, not support, such as std::vector
                return allocate(count);
            }

            void construct(_Ty *_Ptr)
            {	// default construct object at _Ptr
                ::new ((void *)_Ptr) _Ty();
            }

            void construct(pointer _Ptr, const _Ty& _Val)
            {	// construct object at _Ptr with value _Val
                new (_Ptr) _Ty(_Val);
            }

#ifdef __cxx0x
            void construct(pointer _Ptr, _Ty&& _Val)
            {	// construct object at _Ptr with value _Val
                new ((void*)_Ptr) _Ty(std::forward<_Ty>(_Val));
            }

            template<class _Other>
            void construct(pointer _Ptr, _Other&& _Val)
            {	// construct object at _Ptr with value _Val
                new ((void*)_Ptr) _Ty(std::forward<_Other>(_Val));
            }

            template<class _Objty,
                class... _Types>
                void construct(_Objty *_Ptr, _Types&&... _Args)
            {	// construct _Objty(_Types...) at _Ptr
                ::new ((void *)_Ptr) _Objty(std::forward<_Types>(_Args)...);
            }
#endif

            template<class _Uty>
            void destroy(_Uty *_Ptr)
            {	// destroy object at _Ptr, do nothing, because destructor will called in _Mempool.release(_Ptr)
                _Ptr->~_Uty();
            }

            size_type max_size() const throw()
            {	// estimate maximum array size
                size_type _Count = (size_type)(-1) / sizeof(_Ty);
                return (0 < _Count ? _Count : 1);
            }
        };

        template<class _Ty,
            class _Other> inline
            bool operator==(const mempool2_allocator<_Ty>&,
                const mempool2_allocator<_Other>&) throw()
        {	// test for allocator equality
            return (true);
        }

        template<class _Ty,
            class _Other> inline
            bool operator!=(const mempool2_allocator<_Ty>& _Left,
                const mempool2_allocator<_Other>& _Right) throw()
        {	// test for allocator inequality
            return (!(_Left == _Right));
        }

        // typedef std::basic_string<char, std::char_traits<char>, mempool2_allocator<char>> buffer_type;
        typedef std::string buffer_type;

        /// A request received from a client.
        struct request
        {
            request()
            {
                memset(&header_, 0, sizeof(header_));
                offset_ = 0;

                reset();
            }

            void reset()
            {
                expected_size_ = -1;
                receiving_pdu_.clear();
            }

            char                   buffer_[1024];
            int                    offset_;  // 已收到包总字节数

            messages::MsgHeader    header_;
            int                    expected_size_; // 期望包总字节数
            buffer_type receiving_pdu_;
        };

    } // namespace server
} // namespace http

#endif // HTTP_REQUEST_HPP
