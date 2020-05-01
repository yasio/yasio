//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2020 HALX99

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef YASIO__CONCURRENT_QUEUE_HPP
#define YASIO__CONCURRENT_QUEUE_HPP

#include "yasio/detail/config.hpp"
#if defined(YASIO_USE_SPSC_QUEUE)
#  include "yasio/moodycamel/readerwriterqueue.h"
#else
#  include <queue>
#endif

namespace yasio
{
template <typename _T> inline void clear_queue(_T& queue)
{
  _T tmp;
  std::swap(tmp, queue);
}
namespace privacy
{
template <typename _T, bool _Dual = false> class concurrent_queue;

#if defined(YASIO_USE_SPSC_QUEUE)
template <typename _T, bool _Dual> class concurrent_queue : public moodycamel::ReaderWriterQueue<_T>
{
public:
  bool empty() const { return this->peek() == nullptr; }
  void consume(int count, const std::function<void(_T&&)>& func)
  {
    _T event;
    while (count-- > 0 && this->try_dequeue(event))
      func(std::move(event));
  }
  void clear() { clear_queue(static_cast<moodycamel::ReaderWriterQueue<_T>&>(*this)); }
};

#else

template <typename _T> class concurrent_queue_primitive
{
  struct concurrent_item
  {
  public:
    concurrent_item(std::recursive_mutex& mtx, std::queue<_T>& queue)
        : mtx_(mtx), queue_(queue), pitem_(nullptr)
    {}
    ~concurrent_item()
    {
      if (pitem_ != nullptr)
        mtx_.unlock();
    }

    explicit operator bool()
    {
      if (!queue_.empty())
      {
        mtx_.lock();
        if (!queue_.empty())
          pitem_ = &queue_.front();
        else
          mtx_.unlock();
        return pitem_ != nullptr;
      }
      return false;
    }

    _T& operator*() { return *pitem_; }

  private:
    std::recursive_mutex& mtx_;
    std::queue<_T>& queue_;
    _T* pitem_;
  };

public:
  template <typename... _Valty> void emplace(_Valty&&... _Val)
  {
    std::lock_guard<std::recursive_mutex> lck(this->mtx_);
    queue_.emplace(std::forward<_Valty>(_Val)...);
  }

  void pop() { queue_.pop(); }
  bool empty() const { return this->queue_.empty(); }
  void clear() { clear_queue(this->queue_); }

  // peek item to read/write thread safe
  concurrent_item peek() { return concurrent_item{mtx_, queue_}; }

protected:
  std::queue<_T> queue_;
  std::recursive_mutex mtx_;
};
template <typename _T> class concurrent_queue<_T, false> : public concurrent_queue_primitive<_T>
{};
template <typename _T> class concurrent_queue<_T, true> : public concurrent_queue_primitive<_T>
{
public:
  void consume(int count, const std::function<void(_T&&)>& func)
  {
    if (this->deal_.empty())
    {
      if (this->empty())
        return;
      else
      {
        // swap event queue
        std::lock_guard<std::recursive_mutex> lck(this->mtx_);
        std::swap(this->deal_, this->queue_);
      }
    }

    while (count-- > 0 && !this->deal_.empty())
    {
      auto event = std::move(this->deal_.front());
      deal_.pop();
      func(std::move(event));
    };
  }

  void clear()
  {
    concurrent_queue_primitive<_T>::clear();
    clear_queue(deal_);
  }

private:
  std::queue<_T> deal_;
};
#endif
} // namespace privacy
} // namespace yasio

#endif
