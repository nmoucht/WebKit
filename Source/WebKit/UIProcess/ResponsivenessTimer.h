/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef ResponsivenessTimer_h
#define ResponsivenessTimer_h

#include <wtf/AbstractRefCountedAndCanMakeWeakPtr.h>
#include <wtf/FastMalloc.h>
#include <wtf/RefCounted.h>
#include <wtf/RunLoop.h>
#include <wtf/WeakRef.h>

namespace WebKit {

class ResponsivenessTimer : public RefCounted<ResponsivenessTimer> {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(ResponsivenessTimer);
public:
    class Client : public AbstractRefCountedAndCanMakeWeakPtr<Client> {
    public:
        virtual ~Client() { }
        virtual void didBecomeUnresponsive() = 0;
        virtual void didBecomeResponsive() = 0;

        virtual void willChangeIsResponsive() = 0;
        virtual void didChangeIsResponsive() = 0;

        virtual bool mayBecomeUnresponsive() = 0;
    };

    static constexpr Seconds defaultResponsivenessTimeout = 3_s;
    static Ref<ResponsivenessTimer> create(ResponsivenessTimer::Client&, Seconds responsivenessTimeout);
    ~ResponsivenessTimer();

    void start();

    // A responsiveness timer with lazy stop does not stop the underlying system timer when stopped.
    // Instead, it ignores the timeout if stop() was already called.
    //
    // This exists to reduce the rate at which we reset the timer.
    //
    // With a non lazy timer, we may set a timer and reset it soon after because the process is responsive.
    // For events, this means reseting a timer 120 times/s for a 60 Hz event source.
    // By not reseting the timer when responsive, we cut that in half to 60 timeout changes.
    void startWithLazyStop();

    void stop();

    void invalidate();

    // Return true if stop() was not called betfore the responsiveness timeout.
    bool isResponsive() const { return m_isResponsive; }

    // Return true if there is an active timer. The state could be responsive or not.
    bool hasActiveTimer() const { return m_waitingForTimer; }

    void processTerminated();

private:
    ResponsivenessTimer(ResponsivenessTimer::Client&, Seconds responsivenessTimeout);

    void timerFired();

    bool mayBecomeUnresponsive() const;

    WeakPtr<ResponsivenessTimer::Client> m_client;

    RunLoop::Timer m_timer;
    MonotonicTime m_restartFireTime;

    bool m_isResponsive { true };
    bool m_waitingForTimer { false };
    bool m_useLazyStop { false };
    Seconds m_responsivenessTimeout;
};

} // namespace WebKit

#endif // ResponsivenessTimer_h

