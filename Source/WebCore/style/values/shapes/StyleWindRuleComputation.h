/*
 * Copyright (C) 2024 Samuel Weinig <sam@webkit.org>
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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "StyleValueTypes.h"
#include "WindRule.h"

namespace WebCore {
namespace Style {

// All types that want to expose a generated WebCore::WindRule must specialize WindRuleComputation the following member function:
//
//    template<> struct WebCore::Style::WindRuleComputation<StyleType> {
//        WebCore::WindRule operator()(const StyleType&);
//    };

template<typename StyleType> struct WindRuleComputation;

struct WindRuleComputationInvoker {
    template<typename StyleType> WebCore::WindRule operator()(const StyleType& value) const
    {
        return WindRuleComputation<StyleType>{}(value);
    }
};
inline constexpr WindRuleComputationInvoker windRule{};

// Specialization for `FunctionNotation`.
template<CSSValueID Name, typename StyleType> struct WindRuleComputation<FunctionNotation<Name, StyleType>> {
    WebCore::WindRule operator()(const FunctionNotation<Name, StyleType>& value)
    {
        return WebCore::Style::windRule(value.parameters);
    }
};

} // namespace Style
} // namespace WebCore
