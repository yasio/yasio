#ifndef _THREAD_SYNCH_H_
#define _THREAD_SYNCH_H_
#include "simpleppdef.h"
#include "thread_config.h"
#include <limits>

namespace thelib {
namespace asy {

class thread_cond;
class thread_mutex;
class thread_rwlock;
class semaphore;

#ifdef _WIN32
class mutex;
class event;
class waitable_timer;
typedef enum {
    // The specified object is a mutex object that was not released by the thread that 
    // owned the mutex object before the owning thread terminated. Ownership of the mutex object 
    // is granted to the calling thread and the mutex state is set to nonsignaled.
    // If the mutex was protecting persistent state information, you should check it for consistency.
    w_abandoned = WAIT_ABANDONED,

    // The state of the specified object is signaled.
    w_object_0 = WAIT_OBJECT_0,

    // The time-out interval elapsed, and the object's state is nonsignaled.
    w_timeout = WAIT_TIMEOUT,

    // Wait failed.
    w_failed = WAIT_FAILED
} wait_result;

typedef enum {
    lock_mode_not_shared = 0,
    lock_mode_shared = CONDITION_VARIABLE_LOCKMODE_SHARED
} locke_mode;

#endif

template<typename lock_type>
class scoped_lock
{
public:
    scoped_lock(lock_type& locker_ref) : locker(locker_ref)
    {
        this->locker.lock();
    }
    ~scoped_lock(void)
    {
        this->locker.unlock();
    }

	lock_type& native_handle(void)
	{
		return locker;
	}

private:
    lock_type& locker;
};

template<typename lock_type>
class scoped_rlock
{
public:
    scoped_rlock(lock_type& locker_ref) : locker(locker_ref)
    {
        this->locker.lock_shared();
    }
    ~scoped_rlock(void)
    {
        this->locker.unlock_shared();
    }

private:
    lock_type& locker;
};

template<typename lock_type>
class scoped_wlock
{
public:
    scoped_wlock(lock_type& locker_ref) : locker(locker_ref)
    {
        this->locker.lock_exclusive();
    }
    ~scoped_wlock(void)
    {
        this->locker.unlock_exclusive();
    }

private:
    lock_type& locker;
};

class thread_mutex
{
    friend class thread_cond;
public:
    thread_mutex(void);
    ~thread_mutex(void);

    bool trylock(void);

    void lock(void);

    void unlock(void);

private:
    thread_mutex_t object;
};

class thread_rwlock
{
    friend class thread_cond;
public:
    thread_rwlock(void);
    ~thread_rwlock(void);

    void lock_shared(void);
    void unlock_shared(void);

    void lock_exclusive(void);
    void unlock_exclusive(void);

    bool trylock_shared(void);

    bool trylock_exclusive(void);

private:
    thread_rwlock_t object;
};

class thread_cond
{
public:
    thread_cond(void);
    ~thread_cond(void);

	void notify_one(void) {this->signal();};

    void signal(void);
    void broadcast(void);

    bool wait(thread_mutex& cs);
	bool wait(scoped_lock<thread_mutex>& lk) {return this->wait(lk.native_handle());};
    bool timedwait(thread_mutex& cs, long millseconds);

#ifdef _WIN32
    bool wait(thread_rwlock& rwl, locke_mode = lock_mode_shared);
    bool timedwait(thread_rwlock& rwl, long millseconds, locke_mode = lock_mode_shared);
#endif

private:
    thread_cond_t     object;
};

class thread_event
{
public:
    thread_event(void);
    ~thread_event(void);

    void wait(void);

    void notify(void);

private:

    thread_event_t event;
};

class thread_sem
{
public:
//#ifdef max
//#define __user_undef_max
//#undef max
//#endif
    thread_sem(int max_count = (std::numeric_limits<int>::max)());

    ~thread_sem(void);

    void post(void);

    void post(int n);

    void wait(void);

private:

    thread_sem_t sem;
};


}; // namespace: thelib::asy
}; // namespace: thelib

#define WAIT_WHILE_COND(cond,cond_expression,mtx) \
        do { \
            cond.wait(mtx); \
        } while(cond_expression)


#endif

/*
* Copyright (c) 2012-2013 by X.D. Guo  ALL RIGHTS RESERVED.
* Consult your license regarding permissions and restrictions.
**/


