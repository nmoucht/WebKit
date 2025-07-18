/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SettingsBase.h"

#include "AudioSession.h"
#include "BackForwardCache.h"
#include "BackForwardController.h"
#include "CachedResourceLoader.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CookieStorage.h"
#include "DOMTimer.h"
#include "Database.h"
#include "Document.h"
#include "DocumentInlines.h"
#include "FontCache.h"
#include "FrameTree.h"
#include "HistoryItem.h"
#include "LocalFrame.h"
#include "LocalFrameView.h"
#include "Page.h"
#include "RenderObjectInlines.h"
#include "RenderWidget.h"
#include "Settings.h"
#include "StorageMap.h"
#include <limits>
#include <wtf/RuntimeApplicationChecks.h>
#include <wtf/StdLibExtras.h>
#include <wtf/TZoneMallocInlines.h>

#if ENABLE(MEDIA_STREAM)
#include "MockRealtimeMediaSourceCenter.h"
#endif
#if USE(MODERN_AVCONTENTKEYSESSION)
#include "MediaSessionManagerCocoa.h"
#endif

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(SettingsBase);

static void invalidateAfterGenericFamilyChange(Page* page)
{
    // No need to invalidate FontCascadeCaches in worker threads, since workers
    // do not respond to changes in Settings values.
    FontCascadeCache::forCurrentThread().invalidate();

    if (page)
        page->setNeedsRecalcStyleInAllFrames();
}

SettingsBase::SettingsBase(Page* page)
    : m_page(page)
    , m_minimumDOMTimerInterval(DOMTimer::defaultMinimumInterval())
    , m_setImageLoadingSettingsTimer(*this, &SettingsBase::imageLoadingSettingsTimerFired)
{
}

SettingsBase::~SettingsBase() = default;

#if !PLATFORM(COCOA)

void SettingsBase::initializeDefaultFontFamilies()
{
    // Other platforms can set up fonts from a client, but on Mac, we want it in WebCore to share code between WebKit1 and WebKit2.
}

#if ENABLE(MEDIA_SOURCE)
bool SettingsBase::platformDefaultMediaSourceEnabled()
{
    return true;
}

uint64_t SettingsBase::defaultMaximumSourceBufferSize()
{
    // Allow SourceBuffers to store up to 304MB each, enough for approximately five minutes
    // of 1080p video and stereo audio.
    return 318767104;
}
#endif

#endif

const String& SettingsBase::standardFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().standardFontFamily(script);
}

void SettingsBase::setStandardFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setStandardFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::fixedFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().fixedFontFamily(script);
}

void SettingsBase::setFixedFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setFixedFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::serifFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().serifFontFamily(script);
}

void SettingsBase::setSerifFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setSerifFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::sansSerifFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().sansSerifFontFamily(script);
}

void SettingsBase::setSansSerifFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setSansSerifFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::cursiveFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().cursiveFontFamily(script);
}

void SettingsBase::setCursiveFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setCursiveFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::fantasyFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().fantasyFontFamily(script);
}

void SettingsBase::setFantasyFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setFantasyFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

const String& SettingsBase::pictographFontFamily(UScriptCode script) const
{
    return fontGenericFamilies().pictographFontFamily(script);
}

void SettingsBase::setPictographFontFamily(const String& family, UScriptCode script)
{
    bool changes = fontGenericFamilies().setPictographFontFamily(family, script);
    if (changes)
        invalidateAfterGenericFamilyChange(m_page.get());
}

void SettingsBase::setMinimumDOMTimerInterval(Seconds interval)
{
    auto oldTimerInterval = std::exchange(m_minimumDOMTimerInterval, interval);

    if (!m_page)
        return;

    for (RefPtr frame = m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        RefPtr localFrame = dynamicDowncast<LocalFrame>(frame);
        if (!localFrame)
            continue;
        if (RefPtr document = localFrame->document())
            document->adjustMinimumDOMTimerInterval(oldTimerInterval);
    }
}

void SettingsBase::setMediaContentTypesRequiringHardwareSupport(const String& contentTypes)
{
    m_mediaContentTypesRequiringHardwareSupport.shrink(0);
    for (auto type : StringView(contentTypes).split(':'))
        m_mediaContentTypesRequiringHardwareSupport.append(ContentType { type.toString() });
}

void SettingsBase::setMediaContentTypesRequiringHardwareSupport(const Vector<ContentType>& contentTypes)
{
    m_mediaContentTypesRequiringHardwareSupport = contentTypes;
}

void SettingsBase::setAllowedMediaContainerTypes(const String& types)
{
    if (types.isNull()) {
        m_allowedMediaContainerTypes = std::nullopt;
        return;
    }

    Vector<String> newTypes;
    for (auto type : StringView(types).split(','))
        newTypes.append(type.toString());

    m_allowedMediaContainerTypes = WTFMove(newTypes);
}

