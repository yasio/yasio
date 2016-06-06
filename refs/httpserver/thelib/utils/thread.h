/**** Notes:
*  When the object hold by thread will be destroyed , 
*  the thread must already exit or no longer dependent on any 
*  resources of the object.
*/
#ifndef _THREAD_H_
#define _THREAD_H_
#include <functional>
#include <ostream>
#include "thread_basic.h"
#define THREAD_MAGIC_NUMBER 19881219

namespace thelib { 

namespace asy {

int  _Thrd_create(_Thrd_t *, _Thrd_start_t, void *);

int  _Thrd_detach(_Thrd_t);
void  _Thrd_exit(int);
int  _Thrd_join(_Thrd_t, int *);
void  _Thrd_yield(void);
int  _Thrd_equal(_Thrd_t, _Thrd_t);
int  _Thrd_lt(_Thrd_t, _Thrd_t);
_Thrd_t  _Thrd_current(void);

#define _Thrd_startX _Thrd_create
#define _Thrd_detachX _Thrd_detach

class _Pad
	{	// base class for launching threads
public:
	_Pad();
	~_Pad() throw();
	void _Launch(_Thrd_t *_Thr);
	void _Release();
	virtual unsigned _Go() = 0;

private:
	_Cnd_t _Cond;
	_Mtx_t _Mtx;
	bool _Started;
	};

template<class _Target>
	class _LaunchPad
		: public _Pad
	{	// template class for launching threads
public:
	template<class _Other> inline
		_LaunchPad(_Other&& _Tgt)
		: _MyTarget(std::forward<_Other>(_Tgt))
		{	// construct from target
		}

	unsigned _Go()
		{	// run the thread function object
		return (_Run(this));
		}

private:
	static unsigned _Run(_LaunchPad *_Ln)
		{	// make local copy of function object and call it
		_Target _Local(std::forward<_Target>(_Ln->_MyTarget));
		_Ln->_Release();
		_Local();
		return (0);
		}

	typename _Target _MyTarget;
	};

template<class _Target> inline
	void _Launch(_Thrd_t *_Thr, _Target&& _Tg)
	{	// launch a new thread
	_LaunchPad<_Target> _Launcher(std::forward<_Target>(_Tg));
	_Launcher._Launch(_Thr);
	}

/*
**
** CLASS thread
**
*/
class thread
	{	// class for observing and managing threads
public:
	class id;
#ifdef _WIN32
	typedef void * native_handle_type;
#else
    typedef pthread_t native_handle_type;
#endif

	thread() throw()
		{	// construct with no thread
		_Thr_set_null(_Thr);
		}

    template<class _Fn>
    explicit thread(const _Fn& _Fx)
    {
        _Launch(&_Thr, _Fx);
    }
//#define _THREAD_CONS( \
//	TEMPLATE_LIST, PADDING_LIST, LIST, COMMA, X1, X2, X3, X4) \
//	template<class _Fn COMMA LIST(_CLASS_TYPE)> \
//		explicit thread(_Fn _Fx COMMA LIST(_TYPE_REFREF_ARG)) \
//		{	/* construct with _Fx(_Ax...) */ \
//		_Launch(&_Thr, \
//			 _STD bind(_Decay_copy(_STD forward<_Fn>(_Fx)) \
//				COMMA LIST(_FORWARD_ARG))); \
//		}
//
//_VARIADIC_EXPAND_0X(_THREAD_CONS, , , , )
//#undef _THREAD_CONS

	~thread() throw()
		{	// clean up
		if (joinable())
			_XSTD terminate();
		}

	thread(thread&& _Other) throw()
		: _Thr(_Other._Thr)
		{	// move from _Other
		_Thr_set_null(_Other._Thr);
		}

	thread& operator=(thread&& _Other) throw()
		{	// move from _Other
		return (_Move_thread(_Other));
		}

private:
	thread(const thread&);	// not defined
	thread& operator=(const thread&);	// not defined
public:
	void swap(thread& _Other) throw()
		{	// swap with _Other
		std::swap(_Thr, _Other._Thr);
		}

	bool joinable() const throw()
		{	// return true if this thread can be joined
		return (!_Thr_is_null(_Thr));
		}

	void join();

	void detach()
		{	// detach thread
		if (!joinable())
            throw std::invalid_argument("");
		_Thrd_detachX(_Thr);
		_Thr_set_null(_Thr);
		}

	id get_id() const throw();

	//static unsigned int hardware_concurrency() _NOEXCEPT
	//	{	// return number of hardware thread contexts
	//	return (::Concurrency::details::_GetConcurrency());
	//	}

	native_handle_type native_handle()
		{	// return Win32 HANDLE as void *
#ifdef _WIN32
		return (_Thr._Hnd);
#else
        return (_Thr._Id);
#endif
		}

private:
	thread& _Move_thread(thread& _Other)
		{	// move from _Other
		if (joinable())
			std::terminate();
		_Thr = _Other._Thr;
		_Thr_set_null(_Other._Thr);
		return (*this);
		}

