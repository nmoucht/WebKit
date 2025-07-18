/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#if ENABLE(GPU_PROCESS) && USE(AUDIO_SESSION)

#include <WebCore/AudioSession.h>
#include <WebCore/ProcessIdentifier.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/WeakHashSet.h>
#include <wtf/WeakRef.h>

namespace WebKit {

class GPUProcess;
class RemoteAudioSessionProxy;

class RemoteAudioSessionProxyManager
    : public RefCounted<RemoteAudioSessionProxyManager>
    , public WebCore::AudioSessionInterruptionObserver
    , private WebCore::AudioSessionConfigurationChangeObserver {
    WTF_MAKE_TZONE_ALLOCATED(RemoteAudioSessionProxyManager);
public:
    static Ref<RemoteAudioSessionProxyManager> create(GPUProcess& gpuProcess)
    {
        return adoptRef(*new RemoteAudioSessionProxyManager(gpuProcess));
    }

    ~RemoteAudioSessionProxyManager();

    void addProxy(RemoteAudioSessionProxy&, std::optional<audit_token_t>);
    void removeProxy(RemoteAudioSessionProxy&);

    void updateCategory();
    void updatePreferredBufferSizeForProcess();
    void updateSpatialExperience();

    bool tryToSetActiveForProcess(RemoteAudioSessionProxy&, bool);

    void beginInterruptionRemote();
    void endInterruptionRemote(WebCore::AudioSession::MayResume);

    WebCore::AudioSession& session() { return WebCore::AudioSession::singleton(); }
    const WebCore::AudioSession& session() const { return WebCore::AudioSession::singleton(); }
    Ref<WebCore::AudioSession> protectedSession() { return WebCore::AudioSession::singleton(); }
    Ref<const WebCore::AudioSession> protectedSession() const { return WebCore::AudioSession::singleton(); }

    void updatePresentingProcesses();

    USING_CAN_MAKE_WEAKPTR(WebCore::AudioSessionInterruptionObserver);

private:
    RemoteAudioSessionProxyManager(GPUProcess&);

    void beginAudioSessionInterruption() final;
    void endAudioSessionInterruption(WebCore::AudioSession::MayResume) final;

    void hardwareMutedStateDidChange(const WebCore::AudioSession&) final;
    void bufferSizeDidChange(const WebCore::AudioSession&) final;
    void sampleRateDidChange(const WebCore::AudioSession&) final;
    void configurationDidChange(const WebCore::AudioSession&);

    bool hasOtherActiveProxyThan(RemoteAudioSessionProxy& proxyToExclude);
    bool hasActiveNotInterruptedProxy();

    WeakRef<GPUProcess> m_gpuProcess;
    WeakHashSet<RemoteAudioSessionProxy> m_proxies;
};

}

#endif
