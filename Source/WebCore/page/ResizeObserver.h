/*
 * Copyright (C) 2019 Igalia S.L.
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

#include "GCReachableRef.h"
#include "ResizeObservation.h"
#include "ResizeObserverCallback.h"
#include <wtf/Lock.h>
#include <wtf/RefCountedAndCanMakeWeakPtr.h>
#include <wtf/WeakPtr.h>

namespace JSC {

class AbstractSlotVisitor;

}

namespace WebCore {

class Document;
class Element;
struct ResizeObserverOptions;

struct ResizeObserverData {
    WTF_DEPRECATED_MAKE_STRUCT_FAST_ALLOCATED(ResizeObserverData);
    Vector<WeakPtr<ResizeObserver>> observers;
};

using NativeResizeObserverCallback = void (*)(const Vector<Ref<ResizeObserverEntry>>&, ResizeObserver&);
using JSOrNativeResizeObserverCallback = Variant<RefPtr<ResizeObserverCallback>, NativeResizeObserverCallback>;

class ResizeObserver : public RefCountedAndCanMakeWeakPtr<ResizeObserver> {
    WTF_MAKE_TZONE_OR_ISO_ALLOCATED(ResizeObserver);
public:
    static Ref<ResizeObserver> create(Document&, Ref<ResizeObserverCallback>&&);
    static Ref<ResizeObserver> createNativeObserver(Document&, NativeResizeObserverCallback&&);
    ~ResizeObserver();

    bool hasObservations() const { return m_observations.size(); }
    bool hasActiveObservations() const { return m_activeObservations.size(); }

    void observe(Element&);
    void observe(Element&, const ResizeObserverOptions&);
    void unobserve(Element&);
    void disconnect();
    void targetDestroyed(Element&);

    static size_t maxElementDepth() { return SIZE_MAX; }
    size_t gatherObservations(size_t depth);
    void deliverObservations();
    bool hasSkippedObservations() const { return m_hasSkippedObservations; }
    void setHasSkippedObservations(bool skipped) { m_hasSkippedObservations = skipped; }

    void resetObservationSize(Element&);

    ResizeObserverCallback* callbackConcurrently();
    bool isReachableFromOpaqueRoots(JSC::AbstractSlotVisitor&) const;

private:
    ResizeObserver(Document&, JSOrNativeResizeObserverCallback&&);

    bool removeTarget(Element&);
    void removeAllTargets();
    bool removeObservation(const Element&);
    void observeInternal(Element&, const ResizeObserverBoxOptions);
    bool isNativeCallback();
    bool isJSCallback();

    WeakPtr<Document, WeakPtrImplWithEventTargetData> m_document;
    JSOrNativeResizeObserverCallback m_JSOrNativeCallback;
    Vector<Ref<ResizeObservation>> m_observations;

    Vector<Ref<ResizeObservation>> m_activeObservations;
    Vector<GCReachableRef<Element>> m_activeObservationTargets;
    Vector<GCReachableRef<Element>> m_targetsWaitingForFirstObservation;

    bool m_hasSkippedObservations { false };
};

} // namespace WebCore