	_Thrd_t _Thr;
	};

namespace this_thread {
thread::id get_id() throw();

inline void yield() throw()
	{	// give up balance of time slice
	//if (::Concurrency::details::_CurrentScheduler::_Id() != -1)
	//	{	// yield, then quit
	//	::Concurrency::details::_Context::_Yield();
	//	return;
	//	}
	//_Thrd_yield();
	}

//inline void sleep_until(const stdext::threads::xtime *_Abs_time)
//	{	// sleep until _Abs_time
//	if (::Concurrency::details::_CurrentScheduler::_Id() != -1)
//		{
//		stdext::threads::xtime _Now;
//		stdext::threads::xtime_get(&_Now, stdext::threads::TIME_UTC);
//		::Concurrency::wait(_Xtime_diff_to_millis2(_Abs_time, &_Now));
//		return;
//		}
//
//	_Thrd_sleep(_Abs_time);
//	}

//template<class _Clock,
//	class _Duration> inline
//	void sleep_until(
//		const chrono::time_point<_Clock, _Duration>& _Abs_time)
//	{	// sleep until time point
//	stdext::threads::xtime _Tgt;
//	_Tgt.sec = chrono::duration_cast<chrono::seconds>(
//		_Abs_time.time_since_epoch()).count();
//	_Tgt.nsec = (long)chrono::duration_cast<chrono::nanoseconds>(
//		_Abs_time.time_since_epoch() - chrono::seconds(_Tgt.sec)).count();
//	sleep_until(&_Tgt);
//	}

//template<class _Rep,
//	class _Period> inline
//	void sleep_for(const chrono::duration<_Rep, _Period>& _Rel_time)
//	{	// sleep for duration
//	stdext::threads::xtime _Tgt = _To_xtime(_Rel_time);
//	sleep_until(&_Tgt);
//	}
	}	// namespace this_thread

class thread::id
	{	// thread id
public:
	id() throw()
		{	// id for no thread
		_Thr_set_null(_Thr);
		}

	template<class _Ch,
		class _Tr>
		std::basic_ostream<_Ch, _Tr>& _To_text(
			std::basic_ostream<_Ch, _Tr>& _Str)
		{	// insert representation into stream
		return (_Str << _Thr_val(_Thr));
		}

	size_t hash() const
		{	// hash bits to size_t value by pseudorandomizing transform
		return (std::hash<size_t>()((size_t)_Thr_val(_Thr)));
		}

private:
	id(const thread& _Thrd)
		: _Thr(_Thrd._Thr)
		{	// construct from thread object
		}

	id(_Thrd_t _Thrd)
		: _Thr(_Thrd)
		{	// construct from thread identifier
		}

	_Thrd_t _Thr;

	friend thread::id thread::get_id() const throw();
	friend thread::id this_thread::get_id() throw();
	friend bool operator==(thread::id _Left, thread::id _Right) throw();
	friend bool operator<(thread::id _Left, thread::id _Right) throw();
	};

inline void thread::join()
	{	// join thread
	if (!joinable())
        throw std::invalid_argument("");
	if (_Thr_is_null(_Thr))
		 throw std::invalid_argument("");
	if (get_id() == this_thread::get_id())
        throw std::logic_error("");
	if (_Thrd_join(_Thr, 0) != _Thrd_success)
		 throw std::logic_error("");
	_Thr_set_null(_Thr);
	}

inline thread::id thread::get_id() const throw()
	{	// return id for this thread
	return (id(*this));
	}

inline thread::id this_thread::get_id() throw()
	{	// return id for current thread
	return (_Thrd_current());
	}

inline void swap(thread& _Left, thread& _Right) throw()
	{	// swap _Left with _Right
	_Left.swap(_Right);
	}

inline bool operator==(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left and _Right identify the same thread
	return (_Thrd_equal(_Left._Thr, _Right._Thr));
	}

inline bool operator!=(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left and _Right do not identify the same thread
	return (!(_Left == _Right));
	}

inline bool operator<(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left precedes _Right
	return (_Thrd_lt(_Left._Thr, _Right._Thr));
	}

inline bool operator<=(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left precedes or equals _Right
	return (!(_Right < _Left));
	}
inline bool operator>(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left follows _Right
	return (_Right < _Left);
	}
inline bool operator>=(thread::id _Left, thread::id _Right) throw()
	{	// return true if _Left follows or equals _Right
	return (!(_Left < _Right));
	}

template<class _Ch,
	class _Tr>
	std::basic_ostream<_Ch, _Tr>& operator<<(
		std::basic_ostream<_Ch, _Tr>& _Str,
		thread::id _Id)
	{	// insert id into stream
	return (_Id._To_text(_Str));
	}

	// TEMPLATE STRUCT SPECIALIZATION hash
//template<>
//	struct hash<thread::id>
//		: public unary_function<thread::id, size_t>
//	{	// hash functor for thread::id
//	typedef thread::id _Kty;
//
//	size_t operator()(const _Kty& _Keyval) const
//		{	// hash _Keyval to size_t value by pseudorandomizing transform
//		return (_Keyval.hash());
//		}
//	};


}; // namespace: thelib::asy
}; // namespace: thelib

#include "thread_synch.h"

#if defined(_WIN32) && _MSC_VER < 1700
namespace std {
	using namespace thelib::asy;
};
#define mutex thread_mutex
#define condition_variable thread_cond
#define unique_lock scoped_lock
#endif

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

