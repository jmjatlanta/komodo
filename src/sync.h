// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/******************************************************************************
 * Copyright © 2014-2019 The SuperNET Developers.                             *
 *                                                                            *
 * See the AUTHORS, DEVELOPER-AGREEMENT and LICENSE files at                  *
 * the top-level directory of this distribution for the individual copyright  *
 * holder information and the developer policies on copyright and licensing.  *
 *                                                                            *
 * Unless otherwise agreed in a custom licensing agreement, no part of the    *
 * SuperNET software, including this file may be copied, modified, propagated *
 * or distributed except according to the terms contained in the LICENSE file *
 *                                                                            *
 * Removal or modification of this copyright notice is prohibited.            *
 *                                                                            *
 ******************************************************************************/

#ifndef BITCOIN_SYNC_H
#define BITCOIN_SYNC_H

#include "threadsafety.h"

#undef __cpuid
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>


////////////////////////////////////////////////
//                                            //
// THE SIMPLE DEFINITION, EXCLUDING DEBUG CODE //
//                                            //
////////////////////////////////////////////////

/*
CCriticalSection mutex;
    boost::recursive_mutex mutex;

LOCK(mutex);
    boost::unique_lock<boost::recursive_mutex> criticalblock(mutex);

LOCK2(mutex1, mutex2);
    boost::unique_lock<boost::recursive_mutex> criticalblock1(mutex1);
    boost::unique_lock<boost::recursive_mutex> criticalblock2(mutex2);

TRY_LOCK(mutex, name);
    boost::unique_lock<boost::recursive_mutex> name(mutex, boost::try_to_lock_t);

ENTER_CRITICAL_SECTION(mutex); // no RAII
    mutex.lock();

LEAVE_CRITICAL_SECTION(mutex); // no RAII
    mutex.unlock();
 */

///////////////////////////////
//                           //
// THE ACTUAL IMPLEMENTATION //
//                           //
///////////////////////////////

/**
 * Template mixin that adds -Wthread-safety locking
 * annotations to a subset of the mutex API.
 */
template <typename PARENT>
class CAPABILITY("mutex") AnnotatedMixin : public PARENT
{
public:
    void lock() ACQUIRE()
    {
        PARENT::lock();
    }

    void unlock() RELEASE()
    {
        PARENT::unlock();
    }

    bool try_lock() TRY_ACQUIRE(true)
    {
        return PARENT::try_lock();
    }

    const AnnotatedMixin<PARENT> operator!() const { return *this; }
};

/**
 * Wrapped boost mutex: supports recursive locking, but no waiting
 * TODO: We should move away from using the recursive lock by default.
 */
typedef AnnotatedMixin<boost::recursive_mutex> CCriticalSection;

/** Wrapped boost mutex: supports waiting but not recursive locking */
typedef AnnotatedMixin<boost::mutex> CWaitableCriticalSection;

/** Just a typedef for boost::condition_variable, can be wrapped later if desired */
typedef boost::condition_variable CConditionVariable;

#ifdef DEBUG_LOCKORDER
void EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false);
void LeaveCritical();
std::string LocksHeld();
void AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, void* cs);
#else
void static inline EnterCritical(const char* pszName, const char* pszFile, int nLine, void* cs, bool fTry = false) {}
void static inline LeaveCritical() {}
void static inline AssertLockHeldInternal(const char* pszName, const char* pszFile, int nLine, void* cs) {}
#endif
#define AssertLockHeld(cs) AssertLockHeldInternal(#cs, __FILE__, __LINE__, &cs)

#ifdef DEBUG_LOCKCONTENTION
void PrintLockContention(const char* pszName, const char* pszFile, int nLine);
#endif

/** Wrapper around boost::unique_lock<Mutex> */
template <typename Mutex>
class SCOPED_CAPABILITY CMutexLock
{
private:
    Mutex *mutex;
    boost::unique_lock<Mutex> lock;

    void Enter(const char* pszName, const char* pszFile, int nLine) ACQUIRE()
    {
        EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()));
#ifdef DEBUG_LOCKCONTENTION
        if (!lock.try_lock()) {
            PrintLockContention(pszName, pszFile, nLine);
#endif
            lock.lock();
#ifdef DEBUG_LOCKCONTENTION
        }
#endif
    }

    bool TryEnter(const char* pszName, const char* pszFile, int nLine)
    {
        EnterCritical(pszName, pszFile, nLine, (void*)(lock.mutex()), true);
        lock.try_lock();
        if (!lock.owns_lock())
            LeaveCritical();
        return lock.owns_lock();
    }

