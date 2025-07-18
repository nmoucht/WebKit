/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Location.h"

#include "DocumentInlines.h"
#include "FrameLoader.h"
#include "LocalDOMWindow.h"
#include "LocalDOMWindowProperty.h"
#include "LocalFrame.h"
#include "NavigationScheduler.h"
#include "Quirks.h"
#include "SecurityOrigin.h"
#include "ServiceWorkerContainer.h"
#include <wtf/TZoneMallocInlines.h>
#include <wtf/URL.h>
#include <wtf/text/MakeString.h>
#include <wtf/text/StringToIntegerConversion.h>

namespace WebCore {

WTF_MAKE_TZONE_OR_ISO_ALLOCATED_IMPL(Location);

Location::Location(DOMWindow& window)
    : m_window(window)
{
}

Frame* Location::frame()
{
    if (!m_window)
        return nullptr;
    return m_window->frame();
}

const Frame* Location::frame() const
{
    if (!m_window)
        return nullptr;
    return m_window->frame();
}

const URL& Location::url() const
{
    RefPtr localWindow = dynamicDowncast<LocalDOMWindow>(*m_window);
    if (!localWindow) {
        static NeverDestroyed<URL> nullURL;
        return nullURL.get();
    }

    const URL& url = localWindow->document()->urlForBindings();
    if (!url.isValid())
        return aboutBlankURL(); // Use "about:blank" while the page is still loading (before we have a frame).

    return url;
}

String Location::href() const
{
    URL urlWithoutCredentials(url());
    urlWithoutCredentials.removeCredentials();
    return urlWithoutCredentials.string();
}

String Location::protocol() const
{
    return makeString(url().protocol(), ':');
}

String Location::host() const
{
    // Note: this is the IE spec. The NS spec swaps the two, it says
    // "The hostname property is the concatenation of the host and port properties, separated by a colon."
    return url().hostAndPort();
}

String Location::hostname() const
{
    return url().host().toString();
}

String Location::port() const
{
    auto port = url().port();
    return port ? String::number(*port) : emptyString();
}

String Location::pathname() const
{
    auto path = url().path();
    return path.isEmpty() ? "/"_s : path.toString();
}

String Location::search() const
{
    return url().query().isEmpty() ? emptyString() : url().queryWithLeadingQuestionMark().toString();
}

String Location::origin() const
{
    return SecurityOrigin::create(url())->toString();
}

Ref<DOMStringList> Location::ancestorOrigins() const
{
    auto origins = DOMStringList::create();
    RefPtr frame = this->frame();
    if (!frame)
        return origins;
    for (RefPtr ancestor = frame->tree().parent(); ancestor; ancestor = ancestor->tree().parent()) {
        if (RefPtr origin = ancestor->frameDocumentSecurityOrigin())
            origins->append(origin->toString());
    }
    return origins;
}

String Location::hash() const
{
    return url().fragmentIdentifier().isEmpty() ? emptyString() : url().fragmentIdentifierWithLeadingNumberSign().toString();
}

ExceptionOr<void> Location::setHref(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& url)
{
    if (!frame())
        return { };
    return setLocation(incumbentWindow, firstWindow, url);
}

ExceptionOr<void> Location::setProtocol(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& protocol)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    if (!url.setProtocol(protocol))
        return Exception { ExceptionCode::SyntaxError };
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setHost(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& host)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    url.setHostAndPort(host);
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setHostname(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& hostname)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    url.setHost(hostname);
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setPort(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& portString)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    url.setPort(parseInteger<uint16_t>(portString));
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setPathname(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& pathname)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    url.setPath(pathname);
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setSearch(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& search)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    URL url = localFrame->document()->url();
    url.setQuery(search);
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::setHash(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& hash)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return { };
    ASSERT(localFrame->document());
    auto url = localFrame->document()->url();
    auto oldFragmentIdentifier = url.fragmentIdentifier();
    StringView newFragmentIdentifier { hash };
    if (hash.startsWith('#'))
        newFragmentIdentifier = newFragmentIdentifier.substring(1);
    url.setFragmentIdentifier(newFragmentIdentifier);
    // Note that by parsing the URL and *then* comparing fragments, we are 
    // comparing fragments post-canonicalization, and so this handles the 
    // cases where fragment identifiers are ignored or invalid. 
    if (equalIgnoringNullity(oldFragmentIdentifier, url.fragmentIdentifier()))
        return { };
    return setLocation(incumbentWindow, firstWindow, url.string());
}

