//
// Copyright (c) 2014-2015 purelib - All Rights Reserved
//
#ifndef _XXREF_PTR_H_
#define _XXREF_PTR_H_
#include <iostream>

#define __SAFE_RELEASE(p)          do { if(p) { (p)->release(); } } while(0)
#define __SAFE_RELEASE_NULL(p)     do { if(p) { (p)->release(); (p) = nullptr; } } while(0)
#define __SAFE_RETAIN(p)           do { if(p) { (p)->retain(); } } while(0)

namespace purelib {

template<typename _Ty> inline
void ccReleaseObject(_Ty* ccObject)
{
    __SAFE_RELEASE(ccObject);
}

namespace gc {

// TEMPLATE CLASS, equals to cocos2d-x-3.x cocos2d::RefPtr
template< typename _Ty>
class ref_ptr;

template<typename _Ty>
class ref_ptr
{    // wrap an object pointer to ensure destruction
public:
    typedef ref_ptr<_Ty> _Myt;
    typedef _Ty element_type;

    explicit ref_ptr(_Ty *_Ptr = 0) throw()
        : ref(_Ptr)
    {    // construct from object pointer
    }

    ref_ptr(std::nullptr_t) throw() 
        : ref(0)
    {
    }

    ref_ptr(const _Myt& _Right) throw()
    {    // construct by assuming pointer from _Right ref_ptr
        __SAFE_RETAIN(_Right.get());

        ref = _Right.get();
    }

    template<typename _Other>
    ref_ptr(const ref_ptr<_Other>& _Right) throw()
    {    // construct by assuming pointer from _Right
        __SAFE_RETAIN(_Right.get());

        ref = (_Ty*)_Right.get();
    }

    ref_ptr(_Myt&& _Right) throw()
    {
        __SAFE_RETAIN(_Right.get());

        ref = (_Ty*)_Right.get();
    }

    template<typename _Other>
    ref_ptr(ref_ptr<_Other>&& _Right) throw()
    {    // construct by assuming pointer from _Right
        __SAFE_RETAIN(_Right.get());

        ref = (_Ty*)_Right.get();
    }

    _Myt& operator=(const _Myt& _Right) throw()
    {    // assign compatible _Right (assume pointer)
        if(this == &_Right)
            return *this;

        __SAFE_RETAIN(_Right.get());

        reset( _Right.get() );
        return (*this);
    }

    _Myt& operator=(_Myt&& _Right) throw()
    {    // assign compatible _Right (assume pointer)

        __SAFE_RETAIN(_Right.get());

        reset( _Right.get() );
        return (*this);
    }

    template<typename _Other>
    _Myt& operator=(const ref_ptr<_Other>& _Right) throw()
    {    // assign compatible _Right (assume pointer)
        if(this == &_Right)
            return *this;

        __SAFE_RETAIN(_Right.get());

        reset( (_Ty*)_Right.get() );
        return (*this);
    }

    template<typename _Other>
    _Myt& operator=(ref_ptr<_Other>&& _Right) throw()
    {    // assign compatible _Right (assume pointer)
        __SAFE_RETAIN(_Right.get());

        reset( (_Ty*)_Right.get() );
        return (*this);
    }

    _Myt& operator=(std::nullptr_t) throw()
    {
        reset();
        return (*this);
    }

    ~ref_ptr()
    {    // release the object
        __SAFE_RELEASE(ref);
    }

    _Ty& operator*() const throw()
    {    // return designated value
        return (*ref); // return (*get());
    }

    _Ty** operator &() throw()
    {
        return &(ref);
    }

    _Ty *operator->() const throw()
    {    // return pointer to class object
        return (ref); // return (get());
    }

    template<typename _Int>
    _Ty& operator[](_Int index) const throw()
    {
        return (ref[index]);
    }

    _Ty* get() const throw()
    {    // return wrapped pointer
        return (ref);
    }

    _Ty*& get_ref() throw()
    {    // return wrapped pointer
        return (ref);
    }

    operator _Ty*() const throw()
    { // convert to basic type
        return (ref);
    }

    /*
    ** if already have a valid pointer, will call release firstly
    */
    void reset(_Ty *_Ptr = 0) 
    {    // relese designated object and store new pointer
		if (ref != _Ptr) {

			if (ref != nullptr)
			{
				if (ref != _Ptr) // release old
					__SAFE_RELEASE(ref);
			}

			ref = _Ptr;
		}
    }

private:
    _Ty* ref;    // the wrapped object pointer
};

}; /* namespace purelib::gc */

}; /* namespace purelib */

using namespace purelib::gc;

#endif
