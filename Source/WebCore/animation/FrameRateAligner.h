/*
 * Copyright (C) 2022 Apple Inc. All rights reserved.
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

#pragma once

#include "AnimationFrameRate.h"
#include "AnimationMalloc.h"
#include "ReducedResolutionSeconds.h"
#include <wtf/HashMap.h>
#include <wtf/Seconds.h>

namespace WebCore {

class FrameRateAligner {
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED_WITH_HEAP_IDENTIFIER(FrameRateAligner, Animation);
public:
    FrameRateAligner();
    ~FrameRateAligner();

    void beginUpdate(ReducedResolutionSeconds, std::optional<FramesPerSecond>);
    void finishUpdate();

    enum class ShouldUpdate : bool { No, Yes };
    ShouldUpdate updateFrameRate(FramesPerSecond);

    std::optional<Seconds> timeUntilNextUpdateForFrameRate(FramesPerSecond, ReducedResolutionSeconds) const;
    std::optional<FramesPerSecond> maximumFrameRate() const;

private:
    struct FrameRateData {
        ReducedResolutionSeconds firstUpdateTime;
        ReducedResolutionSeconds lastUpdateTime;
        bool isNew { true };
    };

    HashMap<FramesPerSecond, FrameRateData> m_frameRates;
    ReducedResolutionSeconds m_timestamp;
};

} // namespace WebCore
