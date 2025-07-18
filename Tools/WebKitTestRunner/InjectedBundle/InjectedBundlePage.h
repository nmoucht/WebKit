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

#ifndef InjectedBundlePage_h
#define InjectedBundlePage_h

#include <WebKit/WKBundlePage.h>
#include <WebKit/WKBundleScriptWorld.h>
#include <WebKit/WKRetainPtr.h>
#include <wtf/text/WTFString.h>

namespace WTR {

class InjectedBundlePage {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(InjectedBundlePage);
public:
    InjectedBundlePage(WKBundlePageRef);
    ~InjectedBundlePage();

    WKBundlePageRef page() const { return m_page; }

    void notifyDone();
    void forceImmediateCompletion();
    void dump(bool forceRepaint);

    void resetAfterTest();

    String dumpHistory();

    static uint64_t responseHeaderCount(WKURLResponseRef);

private:
    // Loader Client
    static void didStartProvisionalLoadForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didReceiveServerRedirectForProvisionalLoadForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didFailProvisionalLoadWithErrorForFrame(WKBundlePageRef, WKBundleFrameRef, WKErrorRef, WKTypeRef*, const void*);
    static void didCommitLoadForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didFinishLoadForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didFinishProgress(WKBundlePageRef, const void*);
    static void didFinishDocumentLoadForFrame(WKBundlePageRef, WKBundleFrameRef,  WKTypeRef*, const void*);
    static void didFailLoadWithErrorForFrame(WKBundlePageRef, WKBundleFrameRef, WKErrorRef, WKTypeRef*, const void*);
    static void didReceiveTitleForFrame(WKBundlePageRef, WKStringRef title, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didClearWindowForFrame(WKBundlePageRef, WKBundleFrameRef, WKBundleScriptWorldRef, const void*);
    static void didCancelClientRedirectForFrame(WKBundlePageRef, WKBundleFrameRef, const void*);
    static void willPerformClientRedirectForFrame(WKBundlePageRef, WKBundleFrameRef, WKURLRef url, double delay, double date, const void*);
    static void didSameDocumentNavigationForFrame(WKBundlePageRef, WKBundleFrameRef, WKSameDocumentNavigationType, WKTypeRef*, const void*);
    static void didHandleOnloadEventsForFrame(WKBundlePageRef, WKBundleFrameRef, const void*);
    static void didDisplayInsecureContentForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didRunInsecureContentForFrame(WKBundlePageRef, WKBundleFrameRef, WKTypeRef*, const void*);
    static void didInitiateLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLRequestRef, bool pageLoadIsProvisional, const void*);
    static WKURLRequestRef willSendRequestForFrame(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLRequestRef, WKURLResponseRef, const void*);
    static void didReceiveResponseForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLResponseRef, const void*);
    static void didReceiveContentLengthForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, uint64_t length, const void*);
    static void didFinishLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, const void*);
    static void didFailLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKErrorRef, const void*);
    static bool shouldCacheResponse(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, const void*);
    static void willInjectUserScriptForFrame(WKBundlePageRef, WKBundleFrameRef, WKBundleScriptWorldRef, const void*);

    void didStartProvisionalLoadForFrame(WKBundleFrameRef);
    void didReceiveServerRedirectForProvisionalLoadForFrame(WKBundleFrameRef);
    void didFailProvisionalLoadWithErrorForFrame(WKBundleFrameRef, WKErrorRef);
    void didCommitLoadForFrame(WKBundleFrameRef);
    void didFinishLoadForFrame(WKBundleFrameRef);
    void didFinishProgress();
    void didFailLoadWithErrorForFrame(WKBundleFrameRef, WKErrorRef);
    void didReceiveTitleForFrame(WKStringRef title, WKBundleFrameRef);
    void didClearWindowForFrame(WKBundleFrameRef, WKBundleScriptWorldRef);
    void didCancelClientRedirectForFrame(WKBundleFrameRef);
    void willPerformClientRedirectForFrame(WKBundlePageRef, WKBundleFrameRef, WKURLRef, double delay, double date);
    void didSameDocumentNavigationForFrame(WKBundleFrameRef, WKSameDocumentNavigationType);
    void didFinishDocumentLoadForFrame(WKBundleFrameRef);
    void didHandleOnloadEventsForFrame(WKBundleFrameRef);
    void didDisplayInsecureContentForFrame(WKBundleFrameRef);
    void didRunInsecureContentForFrame(WKBundleFrameRef);
    void willInjectUserScriptForFrame();

    // Resource Load Client
    void didInitiateLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLRequestRef, bool pageLoadIsProvisional);
    WKURLRequestRef willSendRequestForFrame(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLRequestRef, WKURLResponseRef);
    void didReceiveResponseForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKURLResponseRef);
    void didReceiveContentLengthForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, uint64_t length);
    void didFinishLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier);
    void didFailLoadForResource(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier, WKErrorRef);
    bool shouldCacheResponse(WKBundlePageRef, WKBundleFrameRef, uint64_t identifier);

    // Editor client
    static bool shouldBeginEditing(WKBundlePageRef, WKBundleRangeHandleRef, const void* clientInfo);
    static bool shouldEndEditing(WKBundlePageRef, WKBundleRangeHandleRef, const void* clientInfo);
    static bool shouldInsertNode(WKBundlePageRef, WKBundleNodeHandleRef, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType, const void* clientInfo);
    static bool shouldInsertText(WKBundlePageRef, WKStringRef, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType, const void* clientInfo);
    static bool shouldDeleteRange(WKBundlePageRef, WKBundleRangeHandleRef, const void* clientInfo);
    static bool shouldChangeSelectedRange(WKBundlePageRef, WKBundleRangeHandleRef fromRange, WKBundleRangeHandleRef toRange, WKAffinityType, bool stillSelecting, const void* clientInfo);
    static bool shouldApplyStyle(WKBundlePageRef, WKBundleCSSStyleDeclarationRef style, WKBundleRangeHandleRef range, const void* clientInfo);
    static void didBeginEditing(WKBundlePageRef, WKStringRef notificationName, const void* clientInfo);
    static void didEndEditing(WKBundlePageRef, WKStringRef notificationName, const void* clientInfo);
    static void didChange(WKBundlePageRef, WKStringRef notificationName, const void* clientInfo);
    static void didChangeSelection(WKBundlePageRef, WKStringRef notificationName, const void* clientInfo);
    bool shouldBeginEditing(WKBundleRangeHandleRef);
    bool shouldEndEditing(WKBundleRangeHandleRef);
    bool shouldInsertNode(WKBundleNodeHandleRef, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType);
    bool shouldInsertText(WKStringRef, WKBundleRangeHandleRef rangeToReplace, WKInsertActionType);
    bool shouldDeleteRange(WKBundleRangeHandleRef);
    bool shouldChangeSelectedRange(WKBundleRangeHandleRef fromRange, WKBundleRangeHandleRef toRange, WKAffinityType, bool stillSelecting);
    bool shouldApplyStyle(WKBundleCSSStyleDeclarationRef style, WKBundleRangeHandleRef range);
    void didBeginEditing(WKStringRef notificationName);
    void didEndEditing(WKStringRef notificationName);
    void didChange(WKStringRef notificationName);
    void didChangeSelection(WKStringRef notificationName);

    void dumpAllFramesText(WTF::StringBuilder&);
    void dumpAllFrameScrollPositions(WTF::StringBuilder&);
    void dumpDOMAsWebArchive(WKBundleFrameRef, WTF::StringBuilder&);

    void platformDidStartProvisionalLoadForFrame(WKBundleFrameRef);
    String platformResponseMimeType(WKURLResponseRef);

    void frameDidChangeLocation(WKBundleFrameRef);

    WKBundlePageRef m_page;
    WKRetainPtr<WKBundleScriptWorldRef> m_world;
    bool m_didCommitMainFrameLoad { false };
};

} // namespace WTR

#endif // InjectedBundlePage_h
