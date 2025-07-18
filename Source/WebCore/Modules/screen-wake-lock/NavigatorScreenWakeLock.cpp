/*
 * Copyright (C) 2022-2025 Apple Inc. All rights reserved.
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

#include "config.h"
#include "NavigatorScreenWakeLock.h"

#include "Document.h"
#include "Navigator.h"
#include "WakeLock.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(NavigatorScreenWakeLock);

NavigatorScreenWakeLock::NavigatorScreenWakeLock(Navigator& navigator)
    : m_navigator(navigator)
{
}

NavigatorScreenWakeLock::~NavigatorScreenWakeLock() = default;

NavigatorScreenWakeLock* NavigatorScreenWakeLock::from(Navigator& navigator)
{
    auto* supplement = static_cast<NavigatorScreenWakeLock*>(Supplement<Navigator>::from(&navigator, supplementName()));
    if (!supplement) {
        auto newSupplement = makeUnique<NavigatorScreenWakeLock>(navigator);
        supplement = newSupplement.get();
        provideTo(&navigator, supplementName(), WTFMove(newSupplement));
    }
    return supplement;
}

ASCIILiteral NavigatorScreenWakeLock::supplementName()
{
    return "NavigatorScreenWakeLock"_s;
}

WakeLock& NavigatorScreenWakeLock::wakeLock(Navigator& navigator)
{
    return NavigatorScreenWakeLock::from(navigator)->wakeLock();
}

WakeLock& NavigatorScreenWakeLock::wakeLock()
{
    if (!m_wakeLock)
        lazyInitialize(m_wakeLock, WakeLock::create(downcast<Document>(m_navigator->protectedScriptExecutionContext().get())));
    return *m_wakeLock;
}

} // namespace WebCore
