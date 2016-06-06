#include "thread.h"

namespace thelib { 

namespace asy {

int  _Thrd_create(_Thrd_t * thr, _Thrd_start_t func, void * b)
{
    return thread_basic::spawn(func, b, 0, 0, thr);
}

int  _Thrd_detach(_Thrd_t thr)
{
#ifdef _WIN32
    return (CloseHandle(thr._Hnd) == 0 ? _Thrd_error : _Thrd_success);
#else
    return pthread_detach(thr._Id) == 0 ? _Thrd_error : _Thrd_success);
#endif
}

void  _Thrd_exit(int code)
{
#ifdef _WIN32
    ExitThread(code);
#else
    pthread_exit((void*)code);
#endif
}

int  _Thrd_join(_Thrd_t _Thr, int *)
{
    return thread_basic::join(_Thr);
}

void  _Thrd_yield(void)
{
}

int  _Thrd_equal(_Thrd_t _Left, _Thrd_t _Right)
{
    return _Left._Id == _Right._Id;
}

int  _Thrd_lt(_Thrd_t _Left, _Thrd_t _Right)
{
    return _Left._Id < _Right._Id;
}

_Thrd_t  _Thrd_current(void)
{
    _Thrd_t _Thr;
    _Thr._Id = thr_self();
#ifdef _WIN32
    _Thr._Hnd = GetCurrentThread();
#endif
    return std::move(_Thr);
}

typedef unsigned int _Call_func_ret;
#ifdef _WIN32
#define _CALL_FUNC_MODIFIER __stdcall
#else
#define _CALL_FUNC_MODIFIER
#endif
extern "C" {
static _Call_func_ret _CALL_FUNC_MODIFIER _Call_func(void *_Data)
	{	// entry point for new thread
    _Call_func_ret _Res = 0;
 #if 1
	try {	// don't let exceptions escape
		_Res = (_Call_func_ret)static_cast<_Pad *>(_Data)->_Go();
		}
	catch (...)
		{	// uncaught exception in thread
		int zero = 0;
		if (zero == 0)
#if defined(_WIN32)
 #if 1300 <= _MSC_VER
			terminate();	// to quiet diagnostics
 #else /* 1300 <= _MSC_VER */
			_XSTD terminate();	// to quiet diagnostics
 #endif /* 1300 <= _MSC_VER */
#endif
		}

 #else /* _HAS_EXCEPTIONS */
	_Res = (_Call_func_ret)static_cast<_Pad *>(_Data)->_Go();
 #endif /* _HAS_EXCEPTIONS */

	//_Cnd_do_broadcast_at_thread_exit();
	return (_Res);
	}
}

_Pad::_Pad()
	{	// initialize handshake
	_Cnd_initX(&_Cond);
	//_Auto_cnd _Cnd_cleaner(&_Cond);
	_Mtx_initX(&_Mtx, _Mtx_plain);
	//_Auto_mtx _Mtx_cleaner(&_Mtx);
	_Started = false;
	_Mtx_lockX(&_Mtx);
	//_Mtx_cleaner._Release();
	//_Cnd_cleaner._Release();
	}

_Pad::~_Pad() throw()
	{	// clean up handshake
	/*_Auto_cnd _Cnd_cleaner(&_Cond);
	_Auto_mtx _Mtx_cleaner(&_Mtx);*/
	_Mtx_unlockX(&_Mtx);
    _Cnd_destroyX(&_Cond);
    _Mtx_destroyX(&_Mtx);
	}

void _Pad::_Launch(_Thrd_t *_Thr)
	{	// launch a thread
	_Thrd_startX(_Thr, (THREAD_PROC)_Call_func, this);
	while (!_Started)
		_Cnd_waitX(&_Cond, &_Mtx);
	}

void _Pad::_Release()
	{	// notify caller that it's okay to continue
	_Mtx_lockX(&_Mtx);
	_Started = true;
	_Cnd_signalX(&_Cond);
	_Mtx_unlockX(&_Mtx);
	}

}; /* namespace simplepp_1_13_201307::asy */
}; /* namespace simplepp_1_13_201307 */

