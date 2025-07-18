/*
 * Copyright (C) 2023 Igalia S.L. All rights reserved.
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

#include "AbortController.h"
#include "AbortSignal.h"
#include "DOMFormData.h"
#include "Element.h"
#include "Event.h"
#include "EventInit.h"
#include "JSValueInWrappedObject.h"
#include "LocalDOMWindowProperty.h"
#include "NavigationDestination.h"
#include "NavigationInterceptHandler.h"
#include "NavigationNavigationType.h"

namespace WebCore {

enum class InterceptionState : uint8_t {
    Intercepted,
    Committed,
    Scrolled,
    Finished,
};

enum class InterceptionHandlersDidFulfill : bool {
    No,
    Yes
};

enum class FocusDidChange : bool {
    No,
    Yes
};

class NavigateEvent final : public Event {
    WTF_MAKE_TZONE_OR_ISO_ALLOCATED(NavigateEvent);
public:
    struct Init : EventInit {
        NavigationNavigationType navigationType { NavigationNavigationType::Push };
        RefPtr<NavigationDestination> destination;
        RefPtr<AbortSignal> signal;
        RefPtr<DOMFormData> formData;
        String downloadRequest;
        JSC::JSValue info;
        RefPtr<Element> sourceElement;
        bool canIntercept { false };
        bool userInitiated { false };
        bool hashChange { false };
        bool hasUAVisualTransition { false };
    };

    enum class NavigationFocusReset : bool {
        AfterTransition,
        Manual,
    };

    enum class NavigationScrollBehavior : bool {
        AfterTransition,
        Manual,
    };

    struct NavigationInterceptOptions {
        RefPtr<NavigationInterceptHandler> handler;
        std::optional<NavigationFocusReset> focusReset;
        std::optional<NavigationScrollBehavior> scroll;
    };

    static Ref<NavigateEvent> create(const AtomString& type, const Init&);
    static Ref<NavigateEvent> create(const AtomString& type, const Init&, AbortController*);

    NavigationNavigationType navigationType() const { return m_navigationType; }
    bool canIntercept() const { return m_canIntercept; }
    bool userInitiated() const { return m_userInitiated; }
    bool hashChange() const { return m_hashChange; }
    bool hasUAVisualTransition() const { return m_hasUAVisualTransition; }
    NavigationDestination* destination() { return m_destination.get(); }
    AbortSignal* signal() { return m_signal.get(); }
    DOMFormData* formData() { return m_formData.get(); }
    String downloadRequest() { return m_downloadRequest; }
    JSC::JSValue info() { return m_info.getValue(); }
    JSValueInWrappedObject& infoWrapper() { return m_info; }
    Element* sourceElement() { return m_sourceElement.get(); }

    ExceptionOr<void> intercept(Document&, NavigationInterceptOptions&&);
    ExceptionOr<void> scroll(Document&);

    bool wasIntercepted() const { return m_interceptionState.has_value(); }
    void setCanIntercept(bool canIntercept) { m_canIntercept = canIntercept; }
    void setInterceptionState(InterceptionState interceptionState) { m_interceptionState = interceptionState; }

    void finish(Document&, InterceptionHandlersDidFulfill, FocusDidChange);

    Vector<Ref<NavigationInterceptHandler>>& handlers() { return m_handlers; }

private:
    NavigateEvent(const AtomString& type, const Init&, EventIsTrusted, AbortController*);

    ExceptionOr<void> sharedChecks(Document&);
    void potentiallyProcessScrollBehavior(Document&);
    void processScrollBehavior(Document&);

    NavigationNavigationType m_navigationType;
    RefPtr<NavigationDestination> m_destination;
    RefPtr<AbortSignal> m_signal;
    RefPtr<DOMFormData> m_formData;
    String m_downloadRequest;
    Vector<Ref<NavigationInterceptHandler>> m_handlers;
    JSValueInWrappedObject m_info;
    RefPtr<Element> m_sourceElement;
    bool m_canIntercept { false };
    bool m_userInitiated { false };
    bool m_hashChange { false };
    bool m_hasUAVisualTransition { false };
    std::optional<InterceptionState> m_interceptionState;
    std::optional<NavigationFocusReset> m_focusReset;
    std::optional<NavigationScrollBehavior> m_scrollBehavior;
    RefPtr<AbortController> m_abortController;
};

} // namespace WebCore
