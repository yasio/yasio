/**** Notes:
*  When the object hold by thread will be destroyed , 
*  the thread must already exit or no longer dependent on any 
*  resources of the object.
*
*  Experience Summary 
*    Windows NT: The function TerminateThread can stop a specific thread unconditional.
*    Linux:
*     1.pthread_join(pthread_t id, nullptr): 
*       #a.  id == 0, will coredump SIGSEGV
*       #b.  double call in same thread, maybe coredump with SIGSEGV when previous call times exceed 4 times
*     2.pthread_cancel:
*       The function can't stop the thread when the thread body
*       has dead-loop and no blocking function call in the loop.
*/
#ifndef _THREAD_BASIC_H_
#define _THREAD_BASIC_H_
#include "thread_config.h"

namespace thelib { 

namespace asy {
/*
** xthreadcommon.h
**
*/
typedef struct
	{	/* thread identifier for Win32 */
#ifdef _WIN32
	void *_Hnd;	/* Win32 HANDLE */
#endif
	unsigned int _Id;
	} _Thrd_imp_t;
typedef _Thrd_imp_t _Thrd_t;
#ifdef _WIN32
typedef unsigned int (_stdcall *_Thrd_callback_t)(void *);
#else
typedef unsigned int (*_Thrd_callback_t)(void *);
#endif
//typedef struct _Mtx_internal_imp_t *_Mtx_imp_t;
//typedef struct _Cnd_internal_imp_t *_Cnd_imp_t;
typedef thread_mutex_t _Mtx_t;
typedef thread_cond_t _Cnd_t;
 
#define _Thr_val(thr) thr._Id
#define _Thr_set_null(thr) (thr._Id = 0)
#define _Thr_is_null(thr) (thr._Id == 0)

enum {	/* return codes */
	_Thrd_success,
	_Thrd_nomem,
	_Thrd_timedout,
	_Thrd_busy,
	_Thrd_error
	};

	/* threads */
typedef _Thrd_imp_t _Thrd_t;
// typedef int (*_Thrd_start_t)(void *);

#ifdef _WIN32
typedef DWORD thr_id;
typedef HANDLE thr_handle;
typedef DWORD (__stdcall*THREAD_PROC)(void*);
typedef DWORD  thread_return_t;
#else
typedef pthread_t thr_id;
typedef pthread_t thr_handle;
typedef void* (*THREAD_PROC)(void*);
typedef u_long thread_return_t;
#endif

typedef THREAD_PROC _Thrd_start_t;

#define _Cnd_initX thread_cond_init
#define _Mtx_initX thread_mutex_init
#define _Mtx_lockX thread_mutex_lock
#define _Mtx_unlockX thread_mutex_unlock
#define _Mtx_destroyX thread_mutex_destroy
#define _Cnd_waitX thread_cond_wait
#define _Cnd_broadcastX thread_cond_broadcast
#define _Cnd_signalX thread_cond_signal
#define _Cnd_destroyX thread_cond_destroy

#ifdef __linux
enum {
    thr_joinable = PTHREAD_CREATE_JOINABLE,
    thr_detached = PTHREAD_CREATE_DETACHED
};
#endif

//enum thread_state {
//    thread_state_unstart,
//    thread_state_starting,
//    thread_state_running,
//    thread_state_exiting,
//    thread_state_exited,
//    thread_state_shutting,
//};

class thread_basic
{
public:
    /*
    ** @brief  : Creates a thread to execute within the virtual address space of the calling process.
    ** @params : 
    **       1st: A pointer to the application-defined function to be executed by the thread. This pointer 
    **            represents the starting address of the thread. For more information on the thread function,
    **            see THREAD_PROC. 
    **       2nd: A pointer to a variable to be passed to the thread.
    **       3rd: A pointer to a variable that receives the thread identifier. If this parameter is NULL, 
    **            the thread identifier is not returned.
    **       4th: The flags that control the creation of the thread.
    **            Windows NT: ignore this parameter
    **            Linux     : thr_joinable or thr_detached
    ** @returns: 
    **       Windows NT: Please checkout reason using VC++ Menu [Tools]-->[Error Lookup]
    **       Linux     : EAGAIN(11)-->System limit new thread being created
    **                   EINVAL(22)-->The attr is illegal which is set by the 2nd parameter
    **
    */
    static int      spawn(THREAD_PROC, void* arg = nullptr,  size_t stacksize = 0, int flags = 0, _Thrd_t* pthr = nullptr);
    static int      join(_Thrd_t _Thr);
    // static int      join_n(thr_handle* thrs, size_t number);

public:
    thread_basic(void);
    virtual ~thread_basic(void);

	void            detach(void)
	{
		if(is_open())
		{
#ifdef _WIN32 
            CloseHandle(this->_Thr._Hnd);
#endif
			this->_Thr._Hnd = nullptr;
            this->_Thr._Id = 0;
		}
	};

    int             open(int flags = 0, size_t stacksize = 0);
    bool            is_open(void) const;

    void            close(void);

    thr_id          get_id(void) const;
    void*           get_handle(void) const;

    int             join(void);
    bool            is_joinable(void) const;

    
private:
    void            _Reset_handle(void);

    virtual 
    thread_return_t svc(void) = 0;

    static
    thread_return_t _ThreadEntry(thread_basic&);

private:
    _Thrd_t         _Thr;
    /*bool            is_dependent;
    thread_mutex    is_dependent_mutex;
    thread_cond     is_dependent_cond;*/
#ifdef __GNUC__
    bool            joinable;
#endif
};

}; // namespace: thelib::asy
}; // namespace: thelib

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/

