/*
 * Copyright (C) 2010, 2015 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc. All rights reserved.
 * Copyright (C) 2017 Sony Interactive Entertainment Inc.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Forward.h>
#include <wtf/FunctionDispatcher.h>
#include <wtf/Seconds.h>
#include <wtf/Threading.h>

#if USE(COCOA_EVENT_LOOP)
#include <dispatch/dispatch.h>
#include <wtf/OSObjectPtr.h>
#else
#include <wtf/RunLoop.h>
#endif

namespace WTF {

class WorkQueueBase : protected ThreadLike {
public:
    using QOS = Thread::QOS;

    WTF_EXPORT_PRIVATE virtual ~WorkQueueBase();

    WTF_EXPORT_PRIVATE void dispatch(Function<void()>&&);
    WTF_EXPORT_PRIVATE void dispatchWithQOS(Function<void()>&&, QOS);
    WTF_EXPORT_PRIVATE virtual void dispatchAfter(Seconds, Function<void()>&&);
    WTF_EXPORT_PRIVATE virtual void dispatchSync(Function<void()>&&);

#if USE(COCOA_EVENT_LOOP)
    dispatch_queue_t dispatchQueue() const { return m_dispatchQueue.get(); }
#endif

    virtual void ref() const = 0;
    virtual void deref() const = 0;

protected:
    enum class Type : bool {
        Serial,
        Concurrent
    };
    WorkQueueBase(ASCIILiteral name, Type, QOS);
#if USE(COCOA_EVENT_LOOP)
    explicit WorkQueueBase(OSObjectPtr<dispatch_queue_t>&&);
#else
    explicit WorkQueueBase(RunLoop&);
#endif

#if USE(COCOA_EVENT_LOOP)
    const OSObjectPtr<dispatch_queue_t> m_dispatchQueue;
#else
    RunLoop* m_runLoop;
#endif
    uint32_t m_threadID { 0 };
private:
    void platformInitialize(ASCIILiteral name, Type, QOS);
    void platformInvalidate();
};

/**
 * A WorkQueue is a function dispatching interface like FunctionDispatcher.
 * Runnables dispatched to a WorkQueue are required to execute serially.
 * That is, two different runnables dispatched to the WorkQueue should never be allowed to execute simultaneously.
 * They may be executed on different threads but can safely be used by objects that aren't already threadsafe.
 * Use `assertIsCurrent(m_myQueue);` in a runnable to assert that the runnable runs in a specific queue.
 */
class WTF_CAPABILITY("is current") WTF_EXPORT_PRIVATE WorkQueue : public WorkQueueBase, public GuaranteedSerialFunctionDispatcher {
public:
    static WorkQueue& mainSingleton();
    static Ref<WorkQueue> create(ASCIILiteral name, QOS = QOS::Default);


    // WorkQueueBase
    void dispatch(Function<void()>&&) override;
    bool isCurrent() const override;
    void ref() const override { GuaranteedSerialFunctionDispatcher::ref(); }
    void deref() const override { GuaranteedSerialFunctionDispatcher::deref(); }

#if !USE(COCOA_EVENT_LOOP)
    RunLoop& runLoop() const { return *m_runLoop; }
#endif

protected:
    WorkQueue(ASCIILiteral name, QOS);
private:
    enum MainTag : bool {
        CreateMain
    };
    explicit WorkQueue(MainTag);
};

/**
 * A ConcurrentWorkQueue unlike a WorkQueue doesn't guarantee the order in which the dispatched runnable will run
 * and each can run concurrently on different threads.
 */
class WTF_EXPORT_PRIVATE ConcurrentWorkQueue final : public WorkQueueBase, public FunctionDispatcher, public ThreadSafeRefCounted<ConcurrentWorkQueue> {
public:
    static Ref<ConcurrentWorkQueue> create(ASCIILiteral name, QOS = QOS::Default);
    static void apply(size_t iterations, WTF::Function<void(size_t index)>&&);
    void dispatch(Function<void()>&&) override;

    void ref() const final;
    void deref() const final;

private:
    ConcurrentWorkQueue(ASCIILiteral, QOS);
};

inline void ConcurrentWorkQueue::ref() const
{
    ThreadSafeRefCounted<ConcurrentWorkQueue>::ref();
}

inline void ConcurrentWorkQueue::deref() const
{
    ThreadSafeRefCounted<ConcurrentWorkQueue>::deref();
}

}

using WTF::WorkQueue;
using WTF::ConcurrentWorkQueue;
