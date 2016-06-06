#ifndef _THREAD_CONFIG_H_
#define _THREAD_CONFIG_H_


#ifdef _WIN32
#include <Windows.h>
//#define sleep(sec) Sleep(sec * 1000) confilict with boost

// thread normal operations
#define thr_kill(thr)   TerminateThread(thr._Hnd, SIGTERM)
#define thr_getid(thr)  GetThreadId(thr._Hnd)
#define thr_self()      GetCurrentThreadId()

// thread mutex operations
#define thread_mutex_t                            CRITICAL_SECTION
#define thread_mutex_init(mtx,attr)               InitializeCriticalSection(mtx)
#define thread_mutex_destroy(mtx)                 DeleteCriticalSection(mtx)
#define thread_mutex_lock(mtx)                    EnterCriticalSection(mtx)
#define thread_mutex_trylock(mtx)                 TryEnterCriticalSection(mtx)
#define thread_mutex_unlock(mtx)                  LeaveCriticalSection(mtx)

// thread rwlock operations
#define thread_rwlock_t                           SRWLOCK
#define thread_rwlock_init(rwlock,attr)           InitializeSRWLock(rwlock)
#define thread_rwlock_destroy(rwlock)
#define thread_rwlock_lock_shared(rwlock)         AcquireSRWLockShared(rwlock)
#define thread_rwlock_trylock_shared(rwlock)      TryAcquireSRWLockShared(rwlock)
#define thread_rwlock_lock_exclusive(rwlock)      AcquireSRWLockExclusive(rwlock)
#define thread_rwlock_trylock_exclusive(rwlock)   TryAcquireSRWLockExclusive(rwlock)
#define thread_rwlock_unlock_shared(rwlock)       ReleaseSRWLockShared(rwlock)
#define thread_rwlock_unlock_exclusive(rwlock)    ReleaseSRWLockExclusive(rwlock)

// thread cond operations
#define thread_cond_t                             CONDITION_VARIABLE
#define thread_cond_init(mtx,attr)                InitializeConditionVariable(mtx)
#define thread_cond_destroy(mtx)
#define thread_cond_wait(cond,mtx)                SleepConditionVariableCS(cond, mtx, INFINITE)
#define thread_cond_signal(cond)                  WakeConditionVariable(cond)
#define thread_cond_broadcast(cond)               WakeAllConditionVariable(cond)

// thread event operations
#define thread_event_t                            HANDLE
#define thread_event_init(event)                  ( event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr) )
#define thread_event_destroy(event)               ::CloseHandle(event)
#define thread_event_wait(event)                  ::WaitForSingleObject(event, INFINITE)
#define thread_event_notify(event)                ::SetEvent(event)

#define thread_sem_t                              HANDLE
#define thread_sem_init(sem,max_count)            sem = CreateSemaphore(nullptr, 0, max_count, nullptr)
#define thread_sem_destroy(sem)                   ::CloseHandle(sem);
#define thread_sem_wait(sem)                      ::WaitForSingleObject(sem,INFINITE)
#define thread_sem_post(sem)                      ::ReleaseSemaphore(sem, 1, nullptr)

#else
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <semaphore.h>
#define getlwpid()      syscall(SYS_gettid)
#define thr_kill(thr)   pthread_cancel(thr._Id)
#define thr_getid(thr)  thr._Id
#define thr_self()      pthread_self()

#define thread_mutex_t                            pthread_mutex_t
#define thread_mutex_init(mtx,attr)               pthread_mutex_init(mtx,attr)
#define thread_mutex_destroy(mtx)                 pthread_mutex_destroy(mtx)
#define thread_mutex_lock(mtx)                    pthread_mutex_lock(mtx)
#define thread_mutex_trylock(mtx)                 pthread_mutex_trylock(mtx)
#define thread_mutex_unlock(mtx)                  pthread_mutex_unlock(mtx)

#define thread_rwlock_t                           pthread_rwlock_t
#define thread_rwlock_init(rwlock,attr)           pthread_rwlock_init(rwlock,attr)
#define thread_rwlock_destroy(rwlock)             pthread_rwlock_destroy(rwlock)
#define thread_rwlock_lock_shared(rwlock)         pthread_rwlock_rdlock(rwlock)
#define thread_rwlock_trylock_shared(rwlock)      pthread_rwlock_tryrdlock(rwlock)
#define thread_rwlock_lock_exclusive(rwlock)      pthread_rwlock_wrlock(rwlock)
#define thread_rwlock_trylock_exclusive(rwlock)   pthread_rwlock_trywrlock(rwlock)
#define thread_rwlock_unlock_shared(rwlock)       pthread_rwlock_unlock(rwlock)
#define thread_rwlock_unlock_exclusive(rwlock)    pthread_rwlock_unlock(rwlock)

#define thread_cond_t                             pthread_cond_t
#define thread_cond_init(mtx,attr)                pthread_cond_init(mtx, attr)
#define thread_cond_destroy(mtx)                  pthread_cond_destroy(mtx)
#define thread_cond_wait(cond,mtx)                pthread_cond_wait(cond, mtx)
#define thread_cond_signal(cond)                  pthread_cond_signal(cond)
#define thread_cond_broadcast(cond)               pthread_cond_broadcast(cond)

#define thread_event_t                            sem_t 
#define thread_event_init(event)                  sem_init(&event, 0, 0)
#define thread_event_destroy(event)               sem_destroy(&event)
#define thread_event_wait(event)                  sem_wait(&event)
#define thread_event_notify(event)                sem_post(&event)

#define thread_sem_t                              sem_t 
#define thread_sem_init(sem,max_count)            sem_init(&sem, 0, 0)
#define thread_sem_destroy(sem)                   sem_destroy(&sem)
#define thread_sem_wait(sem)                      sem_wait(&sem)
#define thread_sem_post(sem)                      sem_post(&sem)

#endif
#include "simpleppdef.h"

#endif
/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
V3.0:2011 */

