//
// Copyright (c) 2014-2019 HALX99 - All Rights Reserved
//
#ifndef YASIO__SINGLETON_HPP
#define YASIO__SINGLETON_HPP
#include <new>
#include <memory>
#include <functional>

#if defined(_ENABLE_MULTITHREAD)
#  include <mutex>
#endif

namespace yasio
{
namespace gc
{

// CLASS TEMPLATE singleton forward decl
template <typename _Ty, bool delayed = false> class singleton;

/// CLASS TEMPLATE singleton
/// the managed singleton object will be destructed after main function.
template <typename _Ty> class singleton<_Ty, false>
{
  typedef singleton<_Ty, false> _Myt;
  typedef _Ty* pointer;

public:
  template <typename... _Args> static pointer instance(_Args&&... args)
  {
    if (_Myt::__single__)
      return _Myt::__single__.get();

#if defined(_ENABLE_MULTITHREAD)
    _Myt::__mutex__.lock();
#endif
    _Myt::__single__.reset(new (std::nothrow) _Ty(args...));
#if defined(_ENABLE_MULTITHREAD)
    _Myt::__mutex__.unlock();
#endif

    return _Myt::__single__.get();
  }

  static void destroy(void)
  {
    if (_Myt::__single__)
    {
      _Myt::__single__.reset();
    }
  }

private:
  static std::unique_ptr<_Ty> __single__;
#if defined(_ENABLE_MULTITHREAD)
  static std::mutex __mutex__;
#endif
private:
  // static class: disable construct, assign operation, copy construct also not allowed.
  singleton(void) = delete;
};

/// CLASS TEMPLATE singleton, support delay init with variadic args
/// the managed singleton object will be destructed after main function.
template <typename _Ty> class singleton<_Ty, true>
{
  typedef singleton<_Ty, true> _Myt;
  typedef _Ty* pointer;

public:
  template <typename... _Args> static pointer instance(_Args&&... args)
  {
    if (_Myt::__single__)
      return _Myt::__single__.get();

#if defined(_ENABLE_MULTITHREAD)
    _Myt::__mutex__.lock();
#endif

    _Myt::__single__.reset(new (std::nothrow) _Ty());
    if (_Myt::__single__)
      _Myt::delay_init(args...);
#if defined(_ENABLE_MULTITHREAD)
    _Myt::__mutex__.unlock();
#endif
    return _Myt::__single__.get();
  }

  static void destroy(void)
  {
    if (_Myt::__single__)
    {
      _Myt::__single__.reset();
    }
  }

private:
  template <typename _Fty, typename... _Args> static void delay_init(_Fty&& memf, _Args&&... args)
  { // init use specific member func with more than 1 args
    std::mem_fn(memf)(_Myt::__single__.get(), args...);
  }

  template <typename _Fty, typename _Arg> static void delay_init(_Fty&& memf, _Arg&& arg)
  { // init use specific member func with 1 arg
    std::mem_fn(memf)(_Myt::__single__.get(), arg);
  }

  template <typename _Fty> static void delay_init(_Fty&& memf)
  { // init use specific member func without arg
    std::mem_fn(memf)(_Myt::__single__.get());
  }

  static void delay_init(void)
  { // dummy init
  }

private:
  static std::unique_ptr<_Ty> __single__;
#if defined(_ENABLE_MULTITHREAD)
  static std::mutex __mutex__;
#endif
private:
  singleton(void) =
      delete; // just disable construct, assign operation, copy construct also not allowed.
};

template <typename _Ty> std::unique_ptr<_Ty> singleton<_Ty, false>::__single__;
#if defined(_ENABLE_MULTITHREAD)
template <typename _Ty> std::mutex singleton<_Ty, false>::__mutex__;
#endif

template <typename _Ty> std::unique_ptr<_Ty> singleton<_Ty, true>::__single__;
#if defined(_ENABLE_MULTITHREAD)
template <typename _Ty> std::mutex singleton<_Ty, true>::__mutex__;
#endif

// TEMPLATE alias
template <typename _Ty> using delayed = singleton<_Ty, true>;
} // namespace gc
} // namespace yasio

#endif
