//
// Copyright (c) 2014 purelib - All Rights Reserved
//
#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include <new>
#include <memory>
#include <functional>

#if defined(_ENABLE_MULTITHREAD)
#include <mutex>
#endif

namespace purelib {
    namespace gc {

        // CLASS TEMPLATE singleton forward decl
        template<typename _Ty, bool delayed = false>
        class singleton;

        /// CLASS TEMPLATE singleton
        /// the managed singleton object will be destructed after main function.
        template<typename _Ty>
        class singleton < _Ty, false >
        {
            typedef singleton<_Ty, false> _Myt;
            typedef _Ty* pointer;
        public:
            template<typename ..._Args>
            static pointer instance(_Args...args)
            {
                if (nullptr == _Myt::__single__.get())
                {
#if defined(_ENABLE_MULTITHREAD)
                    _Myt::__mutex__.lock();
#endif
                    if (nullptr == _Myt::__single__.get())
                    {
                        _Myt::__single__.reset(new(std::nothrow) _Ty(args...));
                    }
#if defined(_ENABLE_MULTITHREAD)
                    _Myt::__mutex__.unlock();
#endif
                }
                return _Myt::__single__.get();
            }

            static void destroy(void)
            {
                if (_Myt::__single__.get() != nullptr)
                {
                    _Myt::__single__.reset();
                }
            }

        private:
            static std::unique_ptr<_Ty> __single__;
#if defined(_ENABLE_MULTITHREAD)
            static std::mutex    __mutex__;
#endif
        private:
            singleton(void) = delete; // just disable construct, assign operation, copy construct also not allowed.
        };

        /// CLASS TEMPLATE singleton, support delay init with variadic args
        /// the managed singleton object will be destructed after main function.
        template<typename _Ty>
        class singleton < _Ty, true >
        {
            typedef singleton<_Ty, true> _Myt;
            typedef _Ty* pointer;
        public:
            template<typename ..._Args>
            static pointer instance(_Args...args)
            {
                if (nullptr == _Myt::__single__.get())
                {
#if defined(_ENABLE_MULTITHREAD)
                    _Myt::__mutex__.lock();
#endif
                    if (nullptr == _Myt::__single__.get())
                    {
                        _Myt::__single__.reset(new(std::nothrow) _Ty());
                        if (_Myt::__single__ != nullptr)
                            _Myt::delay_init(args...);
                    }
#if defined(_ENABLE_MULTITHREAD)
                    _Myt::__mutex__.unlock();
#endif
                }
                return _Myt::__single__.get();
            }

            static void destroy(void)
            {
                if (_Myt::__single__.get() != nullptr)
                {
                    _Myt::__single__.reset();
                }
            }

        private:

            template<typename _Fty, typename..._Args>
            static void delay_init(const _Fty& memf, _Args...args)
            { // init use specific member func with more than 1 args
                std::mem_fn(memf)(_Myt::__single__.get(), args...);
            }

            template<typename _Fty, typename _Arg>
            static void delay_init(const _Fty& memf, const _Arg& arg)
            { // init use specific member func with 1 arg
                std::mem_fun(memf)(_Myt::__single__.get(), arg);
            }

            template<typename _Fty>
            static void delay_init(const _Fty& memf)
            { // init use specific member func without arg
                std::mem_fun(memf)(_Myt::__single__.get());
            }

            static void delay_init(void)
            { // dummy init
            }

        private:
            static std::unique_ptr<_Ty> __single__;
#if defined(_ENABLE_MULTITHREAD)
            static std::mutex    __mutex__;
#endif
        private:
            singleton(void) = delete; // just disable construct, assign operation, copy construct also not allowed.
        };

        template<typename _Ty>
        std::unique_ptr<_Ty> singleton<_Ty, false>::__single__;
#if defined(_ENABLE_MULTITHREAD)
        template<typename _Ty>
        std::mutex    singleton<_Ty, false>::__mutex__;
#endif

        template<typename _Ty>
        std::unique_ptr<_Ty> singleton<_Ty, true>::__single__;
#if defined(_ENABLE_MULTITHREAD)
        template<typename _Ty>
        std::mutex    singleton<_Ty, true>::__mutex__;
#endif

        // TEMPLATE alias
        template<typename _Ty>
        using delayed = singleton < _Ty, true > ;
    };
};

#endif

/*
* Copyright (c) 2012-2014 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

