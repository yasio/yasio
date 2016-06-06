// container_helper standard header, contains several methods to help clear dynamic store of std-container.
#ifndef _CONTAINER_HELPER_H_
#define _CONTAINER_HELPER_H_
#include <algorithm>

namespace thelib {

namespace container_helper {

template<typename _Conty> inline
    void clear_c(_Conty& _Obj)
{
    typename _Conty::iterator _First = _Obj.begin();
    typename _Conty::iterator _Last = _Obj.end();
    for (; _First != _Last; ++_First) {
        delete *_First;
    }
    _Obj.clear();
}

template<typename _Conty, typename _RelFn1> inline
    void clear_c(_Conty& _Obj, _RelFn1 _Relase)
{
    typename _Conty::iterator _First = _Obj.begin();
    typename _Conty::iterator _Last = _Obj.end();
    for (; _First != _Last; ++_First) {
        _Relase(*_First);
    }
    _Obj.clear();
}

template<typename _CConty> inline
    void clear_cc(_CConty& _Obj)
{
    typename _CConty::iterator _First = _Obj.begin();
    typename _CConty::iterator _Last = _Obj.end();
    for (; _First != _Last; ++_First) {
        delete _First->second;
    }
    _Obj.clear();
}

template<typename _CConty, typename _RelFn1> inline
    void clear_cc(_CConty& _Obj, _RelFn1 _Relase)
{
    typename _CConty::iterator _First = _Obj.begin();
    typename _CConty::iterator _Last = _Obj.end();
    for (; _First != _Last; ++_First) {
        _Relase(_First->second);
    }
    _Obj.clear();
}

/// Only work for Associative Container
template<typename _Conty, typename _Fty>
void erase_if(_Conty& _Container, _Fty _Cond)
{
    typename _Conty::iterator _First = _Container.begin();
    typename _Conty::iterator _Last = _Container.end();
    for ( ; _First != _Last; ) {
        if( _Cond(*_First) ) {
            _Container.erase(_First++);
            continue;
        }
        ++_First;
    }
}

/// Only work for Sequence Container
template<typename _Conty, typename _Fty>
void remove_if(_Conty& _Container, _Fty _Cond)
{
    _Container.erase(std::remove_if(_Container.begin(), _Container.end(), _Cond), _Container.end());
}

template<typename _InIt, typename _Fn1> inline
    _Fn1 for_each2(_InIt _First, _InIt _Last, _Fn1 _Func)
{    // perform function for each 2nd element
    for (; _First != _Last; ++_First)
        _Func(_First->second);
    return (_Func);
}

template<typename _Container> inline
void push_back_to(const typename _Container::value_type& val, _Container& container)
{
    container.push_back(val);
}

template<typename _Container> inline
void insert_into(const typename _Container::value_type& val, _Container& container)
{
    container.insert(val);
}

}; // namespace: thelib::container_helper
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

