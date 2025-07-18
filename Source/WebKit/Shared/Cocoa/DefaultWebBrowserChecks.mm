/*
 * Copyright (C) 2020-2021 Apple Inc. All rights reserved.
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

#import "config.h"
#import "DefaultWebBrowserChecks.h"

#import "AuxiliaryProcess.h"
#import "Connection.h"
#import "Logging.h"
#import <WebCore/RegistrableDomain.h>
#import <wtf/HashMap.h>
#import <wtf/NeverDestroyed.h>
#import <wtf/RunLoop.h>
#import <wtf/RuntimeApplicationChecks.h>
#import <wtf/WorkQueue.h>
#import <wtf/cocoa/Entitlements.h>
#import <wtf/cocoa/RuntimeApplicationChecksCocoa.h>
#import <wtf/text/StringHash.h>

#import "TCCSoftLink.h"

namespace WebKit {

static bool isFullWebBrowserOrRunningTest(const String&);

static bool treatAsNonBrowser(const String& bundleID)
{
    return bundleID == "inAppBrowserPrivacyTestIdentifier"_s;
}

bool isRunningTest(const String& bundleID)
{
    return bundleID == "com.apple.WebKit.TestWebKitAPI"_s || bundleID == "com.apple.WebKit.WebKitTestRunner"_s || bundleID == "org.webkit.WebKitTestRunnerApp"_s;
}

std::span<const WebCore::RegistrableDomain> appBoundDomainsForTesting(const String& bundleID)
{
    if (bundleID == "inAppBrowserPrivacyTestIdentifier"_s) {
        static NeverDestroyed domains = std::array {
            WebCore::RegistrableDomain::uncheckedCreateFromRegistrableDomainString("127.0.0.1"_s),
        };
        return domains.get();
    }
    return { };
}

#if ASSERT_ENABLED
static bool isInWebKitChildProcess()
{
    static bool isInSubProcess;

    static dispatch_once_t once;
    dispatch_once(&once, ^{
#if USE(EXTENSIONKIT)
        isInSubProcess |= WTF::processHasEntitlement("com.apple.developer.web-browser-engine.networking"_s)
            || WTF::processHasEntitlement("com.apple.developer.web-browser-engine.rendering"_s)
            || WTF::processHasEntitlement("com.apple.developer.web-browser-engine.webcontent"_s);
        if (isInSubProcess)
            return;
#endif // USE(EXTENSIONKIT)
        NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
        isInSubProcess = [bundleIdentifier hasPrefix:@"com.apple.WebKit.WebContent"]
            || [bundleIdentifier hasPrefix:@"com.apple.WebKit.Networking"]
            || [bundleIdentifier hasPrefix:@"com.apple.WebKit.GPU"];
#if ENABLE(MODEL_PROCESS)
        isInSubProcess = isInSubProcess || [bundleIdentifier hasPrefix:@"com.apple.WebKit.Model"];
#endif // ENABLE(MODEL_PROCESS)
    });

    return isInSubProcess;
}
#endif

enum class TrackingPreventionState : uint8_t {
    Uninitialized,
    Enabled,
    Disabled
};

static std::atomic<TrackingPreventionState> currentTrackingPreventionState = TrackingPreventionState::Uninitialized;

bool hasRequestedCrossWebsiteTrackingPermission()
{
    ASSERT(!isInWebKitChildProcess());

    static std::atomic<bool> hasRequestedCrossWebsiteTrackingPermission = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSCrossWebsiteTrackingUsageDescription"];
    return hasRequestedCrossWebsiteTrackingPermission;
}

static bool determineTrackingPreventionStateInternal(bool appWasLinkedOnOrAfter, const String& bundleIdentifier)
{
    ASSERT(!RunLoop::isMain());
    ASSERT(!isInWebKitChildProcess());

#if ENABLE(APP_BOUND_DOMAINS)
    bool isFullWebBrowser = isFullWebBrowserOrRunningTest(bundleIdentifier);
#else
    bool isFullWebBrowser = isRunningTest(bundleIdentifier);
#endif
    if (!appWasLinkedOnOrAfter && !isFullWebBrowser)
        return false;

    if (!isFullWebBrowser && !hasRequestedCrossWebsiteTrackingPermission())
        return true;

    TCCAccessPreflightResult result = kTCCAccessPreflightDenied;
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(VISION)
    result = TCCAccessPreflight(get_TCC_kTCCServiceWebKitIntelligentTrackingPrevention(), nullptr);
#endif
    return result != kTCCAccessPreflightDenied;
}

static RefPtr<WorkQueue>& itpQueue()
{
    static NeverDestroyed<RefPtr<WorkQueue>> itpQueue;
    return itpQueue;
}

void determineTrackingPreventionState()
{
    ASSERT(RunLoop::isMain());
    if (currentTrackingPreventionState != TrackingPreventionState::Uninitialized)
        return;

    bool appWasLinkedOnOrAfter = linkedOnOrAfterSDKWithBehavior(SDKAlignedBehavior::SessionCleanupByDefault);

    Ref queue = WorkQueue::create("com.apple.WebKit.itpCheckQueue"_s);
    itpQueue() = queue.copyRef();
    queue->dispatch([appWasLinkedOnOrAfter, bundleIdentifier = applicationBundleIdentifier().isolatedCopy()] {
        currentTrackingPreventionState = determineTrackingPreventionStateInternal(appWasLinkedOnOrAfter, bundleIdentifier) ? TrackingPreventionState::Enabled : TrackingPreventionState::Disabled;
        RunLoop::mainSingleton().dispatch([] {
            itpQueue() = nullptr;
        });
    });
}

bool doesAppHaveTrackingPreventionEnabled()
{
    ASSERT(!isInWebKitChildProcess());
    ASSERT(RunLoop::isMain());
    // If we're still computing the ITP state on the background thread, then synchronize with it.
    if (RefPtr queue = itpQueue())
        queue->dispatchSync([] { });
    ASSERT(currentTrackingPreventionState != TrackingPreventionState::Uninitialized);
    return currentTrackingPreventionState == TrackingPreventionState::Enabled;
}

bool doesParentProcessHaveTrackingPreventionEnabled(AuxiliaryProcess& auxiliaryProcess, bool hasRequestedCrossWebsiteTrackingPermission)
{
    ASSERT(isInWebKitChildProcess());
    ASSERT(RunLoop::isMain());

    if (!isParentProcessAFullWebBrowser(auxiliaryProcess) && !hasRequestedCrossWebsiteTrackingPermission)
        return true;

    static bool trackingPreventionEnabled { true };
    static dispatch_once_t once;
    dispatch_once(&once, ^{

        TCCAccessPreflightResult result = kTCCAccessPreflightDenied;
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(VISION)
        RefPtr<IPC::Connection> connection = auxiliaryProcess.parentProcessConnection();
        if (!connection) {
            ASSERT_NOT_REACHED();
            RELEASE_LOG_ERROR(IPC, "Unable to get parent process connection");
            return;
        }

        auto auditToken = connection->getAuditToken();
        if (!auditToken) {
            ASSERT_NOT_REACHED();
            RELEASE_LOG_ERROR(IPC, "Unable to get parent process audit token");
            return;
        }
        result = TCCAccessPreflightWithAuditToken(get_TCC_kTCCServiceWebKitIntelligentTrackingPrevention(), auditToken.value(), nullptr);
#endif
        trackingPreventionEnabled = result != kTCCAccessPreflightDenied;
    });
    return trackingPreventionEnabled;
}

static std::atomic<bool> hasCheckedUsageStrings = false;
bool hasProhibitedUsageStrings()
{
    ASSERT(!isInWebKitChildProcess());

    static bool hasProhibitedUsageStrings = false;

    if (hasCheckedUsageStrings)
        return hasProhibitedUsageStrings;

    RetainPtr<NSDictionary> infoDictionary = [[NSBundle mainBundle] infoDictionary];
    RELEASE_ASSERT(infoDictionary);

    // See <rdar://problem/59979468> for details about how this list was selected.
    auto prohibitedStrings = @[
        @"NSHomeKitUsageDescription",
        @"NSBluetoothAlwaysUsageDescription",
        @"NSPhotoLibraryUsageDescription",
        @"NSHealthShareUsageDescription",
        @"NSHealthUpdateUsageDescription",
        @"NSLocationAlwaysUsageDescription",
        @"NSLocationAlwaysAndWhenInUseUsageDescription"
    ];

    for (NSString *prohibitedString : prohibitedStrings) {
        if ([infoDictionary objectForKey:prohibitedString]) {
            String message = adoptNS([[NSString alloc] initWithFormat:@"[In-App Browser Privacy] %@ used prohibited usage string %@.", [[NSBundle mainBundle] bundleIdentifier], prohibitedString]).get();
            WTFLogAlways(message.utf8().data());
            hasProhibitedUsageStrings = true;
            break;
        }
    }
    hasCheckedUsageStrings = true;
    return hasProhibitedUsageStrings;
}

bool isParentProcessAFullWebBrowser(AuxiliaryProcess& auxiliaryProcess)
{
    ASSERT(isInWebKitChildProcess());

    static bool fullWebBrowser { false };
    static dispatch_once_t once;
    dispatch_once(&once, ^{
        RefPtr<IPC::Connection> connection = auxiliaryProcess.parentProcessConnection();
        if (!connection) {
            ASSERT_NOT_REACHED();
            RELEASE_LOG_ERROR(IPC, "Unable to get parent process connection");
            return;
        }

        auto auditToken = connection->getAuditToken();
        if (!auditToken) {
            ASSERT_NOT_REACHED();
            RELEASE_LOG_ERROR(IPC, "Unable to get parent process audit token");
            return;
        }

        fullWebBrowser = WTF::hasEntitlement(*auditToken, "com.apple.developer.web-browser"_s);
    });

    auto bundleID = applicationBundleIdentifier();

    if (isRunningTest(bundleID))
        return true;

    return fullWebBrowser && !treatAsNonBrowser(bundleID);
}

bool isFullWebBrowserOrRunningTest(const String& bundleIdentifier)
{
    ASSERT(!isInWebKitChildProcess());

#if ENABLE(APP_BOUND_DOMAINS)
    static bool fullWebBrowser = WTF::processHasEntitlement("com.apple.developer.web-browser"_s);
#elif PLATFORM(MAC)
    static bool fullWebBrowser;
    static std::once_flag once;
    std::call_once(once, [] {
        RetainPtr<NSURL> currentURL = [[NSBundle mainBundle] bundleURL];
        RetainPtr<NSArray<NSURL *>> httpURLs = [[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:[NSURL URLWithString:@"http:"]];
        bool canOpenHTTP = [httpURLs containsObject:currentURL.get()];
        RetainPtr<NSArray<NSURL *>> httpsURLs = [[NSWorkspace sharedWorkspace] URLsForApplicationsToOpenURL:[NSURL URLWithString:@"https:"]];
        bool canOpenHTTPS = [httpsURLs containsObject:currentURL.get()];
        fullWebBrowser = canOpenHTTPS && canOpenHTTP;
    });
#else
    ASSERT_NOT_REACHED();
    static bool fullWebBrowser = false;
#endif

    if (isRunningTest(bundleIdentifier))
        return true;

    return fullWebBrowser && !treatAsNonBrowser(bundleIdentifier);
}

bool shouldEvaluateJavaScriptWithoutTransientActivation()
{
    static bool staticShouldEvaluateJavaScriptWithoutTransientActivation = [] {
        if (linkedOnOrAfterSDKWithBehavior(SDKAlignedBehavior::EvaluateJavaScriptWithoutTransientActivation))
            return true;

        return isFullWebBrowserOrRunningTest();
    }();

    return staticShouldEvaluateJavaScriptWithoutTransientActivation;
}

bool isFullWebBrowserOrRunningTest()
{
    ASSERT(!isInWebKitChildProcess());
    ASSERT(RunLoop::isMain());

    return isFullWebBrowserOrRunningTest(applicationBundleIdentifier());
}

} // namespace WebKit
