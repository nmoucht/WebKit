/*
 * Copyright (C) 2013-2025 Apple Inc. All rights reserved.
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

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)

#include "LegacyCDM.h"
#include <JavaScriptCore/Forward.h>
#include <wtf/CheckedRef.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>

namespace WebCore {

class Document;
class WeakPtrImplWithEventTargetData;
class HTMLMediaElement;
class WebKitMediaKeySession;
template<typename> class ExceptionOr;

class WebKitMediaKeys final : public RefCounted<WebKitMediaKeys>, private LegacyCDMClient {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(WebKitMediaKeys);
    WTF_OVERRIDE_DELETE_FOR_CHECKED_PTR(WebKitMediaKeys);
public:
    static ExceptionOr<Ref<WebKitMediaKeys>> create(const String& keySystem);
    virtual ~WebKitMediaKeys();

    void decrementCheckedPtrCount() const { CanMakeCheckedPtr::decrementCheckedPtrCount(); }
    void incrementCheckedPtrCount() const { CanMakeCheckedPtr::incrementCheckedPtrCount(); }

    ExceptionOr<Ref<WebKitMediaKeySession>> createSession(Document&, const String& mimeType, Ref<Uint8Array>&& initData);
    static bool isTypeSupported(const String& keySystem, const String& mimeType);
    const String& keySystem() const { return m_keySystem; }

    LegacyCDM& cdm() { return m_cdm; }

    void setMediaElement(HTMLMediaElement*);

    void keyAdded();
    RefPtr<ArrayBuffer> cachedKeyForKeyId(const String& keyId) const;

private:
    RefPtr<MediaPlayer> cdmMediaPlayer(const LegacyCDM*) const final;

    WebKitMediaKeys(const String& keySystem, Ref<LegacyCDM>&&);

    Vector<Ref<WebKitMediaKeySession>> m_sessions;
    WeakPtr<HTMLMediaElement> m_mediaElement;
    String m_keySystem;
    const Ref<LegacyCDM> m_cdm;
};

}

#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
