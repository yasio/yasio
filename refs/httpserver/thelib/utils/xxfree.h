#ifndef _XXFREE_H_
#define _XXFREE_H_
#include <stdlib.h>

namespace thelib {

namespace gc {

// TEMPLATE STRUCT destroyer
template<typename _Ty>
struct _Free
{
	typedef void (*type)(_Ty*); 
};

// c++ single object deleter
template<typename _Ty>
void object_free(_Ty* _Ptr)
{
	delete _Ptr;
}

// c++ objects array free function template
template<typename _Ty>
void array_free(_Ty* _Ptr)
{
	delete [] _Ptr;
}

// pod type object free function template
template<typename _Ty>
void pod_free(_Ty* _Ptr)
{
	free(_Ptr);
}

// pseudo free, do not influence the object
template<typename _Ty>
void pseudo_free(_Ty*)
{
}

/// template free v2
	// TEMPLATE CLASS integral_constant
template<class _Ty,
	_Ty _Val>
	struct integral_constant
	{	// convenient template for integral constant types
	static const _Ty value = _Val;

	typedef _Ty value_type;
	typedef integral_constant<_Ty, _Val> type;
	};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

// object cleaner, do not influence the object
template<typename _Ty>
struct object_cleaner : true_type
{
    static void cleanup(_Ty* _Ptr)
    {
        delete _Ptr;
    }
};

// array cleaner, do not influence the object
template<typename _Ty>
struct array_cleaner : true_type
{
    static void cleanup(_Ty* _Ptr)
    {
        delete [] _Ptr;
    }
};

// pod cleaner, do not influence the object
template<typename _Ty>
struct pod_cleaner : true_type
{
    static void cleanup(_Ty* _Ptr)
    {
        free(_Ptr);
    }
};

// pseudo cleaner, do not influence the object
template<typename _Ty>
struct pseudo_cleaner : false_type
{
    static void cleanup(_Ty*)
    {
    }
};

}; // namespace: thelib::gc
}; // namespace: thelib

#endif
 /* _XXFREE_H_ */
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

