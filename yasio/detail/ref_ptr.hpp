//
// Copyright (c) 2014-2020 HALX99 - All Rights Reserved
//
#ifndef YASIO__REF_PTR_HPP
#define YASIO__REF_PTR_HPP
#include <iostream>

#define __SAFE_RELEASE(p)                                                                          \
  do                                                                                               \
  {                                                                                                \
    if (p)                                                                                         \
    {                                                                                              \
      (p)->release();                                                                              \
    }                                                                                              \
  } while (0)
#define __SAFE_RELEASE_NULL(p)                                                                     \
  do                                                                                               \
  {                                                                                                \
    if (p)                                                                                         \
    {                                                                                              \
      (p)->release();                                                                              \
      (p) = nullptr;                                                                               \
    }                                                                                              \
  } while (0)
#define __SAFE_RETAIN(p)                                                                           \
  do                                                                                               \
  {                                                                                                \
    if (p)                                                                                         \
    {                                                                                              \
      (p)->retain();                                                                               \
    }                                                                                              \
  } while (0)

namespace yasio
{
namespace gc
{

// TEMPLATE CLASS, equals to cocos2d-x-3.x cocos2d::RefPtr
template <typename _Ty> class ref_ptr;

template <typename _Ty> class ref_ptr
{ // wrap an object pointer to ensure destruction
public:
  typedef ref_ptr<_Ty> _Myt;
  typedef _Ty element_type;

  explicit ref_ptr(_Ty* _Ptr = 0) throw() : ptr_(_Ptr)
  { // construct from object pointer
  }

  ref_ptr(std::nullptr_t) throw() : ptr_(0) {}

  ref_ptr(const _Myt& _Right) throw()
  { // construct by assuming pointer from _Right ref_ptr
    __SAFE_RETAIN(_Right.get());

    ptr_ = _Right.get();
  }

  template <typename _Other> ref_ptr(const ref_ptr<_Other>& _Right) throw()
  { // construct by assuming pointer from _Right
    __SAFE_RETAIN(_Right.get());

    ptr_ = (_Ty*)_Right.get();
  }

  ref_ptr(_Myt&& _Right) throw()
  {
    __SAFE_RETAIN(_Right.get());

    ptr_ = (_Ty*)_Right.get();
  }

  template <typename _Other> ref_ptr(ref_ptr<_Other>&& _Right) throw()
  { // construct by assuming pointer from _Right
    __SAFE_RETAIN(_Right.get());

    ptr_ = (_Ty*)_Right.get();
  }

  _Myt& operator=(const _Myt& _Right) throw()
  { // assign compatible _Right (assume pointer)
    if (this == &_Right)
      return *this;

    __SAFE_RETAIN(_Right.get());

    reset(_Right.get());
    return (*this);
  }

  _Myt& operator=(_Myt&& _Right) throw()
  { // assign compatible _Right (assume pointer)

    __SAFE_RETAIN(_Right.get());

    reset(_Right.get());
    return (*this);
  }

  template <typename _Other> _Myt& operator=(const ref_ptr<_Other>& _Right) throw()
  { // assign compatible _Right (assume pointer)
    if (this == &_Right)
      return *this;

    __SAFE_RETAIN(_Right.get());

    reset((_Ty*)_Right.get());
    return (*this);
  }

  template <typename _Other> _Myt& operator=(ref_ptr<_Other>&& _Right) throw()
  { // assign compatible _Right (assume pointer)
    __SAFE_RETAIN(_Right.get());

    reset((_Ty*)_Right.get());
    return (*this);
  }

  _Myt& operator=(std::nullptr_t) throw()
  {
    reset();
    return (*this);
  }

  ~ref_ptr()
  { // release the object
    __SAFE_RELEASE(ptr_);
  }

  _Ty& operator*() const throw()
  {                 // return designated value
    return (*ptr_); // return (*get());
  }

  _Ty** operator&() throw() { return &(ptr_); }

  _Ty* operator->() const throw()
  {                // return pointer to class object
    return (ptr_); // return (get());
  }

  template <typename _Int> _Ty& operator[](_Int index) const throw() { return (ptr_[index]); }

  _Ty* get() const throw()
  { // return wrapped pointer
    return (ptr_);
  }

  _Ty*& get_ref() throw()
  { // return wrapped pointer
    return (ptr_);
  }

  operator _Ty*() const throw()
  { // convert to basic type
    return (ptr_);
  }

  /*
  ** if already have a valid pointer, will call release firstly
  */
  void reset(_Ty* _Ptr = 0)
  { // relese designated object and store new pointer
    if (ptr_ != _Ptr)
    {

      if (ptr_ != nullptr)
      {
        if (ptr_ != _Ptr) // release old
          __SAFE_RELEASE(ptr_);
      }

      ptr_ = _Ptr;
    }
  }

private:
  _Ty* ptr_; // the wrapped object pointer
};

}; // namespace gc

}; // namespace yasio

#endif
