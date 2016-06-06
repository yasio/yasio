#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include "simple_ptr.h"
#define _ENABLE_MULTITHREAD
#if defined(_ENABLE_MULTITHREAD)
#include <mutex>
#endif


namespace thelib {
namespace gc {

    template<typename _Ty>
    class singleton
    {
    public:
        static _Ty* instance(void)
        {
            if(nullptr == singleton<_Ty>::__single__.get())
            {
#if defined(_ENABLE_MULTITHREAD)
                singleton<_Ty>::__mutex__.lock();
#endif
                if(nullptr == singleton<_Ty>::__single__.get())
                {
                    singleton<_Ty>::__single__.reset(new _Ty);
                }
#if defined(_ENABLE_MULTITHREAD)
                singleton<_Ty>::__mutex__.unlock();
#endif
            }
            return singleton<_Ty>::__single__.get();
        }

        static void destroy(void)
        {
            if(singleton<_Ty>::__single__.get() != nullptr)
            {
                singleton<_Ty>::__single__.reset();
            }
        }

    private:
        static simple_ptr<_Ty> __single__;
#if defined(_ENABLE_MULTITHREAD)
        static std::mutex    __mutex__;
#endif
    };

	template<typename _Ty>
	simple_ptr<_Ty> singleton<_Ty>::__single__;
#if defined(_ENABLE_MULTITHREAD)
    template<typename _Ty>
    std::mutex    singleton<_Ty>::__mutex__;
#endif
};

};

#endif

/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

