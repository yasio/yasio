// The optimization of auto_ptr from standard library to support deleting object appropriately.
#ifndef _SIMPLE_PTR_H_
#define _SIMPLE_PTR_H_
#include "xxfree.h"

namespace thelib {

namespace gc {

// TEMPLATE CLASS simple_ptr
template< typename _Ty, typename _Free<_Ty>::type = object_free<_Ty> >
class simple_ptr;

template<typename _Ty, typename _Free<_Ty>::type _Destroy>
class simple_ptr
{    // wrap an object pointer to ensure destruction
public:
    typedef simple_ptr<_Ty, _Destroy> _Myt;
    typedef _Ty element_type;

    explicit simple_ptr(_Ty *_Ptr = 0) throw()
        : _Myptr(_Ptr)
    {    // construct from object pointer
    }

    simple_ptr(const _Myt& _Right) throw()
        : _Myptr(const_cast<_Myt&>(_Right).release())
    {    // construct by assuming pointer from _Right simple_ptr
    }

    _Myt& operator=(const _Myt& _Right) throw()
    {    // assign compatible _Right (assume pointer)
        reset(const_cast< _Myt& >(_Right).release());
        return (*this);
    }

    template<typename _T, typename _Free<_Ty>::type _Dr>
    simple_ptr(const simple_ptr<_T, _Dr>& _Right) throw()
    {    // construct by assuming pointer from _Right
#ifdef __cxx11
        static_assert(std::is_same<_Myt, decltype(_Right)>::value, "error, the deleting behavior must be consistent!");
#endif
        _Myptr = const_cast< simple_ptr<_T, _Dr>& >(_Right).release();
    }

    template<typename _T, typename _Free<_Ty>::type _Dr>
    _Myt& operator=(const simple_ptr<_T, _Dr>& _Right) throw()
    {    // assign compatible _Right (assume pointer)
#ifdef __cxx11
        static_assert(std::is_same<_Myt, decltype(_Right)>::value, "error, the deleting behavior must be consistent!");
#endif
        reset(const_cast< simple_ptr<_T, _Dr>& >(_Right).release());
        return (*this);
    }

    ~simple_ptr()
    {    // destroy the object
        _Destroy(_Myptr);
    }

    _Ty& operator*() const throw()
    {    // return designated value
        return (*_Myptr); // return (*get());
    }

    _Ty** operator &() throw()
    {
        return &(_Myptr);
    }

    _Ty *operator->() const throw()
    {    // return pointer to class object
        return (_Myptr); // return (get());
    }

    template<typename _Int>
    _Ty& operator[](_Int index) const throw()
    {
        return (_Myptr[index]);
    }

    _Ty* get() const throw()
    {    // return wrapped pointer
        return (_Myptr);
    }

    _Ty*& ref() throw()
    {    // return wrapped pointer
        return (_Myptr);
    }

    operator _Ty*() throw()
    {
        return (_Myptr);
    }

    _Ty *release() throw()
    {    // return wrapped pointer and give up ownership
        _Ty *_Tmp = _Myptr;
        _Myptr = 0;
        return (_Tmp);
    }

    void reset(_Ty *_Ptr = 0)
    {    // destroy designated object and store new pointer
        if (_Ptr != _Myptr) 
        { _Destroy(_Myptr); }
        _Myptr = _Ptr;
    }

private:
    _Ty *_Myptr;    // the wrapped object pointer
};

template<typename _Ty>
struct single_ptr // C++ single ptr
{
    typedef simple_ptr< _Ty, object_free<_Ty> > type;
};
template<typename _Ty>
struct multiple_ptr // C++ multiple ptr
{
    typedef simple_ptr< _Ty, array_free<_Ty> > type;
};

template<typename _Ty>
struct pod_ptr // C language default ptr
{
    typedef simple_ptr< _Ty, pod_free<_Ty> > type;
};

}; // namespace: simplepp_1_13_201307::gc
}; // namespace: simplepp_1_13_201307

#endif /* _SIMPLE_PTR_ */
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

