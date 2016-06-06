#include <errno.h>
#include <signal.h>
#include <process.h>
#include "thread_basic.h"
#ifndef _WIN32
#include <string.h>
#endif
#include <iostream>

using namespace thelib::asy;

int thread_basic::spawn(THREAD_PROC base, void* arg, size_t stacksize, int flags, _Thrd_t* pthr)
{
    _Thrd_t thr;
    int retc = 0;
#ifdef _WIN32
    #ifdef _WIN32_WCE
    SetLastError(0);
    thr._Hnd = ::CreateThread(nullptr, stacksize, base, arg, flags, (LPDWORD)&thr._Id);
    #else
    thr._Hnd = (HANDLE)::_beginthreadex(nullptr, stacksize, (unsigned int(__stdcall*)(void*))base, arg, flags, &thr._Id);
    retc = GetLastError();
    #endif
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, flags);
    pthread_attr_setstacksize(&attr, stacksize);
    retc = pthread_create(&thr._Id, &attr, base, arg);
    pthread_attr_destroy(&attr);
#endif    
//#if 1
//    thr._Hnd = ::_beginthreadex(nullptr, stacksize, 
//#endif
    if(pthr != nullptr) {
        *pthr = thr;
    }
    return retc; 
}

int thread_basic::join(_Thrd_t thr)
{
#ifdef _WIN32
    return WaitForSingleObject(thr._Hnd, INFINITE);
#else
    return pthread_join(thr._Id, NULL);
#endif
}

//int thread_basic::join_n(thr_handle* thrs, size_t number)
//{
//    int retc = 0;
//#ifdef _WIN32
//    retc = WaitForMultipleObjects(number, thrs, TRUE, INFINITE);
//#else
//    for(size_t i = 0; i < number; ++i) 
//    {
//        retc += thread_basic::join(thrs[i]);
//    }
//#endif
//    return retc;
//}

thread_basic::thread_basic(void)
{
#ifdef __GNUC__
    this->joinable = false;
#endif
}

thread_basic::~thread_basic(void)
{
    close();
}

int thread_basic::open(int flags, size_t stacksize)
{
    if(!this->is_open()) {
        int stat = thread_basic::spawn((THREAD_PROC)thread_basic::_ThreadEntry, this, stacksize, flags, &this->_Thr);
#ifdef __GNUC__
        this->joinable = (0 == stat && (thr_joinable == flags || 0 == flags));
#endif
        return stat;
    }
    return 0;
}

bool thread_basic::is_open(void) const 
{
    return !_Thr_is_null(_Thr);
}

void thread_basic::close(void)
{
    if(this->is_open())
    {
        thr_kill(this->_Thr);

        this->join();

        this->_Reset_handle();
    }
}

int thread_basic::join(void)
{
    int ret = 0;
    if( this->is_joinable() )
    {
#ifdef __GNUC__
        this->joinable = false;
#endif
        ret = thread_basic::join(this->_Thr);

#ifdef _WIN32
        ::CloseHandle(this->_Thr._Hnd);
#endif

        this->_Reset_handle();
        return ret;
    }
    return 0;
}

bool thread_basic::is_joinable(void) const
{
#ifdef _WIN32
    return this->is_open();
#else
    return this->joinable;
#endif
}

thr_id thread_basic::get_id(void) const
{
    return thr_getid(this->_Thr);
}

void* thread_basic::get_handle(void) const
{
    return this->_Thr._Hnd;
}

void thread_basic::_Reset_handle(void)
{
    this->_Thr._Hnd = 0;
}

thread_return_t thread_basic::_ThreadEntry(thread_basic& object)
{
    return object.svc();
}

