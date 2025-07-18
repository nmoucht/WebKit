/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
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

#include <wtf/Function.h>
#include <wtf/Noncopyable.h>
#include <wtf/WeakPtr.h>

namespace WTF {
template<typename> class Observer;
}

namespace WTF {
template<typename T> struct IsDeprecatedWeakRefSmartPointerException;
template<typename Out, typename... In> struct IsDeprecatedWeakRefSmartPointerException<WTF::Observer<Out(In...)>> : std::true_type { };
}

namespace WTF {

template<typename> class Observer;

template <typename Out, typename... In>
class Observer<Out(In...)> : public CanMakeWeakPtr<Observer<Out(In...)>> {
    WTF_MAKE_NONCOPYABLE(Observer);
    WTF_DEPRECATED_MAKE_FAST_ALLOCATED(Observer);
public:
    Observer(Function<Out(In...)>&& callback)
        : m_callback(WTFMove(callback))
    {
        ASSERT(m_callback);
    }

    Out operator()(In... in) const
    {
        ASSERT(m_callback);
        return m_callback(std::forward<In>(in)...);
    }

private:
    Function<Out(In...)> m_callback;
};

}

using WTF::Observer;

