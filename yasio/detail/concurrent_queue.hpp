//////////////////////////////////////////////////////////////////////////////////////////
// A cross platform socket APIs, support ios & android & wp8 & window store
// universal app
//////////////////////////////////////////////////////////////////////////////////////////
/*
The MIT License (MIT)

Copyright (c) 2012-2019 halx99

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
#if !defined(YASIO_DISABLE_SPSC_QUEUE)
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
namespace concurrency
{
template <typename _T, bool _Dual = false> class concurrent_queue;

#if !defined(YASIO_DISABLE_SPSC_QUEUE)
template <typename _T, bool _Dual> class concurrent_queue : public moodycamel::ReaderWriterQueue<_T>
{
public:
  bool empty() const { return this->peek() == nullptr; }
  void consume(size_t count, const std::function<void(_T&&)>& func)
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
public:
  template <typename... _Valty> void emplace(_Valty&&... _Val)
  {
    std::lock_guard<std::recursive_mutex> lck(this->mtx_);
    queue_.emplace(std::forward<_Valty>(_Val)...);
  }
  void pop()
  {
    std::lock_guard<std::recursive_mutex> lck(this->mtx_);
    queue_.pop();
  }
  bool empty() const { return this->queue_.empty(); }
  _T* peek()
  {
    if (this->empty())
      return nullptr;
    return &this->queue_.front();
  }
  void clear() { clear_queue(this->queue_); }

protected:
  std::queue<_T> queue_;
  std::recursive_mutex mtx_;
};
template <typename _T> class concurrent_queue<_T, false> : public concurrent_queue_primitive<_T>
{};
template <typename _T> class concurrent_queue<_T, true> : public concurrent_queue_primitive<_T>
{
public:
  void consume(size_t count, const std::function<void(_T&&)>& func)
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

    while (!this->deal_.empty() && --count >= 0)
    {
      auto event = std::move(this->deal_.front());
      deal_.pop();
      func(std::move(event));
    };
  }

  void clear()
  {
    clear_queue(queue_);
    clear_queue(deal_);
  }

private:
  std::queue<_T> deal_;
};
#endif
} // namespace concurrency
} // namespace yasio

#endif