ExceptionOr<void> Location::assign(LocalDOMWindow& activeWindow, LocalDOMWindow& firstWindow, const String& url)
{
    if (!frame())
        return { };
    return setLocation(activeWindow, firstWindow, url);
}

ExceptionOr<void> Location::replace(LocalDOMWindow& activeWindow, LocalDOMWindow& firstWindow, const String& urlString)
{
    RefPtr frame = this->frame();
    if (!frame)
        return { };
    ASSERT(frame->window());

    RefPtr firstFrame = firstWindow.localFrame();
    if (!firstFrame || !firstFrame->document())
        return { };

    URL completedURL = firstFrame->document()->completeURL(urlString);
    if (!completedURL.isValid())
        return Exception { ExceptionCode::SyntaxError };

    auto canNavigateState = activeWindow.document()->canNavigate(frame.get(), completedURL);
    if (canNavigateState == CanNavigateState::Unable)
        return Exception { ExceptionCode::SecurityError };

    // We call LocalDOMWindow::setLocation directly here because replace() always operates on the current frame.
    frame->window()->setLocation(activeWindow, completedURL, NavigationHistoryBehavior::Replace, SetLocationLocking::LockHistoryAndBackForwardList, canNavigateState);
    return { };
}

void Location::reload(LocalDOMWindow& activeWindow)
{
    RefPtr localFrame = dynamicDowncast<LocalFrame>(frame());
    if (!localFrame)
        return;

    ASSERT(activeWindow.document());
    ASSERT(localFrame->document());
    ASSERT(localFrame->document()->window());

    Ref activeDocument = *activeWindow.document();
    Ref targetDocument = *localFrame->document();

    // FIXME: It's not clear this cross-origin security check is valuable.
    // We allow one page to change the location of another. Why block attempts to reload?
    // Other location operations simply block use of JavaScript URLs cross origin.
    if (!activeDocument->protectedSecurityOrigin()->isSameOriginDomain(targetDocument->protectedSecurityOrigin())) {
        Ref targetWindow = *targetDocument->window();
        targetWindow->printErrorMessage(targetWindow->crossDomainAccessErrorMessage(activeWindow, IncludeTargetOrigin::Yes));
        return;
    }

    if (targetDocument->url().protocolIsJavaScript())
        return;

    if (targetDocument->quirks().shouldDelayReloadWhenRegisteringServiceWorker()) {
        if (RefPtr container = targetDocument->serviceWorkerContainer()) {
            container->whenRegisterJobsAreFinished([localFrame, activeDocument] {
                localFrame->protectedNavigationScheduler()->scheduleRefresh(activeDocument);
            });
            return;
        }
    }

    localFrame->protectedNavigationScheduler()->scheduleRefresh(activeDocument);
}

ExceptionOr<void> Location::setLocation(LocalDOMWindow& incumbentWindow, LocalDOMWindow& firstWindow, const String& urlString)
{
    RefPtr frame = this->frame();
    ASSERT(frame);

    RefPtr firstFrame = firstWindow.localFrame();
    if (!firstFrame || !firstFrame->document())
        return { };

    URL completedURL = firstFrame->document()->completeURL(urlString);

    if (!completedURL.isValid())
        return Exception { ExceptionCode::SyntaxError, "Invalid URL"_s };

    auto canNavigateState = incumbentWindow.document()->canNavigate(frame.get(), completedURL);
    if (canNavigateState == CanNavigateState::Unable)
        return Exception { ExceptionCode::SecurityError };

    // https://html.spec.whatwg.org/multipage/nav-history-apis.html#the-location-interface:location-object-navigate
    auto historyHandling = NavigationHistoryBehavior::Auto;
    if (!firstFrame->loader().isComplete() && firstFrame->document() && !firstFrame->document()->window()->hasTransientActivation())
        historyHandling = NavigationHistoryBehavior::Replace;

    ASSERT(frame->window());
    frame->window()->setLocation(incumbentWindow, completedURL, historyHandling, SetLocationLocking::LockHistoryBasedOnGestureState, canNavigateState);
    return { };
}

RefPtr<DOMWindow> Location::protectedWindow()
{
    return m_window.get();
}

} // namespace WebCore
