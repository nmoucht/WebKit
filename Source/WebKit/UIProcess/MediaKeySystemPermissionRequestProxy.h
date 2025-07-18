/*
 * Copyright (C) 2021 Igalia S.L.
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

#include "APIObject.h"
#include <WebCore/FrameIdentifier.h>
#include <WebCore/MediaKeySystemRequest.h>
#include <WebCore/MediaKeySystemRequestIdentifier.h>
#include <wtf/Vector.h>
#include <wtf/WeakPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class SecurityOrigin;
}

namespace WebKit {

class MediaKeySystemPermissionRequestManagerProxy;

class MediaKeySystemPermissionRequestProxy : public RefCounted<MediaKeySystemPermissionRequestProxy> {
public:
    static Ref<MediaKeySystemPermissionRequestProxy> create(MediaKeySystemPermissionRequestManagerProxy& manager, WebCore::MediaKeySystemRequestIdentifier mediaKeySystemID, WebCore::FrameIdentifier mainFrameID, WebCore::FrameIdentifier frameID, Ref<WebCore::SecurityOrigin>&& mediaOrigin, Ref<WebCore::SecurityOrigin>&& topLevelDocumentOrigin, const String& keySystem)
    {
        return adoptRef(*new MediaKeySystemPermissionRequestProxy(manager, mediaKeySystemID, mainFrameID, frameID, WTFMove(mediaOrigin), WTFMove(topLevelDocumentOrigin), keySystem));
    }

    void allow();
    void deny();

    void invalidate();

    WebCore::MediaKeySystemRequestIdentifier mediaKeySystemID() const { return m_mediaKeySystemID; }
    WebCore::FrameIdentifier mainFrameID() const { return m_mainFrameID; }
    WebCore::FrameIdentifier frameID() const { return m_frameID; }

    WebCore::SecurityOrigin& mediaKeyRequestSecurityOrigin() { return m_mediaKeyRequestSecurityOrigin.get(); }
    const WebCore::SecurityOrigin& mediaKeyRequestSecurityOrigin() const { return m_mediaKeyRequestSecurityOrigin.get(); }

    WebCore::SecurityOrigin& topLevelDocumentSecurityOrigin() { return m_topLevelDocumentSecurityOrigin.get(); }
    const WebCore::SecurityOrigin& topLevelDocumentSecurityOrigin() const { return m_topLevelDocumentSecurityOrigin.get(); }

    const String& keySystem() const { return m_keySystem; }

    const String& mediaKeysHashSalt() const { return m_mediaKeysHashSalt; }
    void setMediaKeysHashSalt(String&& salt) { m_mediaKeysHashSalt = WTFMove(salt); }

    void doDefaultAction();

private:
    MediaKeySystemPermissionRequestProxy(MediaKeySystemPermissionRequestManagerProxy&, WebCore::MediaKeySystemRequestIdentifier, WebCore::FrameIdentifier mainFrameID, WebCore::FrameIdentifier, Ref<WebCore::SecurityOrigin>&& userMediaSecurityOrigin, Ref<WebCore::SecurityOrigin>&& topLevelDocumentOrigin, const String& keySystem);

    WeakPtr<MediaKeySystemPermissionRequestManagerProxy> m_manager;
    WebCore::MediaKeySystemRequestIdentifier m_mediaKeySystemID;
    WebCore::FrameIdentifier m_mainFrameID;
    WebCore::FrameIdentifier m_frameID;
    const Ref<WebCore::SecurityOrigin> m_mediaKeyRequestSecurityOrigin;
    const Ref<WebCore::SecurityOrigin> m_topLevelDocumentSecurityOrigin;
    String m_keySystem;
    String m_mediaKeysHashSalt;
};

} // namespace WebKit