void SettingsBase::setAllowedMediaCodecTypes(const String& types)
{
    if (types.isNull()) {
        m_allowedMediaCodecTypes = std::nullopt;
        return;
    }

    Vector<String> newTypes;
    for (auto type : StringView(types).split(','))
        newTypes.append(type.toString());

    m_allowedMediaCodecTypes = WTFMove(newTypes);
}

void SettingsBase::setAllowedMediaVideoCodecIDs(const String& types)
{
    if (types.isNull()) {
        m_allowedMediaVideoCodecIDs = std::nullopt;
        return;
    }

    Vector<FourCC> newTypes;
    for (auto type : StringView(types).split(',')) {
        if (auto fourCC = FourCC::fromString(type))
            newTypes.append(WTFMove(*fourCC));
    }

    m_allowedMediaVideoCodecIDs = WTFMove(newTypes);
}

void SettingsBase::setAllowedMediaAudioCodecIDs(const String& types)
{
    if (types.isNull()) {
        m_allowedMediaAudioCodecIDs = std::nullopt;
        return;
    }

    Vector<FourCC> newTypes;
    for (auto type : StringView(types).split(',')) {
        if (auto fourCC = FourCC::fromString(type))
            newTypes.append(WTFMove(*fourCC));
    }

    m_allowedMediaAudioCodecIDs = WTFMove(newTypes);
}

void SettingsBase::setAllowedMediaCaptionFormatTypes(const String& types)
{
    if (types.isNull()) {
        m_allowedMediaCaptionFormatTypes = std::nullopt;
        return;
    }

    Vector<FourCC> newTypes;
    for (auto type : StringView(types).split(',')) {
        if (auto fourCC = FourCC::fromString(type))
            newTypes.append(WTFMove(*fourCC));
    }

    m_allowedMediaCaptionFormatTypes = WTFMove(newTypes);
}

void SettingsBase::resetToConsistentState()
{
    m_minimumDOMTimerInterval = DOMTimer::defaultMinimumInterval();

    setAllowedMediaContainerTypes(std::nullopt);
    setAllowedMediaCodecTypes(std::nullopt);
    setAllowedMediaVideoCodecIDs(std::nullopt);
    setAllowedMediaAudioCodecIDs(std::nullopt);
    setAllowedMediaCaptionFormatTypes(std::nullopt);

#if ENABLE(TEXT_AUTOSIZING)
    m_oneLineTextMultiplierCoefficient = defaultOneLineTextMultiplierCoefficient;
    m_multiLineTextMultiplierCoefficient = defaultMultiLineTextMultiplierCoefficient;
    m_maxTextAutosizingScaleIncrease = defaultMaxTextAutosizingScaleIncrease;
#endif
}

// MARK - onChange handlers

void SettingsBase::setNeedsRecalcStyleInAllFrames()
{
    if (m_page)
        m_page->setNeedsRecalcStyleInAllFrames();
}

void SettingsBase::setNeedsRelayoutAllFrames()
{
    if (!m_page)
        return;

    for (Frame* frame = &m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        auto* localFrame = dynamicDowncast<LocalFrame>(frame);
        if (!localFrame)
            continue;
        if (!localFrame->ownerRenderer())
            continue;
        localFrame->ownerRenderer()->setNeedsLayoutAndPreferredWidthsUpdate();
    }
}

void SettingsBase::updateDisplayEDRHeadroom()
{
#if HAVE(SUPPORT_HDR_DISPLAY)
    if (m_page)
        m_page->updateDisplayEDRHeadroom();
#endif
}

void SettingsBase::mediaTypeOverrideChanged()
{
    RefPtr page = m_page.get();
    if (!page)
        return;

    RefPtr localMainFrame = page->localMainFrame();
    if (!localMainFrame)
        return;

    if (RefPtr view = localMainFrame->view())
        view->setMediaType(AtomString(page->settings().mediaTypeOverride()));

    page->setNeedsRecalcStyleInAllFrames();
}

void SettingsBase::imagesEnabledChanged()
{
    // Changing this setting to true might immediately start new loads for images that had previously had loading disabled.
    // If this happens while a WebView is being dealloc'ed, and we don't know the WebView is being dealloc'ed, these new loads
    // can cause crashes downstream when the WebView memory has actually been free'd.
    // One example where this can happen is in Mac apps that subclass WebView then do work in their overridden dealloc methods.
    // Starting these loads synchronously is not important. By putting it on a 0-delay, properly closing the Page cancels them
    // before they have a chance to really start.
    // See http://webkit.org/b/60572 for more discussion.
    m_setImageLoadingSettingsTimer.startOneShot(0_s);
}

void SettingsBase::imageLoadingSettingsTimerFired()
{
    if (!m_page)
        return;

    for (RefPtr frame = m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        RefPtr localFrame = dynamicDowncast<LocalFrame>(frame);
        if (!localFrame)
            continue;
        RefPtr document = localFrame->document();
        if (!document)
            continue;
        document->protectedCachedResourceLoader()->setImagesEnabled(m_page->settings().areImagesEnabled());
        document->protectedCachedResourceLoader()->setAutoLoadImages(m_page->settings().loadsImagesAutomatically());
    }
}