public:
    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine) 
            ACQUIRE(mutexIn) : mutex(&mutexIn), lock(mutexIn, boost::defer_lock)
    {
        Enter(pszName, pszFile, nLine);
    }

    CMutexLock(Mutex* pmutexIn, const char* pszName, const char* pszFile, int nLine) 
            ACQUIRE(pmutexIn) : mutex(pmutexIn)
    {
        if (!pmutexIn) return;

        lock = boost::unique_lock<Mutex>(*pmutexIn, boost::defer_lock);
        Enter(pszName, pszFile, nLine);
    }
    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine, boost::try_to_lock_t) 
            ACQUIRE(mutexIn) : mutex(&mutexIn), lock(mutexIn, boost::defer_lock)
    {
        TryEnter(pszName, pszFile, nLine);
    }
    CMutexLock(Mutex* pmutexIn, const char* pszName, const char* pszFile, int nLine, boost::try_to_lock_t) 
            ACQUIRE(pmutexIn) : mutex(pmutexIn)
    {
        if (!pmutexIn) 
            return;

        lock = boost::unique_lock<Mutex>(*pmutexIn, boost::defer_lock);
        TryEnter(pszName, pszFile, nLine);
    }
    /****
     * Build the scoping mechanism for a mutex for a mutex that is already locked
     * @param mutexIn The mutex
     * @param pszName the name of the mutex
     * @param pszFile calling point
     * @param nLine calling line
     */
    CMutexLock(Mutex& mutexIn, const char* pszName, const char* pszFile, int nLine, boost::adopt_lock_t) 
            REQUIRES_SHARED(mutexIn): mutex(&mutexIn), lock(mutexIn, boost::adopt_lock)
    {
    }

    bool TryLock() TRY_ACQUIRE(true, getMutex())
    {
        return lock.try_lock();
    }

    // move constructor/assignment
    CMutexLock(CMutexLock&& in) : mutex(std::move(in.mutex)), lock(std::move(in.lock)) {}
    CMutexLock& operator=(CMutexLock&& in)
    {
        mutex = in.mutex;
        lock = in.lock;
    }

    ~CMutexLock() RELEASE()
    {
        if (lock.owns_lock())
            LeaveCritical();
    }

    operator bool()
    {
        return lock.owns_lock();
    }
    Mutex *getMutex() RETURN_CAPABILITY(mutex); // declaration only, for static analysis
};

typedef CMutexLock<CCriticalSection> CCriticalBlock;

#define LOCK(cs) CCriticalBlock criticalblock(cs, #cs, __FILE__, __LINE__)
#define LOCK2(cs1, cs2) CCriticalBlock criticalblock1(cs1, #cs1, __FILE__, __LINE__), criticalblock2(cs2, #cs2, __FILE__, __LINE__)
#define TRY_LOCK(cs, name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, boost::try_to_lock)
#define ADOPT_LOCK(cs, name) CCriticalBlock name(cs, #cs, __FILE__, __LINE__, boost::adopt_lock)

#define ENTER_CRITICAL_SECTION(cs)                            \
    {                                                         \
        EnterCritical(#cs, __FILE__, __LINE__, (void*)(&cs)); \
        (cs).lock();                                          \
    }

#define LEAVE_CRITICAL_SECTION(cs) \
    {                              \
        (cs).unlock();             \
        LeaveCritical();           \
    }

class CSemaphore
{
private:
    boost::condition_variable condition;
    boost::mutex mutex;
    int value;

public:
    CSemaphore(int init) : value(init) {}

    void wait()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        while (value < 1) {
            condition.wait(lock);
        }
        value--;
    }

    bool try_wait()
    {
        boost::unique_lock<boost::mutex> lock(mutex);
        if (value < 1)
            return false;
        value--;
        return true;
    }

    void post()
    {
        {
            boost::unique_lock<boost::mutex> lock(mutex);
            value++;
        }
        condition.notify_one();
    }
};

/** RAII-style semaphore lock */
class CSemaphoreGrant
{
private:
    CSemaphore* sem;
    bool fHaveGrant;

public:
    void Acquire()
    {
        if (fHaveGrant)
            return;
        sem->wait();
        fHaveGrant = true;
    }

    void Release()
    {
        if (!fHaveGrant)
            return;
        sem->post();
        fHaveGrant = false;
    }

    bool TryAcquire()
    {
        if (!fHaveGrant && sem->try_wait())
            fHaveGrant = true;
        return fHaveGrant;
    }

    void MoveTo(CSemaphoreGrant& grant)
    {
        grant.Release();
        grant.sem = sem;
        grant.fHaveGrant = fHaveGrant;
        sem = NULL;
        fHaveGrant = false;
    }

    CSemaphoreGrant() : sem(NULL), fHaveGrant(false) {}

    CSemaphoreGrant(CSemaphore& sema, bool fTry = false) : sem(&sema), fHaveGrant(false)
    {
        if (fTry)
            TryAcquire();
        else
            Acquire();
    }

    ~CSemaphoreGrant()
    {
        Release();
    }

    operator bool()
    {
        return fHaveGrant;
    }
};

#endif // BITCOIN_SYNC_H
