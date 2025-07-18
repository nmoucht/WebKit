/*
 * Copyright (C) 2015-2025 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#pragma once

#include <wtf/Box.h>
#include <wtf/Condition.h>
#include <wtf/Lock.h>
#include <wtf/RefPtr.h>
#include <wtf/SharedTask.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/WeakRandom.h>
#include <wtf/text/CString.h>

namespace WTF {

class AutomaticThread;
class AutomaticThreadCondition;

// A ParallelHelperPool is a shared pool of threads that can be asked to help with some finite-time
// parallel activity. It's designed to work well when there are multiple concurrent tasks that may
// all want parallel help. In that case, we don't want each task to start its own thread pool. It's
// also designed to work well for tasks that do their own load balancing and do not wish to
// participate in microtask-style load balancing.
//
// A pool can have many clients, and each client may have zero or one tasks. The pool will have up
// to some number of threads, configurable with ParallelHelperPool::addThreads(); usually you bound
// this by the number of CPUs. Whenever a thread is idle and it notices that some client has a
// task, it will run the task. A task may be run on anywhere between zero and N threads, where N is
// the number of threads in the pool. Tasks run to completion. It's expected that a task will have
// its own custom ideas about how to participate in some parallel activity's load balancing, and it
// will return when the parallel activity is done. For example, a parallel marking task will return
// when the mark phase is done.
//
// Threads may have a choice between many tasks, since there may be many clients and each client
// may have a task. For the marking example, that may happen if there are multiple VM instances and
// each instance decides to start parallel marking at the same time. In that case, threads choose
// a task at random. So long as any client has a task, all threads in the pool will continue
// running the available tasks. Threads go idle when no client has tasks to run.

class ParallelHelperClient;

class ParallelHelperPool : public ThreadSafeRefCounted<ParallelHelperPool> {
public:
    WTF_EXPORT_PRIVATE ParallelHelperPool(ASCIILiteral threadName);
    WTF_EXPORT_PRIVATE ~ParallelHelperPool();

    WTF_EXPORT_PRIVATE void ensureThreads(unsigned numThreads);

    unsigned numberOfThreads() const { return m_numThreads; }
    
    WTF_EXPORT_PRIVATE void doSomeHelping();

private:
    friend class ParallelHelperClient;
    class Thread;
    friend class Thread;

    void didMakeWorkAvailable(const AbstractLocker&) WTF_REQUIRES_LOCK(m_lock);

    bool hasClientWithTask() WTF_REQUIRES_LOCK(m_lock);
    ParallelHelperClient* getClientWithTask() WTF_REQUIRES_LOCK(m_lock);
    
    Box<Lock> m_lock; // AutomaticThread wants this in a box for safety.
    const Ref<AutomaticThreadCondition> m_workAvailableCondition;
    Condition m_workCompleteCondition;

    WeakRandom m_random;
    
    Vector<ParallelHelperClient*> m_clients WTF_GUARDED_BY_LOCK(*m_lock);
    Vector<RefPtr<AutomaticThread>> m_threads;
    ASCIILiteral m_threadName;
    unsigned m_numThreads { 0 }; // This can be larger than m_threads.size() because we start threads only once there is work.
    bool m_isDying { false };
};

// A client is a placeholder for a parallel algorithm. A parallel algorithm will have a task that
// can be run concurrently. Whenever a client has a task set (you have called setTask() or
// setFunction()), threads in the pool may run that task. If a task returns on any thread, the
// client will assume that the task is done and will clear the task. If the task is cleared (the
// task runs to completion on any thread or you call finish()), any threads in the pool already
// running the last set task(s) will continue to run them. You can wait for all of them to finish
// by calling finish(). That method will clear the task and wait for any threads running the last
// set task to finish. There are two known-good patterns for using a client:
//
// 1) Tasks intrinsically know when the algorithm reaches termination, and simply returns when
//    this happens. The main thread runs the task by doing:
//
//    client->setFunction(
//        [=] () {
//            do things;
//        });
//    client->doSomeHelping();
//    client->finish();
//
//    Calling doSomeHelping() ensures that the algorithm runs on at least one thread (this one).
//    Tasks will know when to complete, and will return when they are done. This will clear the
//    task to ensure that no new threads will run the task. Then, finish() clears the current task
//    and waits for any parallel tasks to finish after the main thread has finished. It's possible
//    for threads to still be running the last set task (i.e. the one set by setFunction()) even
//    after the task has been cleared. Waiting for idle ensures that no old tasks are running
//    anymore.
//
//    You can do this more easily by using the runFunctionInParallel() helper:
//
//    clients->runFunctionInParallel(
//        [=] () {
//            do things;
//        });
//
// 2) Tasks keep doing things until they are told to quit using some custom notification mechanism.
//    The main thread runs the task by doing:
//
//    bool keepGoing = true;
//    client->setFunction(
//        [=] () {
//            while (keepGoing) {
//                do things;
//            }
//        });
//
//    When work runs out, the main thread will inform tasks that there is no more work, and then
//    wait until no more tasks are running:
//
//    keepGoing = false;
//    client->finish();
//
//    This works best when the main thread doesn't actually want to run the task that it set in the
//    client. This happens for example in parallel marking. The main thread uses a somewhat
//    different marking algorithm than the helpers. The main thread may provide work that the
//    helpers steal. The main thread knows when termination is reached, and simply tells the
//    helpers to stop upon termination.
//
// The known-good styles of using ParallelHelperClient all involve a parallel algorithm that has
// its own work distribution and load balancing.
//
// Note that it is not valid to use the same ParallelHelperClient instance from multiple threads.
// Each thread should have its own ParallelHelperClient in that case. Failure to follow this advice
// will lead to RELEASE_ASSERT's or worse.
class ParallelHelperClient {
    WTF_MAKE_NONCOPYABLE(ParallelHelperClient);
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(ParallelHelperClient);
public:
    WTF_EXPORT_PRIVATE ParallelHelperClient(RefPtr<ParallelHelperPool>&&);
    WTF_EXPORT_PRIVATE ~ParallelHelperClient();

    WTF_EXPORT_PRIVATE void setTask(RefPtr<SharedTask<void ()>>&&);

    template<typename Functor>
    void setFunction(const Functor& functor)
    {
        setTask(createSharedTask<void ()>(functor));
    }

    WTF_EXPORT_PRIVATE void finish();

    WTF_EXPORT_PRIVATE void doSomeHelping();

    // Equivalent to:
    // client->setTask(task);
    // client->doSomeHelping();
    // client->finish();
    WTF_EXPORT_PRIVATE void runTaskInParallel(RefPtr<SharedTask<void ()>>&&);

    // Equivalent to:
    // client->setFunction(functor);
    // client->doSomeHelping();
    // client->finish();
    template<typename Functor>
    void runFunctionInParallel(const Functor& functor)
    {
        runTaskInParallel(createSharedTask<void ()>(functor));
    }

    ParallelHelperPool& pool() { return *m_pool; }
    unsigned numberOfActiveThreads() const { return m_numActive; }

private:
    friend class ParallelHelperPool;

    void finishWithLock() WTF_REQUIRES_LOCK(*m_pool->m_lock);
    RefPtr<SharedTask<void ()>> claimTask() WTF_REQUIRES_LOCK(*m_pool->m_lock);
    void runTask(const RefPtr<SharedTask<void ()>>&);

    RefPtr<ParallelHelperPool> m_pool;
    RefPtr<SharedTask<void ()>> m_task;
    unsigned m_numActive { 0 };
};

} // namespace WTF

using WTF::ParallelHelperClient;
using WTF::ParallelHelperPool;