void SettingsBase::iceCandidateFilteringEnabledChanged()
{
    if (!m_page)
        return;

    if (m_page->settings().iceCandidateFilteringEnabled())
        m_page->enableICECandidateFiltering();
    else
        m_page->disableICECandidateFiltering();
}

#if ENABLE(TEXT_AUTOSIZING)

void SettingsBase::shouldEnableTextAutosizingBoostChanged()
{
    if (!m_page)
        return;

    bool boostAutosizing = m_page->settings().shouldEnableTextAutosizingBoost();
    m_oneLineTextMultiplierCoefficient = boostAutosizing ? boostedOneLineTextMultiplierCoefficient : defaultOneLineTextMultiplierCoefficient;
    m_multiLineTextMultiplierCoefficient = boostAutosizing ? boostedMultiLineTextMultiplierCoefficient : defaultMultiLineTextMultiplierCoefficient;
    m_maxTextAutosizingScaleIncrease = boostAutosizing ? boostedMaxTextAutosizingScaleIncrease : defaultMaxTextAutosizingScaleIncrease;

    setNeedsRecalcStyleInAllFrames();
}

void SettingsBase::textAutosizingUsesIdempotentModeChanged()
{
    if (m_page)
        m_page->chrome().client().textAutosizingUsesIdempotentModeChanged();
    setNeedsRecalcStyleInAllFrames();
}

#endif // ENABLE(TEXT_AUTOSIZING)

#if ENABLE(MEDIA_STREAM)

void SettingsBase::mockCaptureDevicesEnabledChanged()
{
    bool enabled = false;
    if (m_page)
        enabled = m_page->settings().mockCaptureDevicesEnabled();

    MockRealtimeMediaSourceCenter::setMockRealtimeMediaSourceCenterEnabled(enabled);
}

#endif

void SettingsBase::layerBasedSVGEngineEnabledChanged()
{
    if (!m_page)
        return;

    for (RefPtr frame = m_page->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        RefPtr localFrame = dynamicDowncast<LocalFrame>(frame);
        if (!localFrame)
            continue;
        RefPtr document = localFrame->document();
        if (!document)
            continue;

        RefPtr documentElement = document->documentElement();
        if (!documentElement)
            continue;

        documentElement->invalidateStyleAndRenderersForSubtree();
        document->scheduleFullStyleRebuild();
    }
}

void SettingsBase::userStyleSheetLocationChanged()
{
    if (m_page)
        m_page->userStyleSheetLocationChanged();
}

void SettingsBase::usesBackForwardCacheChanged()
{
    if (!m_page)
        return;

    if (!m_page->settings().usesBackForwardCache())
        BackForwardCache::singleton().pruneToSizeNow(0, PruningReason::None);
}

void SettingsBase::storageBlockingPolicyChanged()
{
    if (m_page)
        m_page->storageBlockingStateChanged();
}

void SettingsBase::backgroundShouldExtendBeyondPageChanged()
{
    RefPtr page = m_page.get();
    if (!page)
        return;

    RefPtr localMainFrame = page->localMainFrame();
    if (!localMainFrame)
        return;

    if (RefPtr view = localMainFrame->view())
        view->updateExtendBackgroundIfNecessary();
}

void SettingsBase::scrollingPerformanceTestingEnabledChanged()
{
    RefPtr page = m_page.get();
    if (!page)
        return;

    RefPtr localMainFrame = page->localMainFrame();
    if (!localMainFrame)
        return;

    if (RefPtr view = localMainFrame->view())
        view->setScrollingPerformanceTestingEnabled(page->settings().scrollingPerformanceTestingEnabled());
}

void SettingsBase::hiddenPageDOMTimerThrottlingStateChanged()
{
    if (m_page)
        m_page->hiddenPageDOMTimerThrottlingStateChanged();
}

void SettingsBase::hiddenPageCSSAnimationSuspensionEnabledChanged()
{
    if (m_page)
        m_page->hiddenPageCSSAnimationSuspensionStateChanged();
}

void SettingsBase::resourceUsageOverlayVisibleChanged()
{
#if ENABLE(RESOURCE_USAGE)
    if (m_page)
        m_page->setResourceUsageOverlayVisible(m_page->settings().resourceUsageOverlayVisible());
#endif
}

#if USE(MODERN_AVCONTENTKEYSESSION)
void SettingsBase::shouldUseModernAVContentKeySessionChanged()
{
    if (m_page)
        MediaSessionManagerCocoa::setShouldUseModernAVContentKeySession(m_page->settings().shouldUseModernAVContentKeySession());
}
#endif

void SettingsBase::useSystemAppearanceChanged()
{
    if (m_page)
        m_page->useSystemAppearanceChanged();
}

RefPtr<Page> SettingsBase::protectedPage() const
{
    return m_page.get();
}

void SettingsBase::fontFallbackPrefersPictographsChanged()
{
    invalidateAfterGenericFamilyChange(protectedPage().get());
}

} // namespace WebCore
