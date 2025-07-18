/*
 * Copyright (C) 2024, 2025 Igalia S.L.
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
#include "GraphicsLayerAsyncContentsDisplayDelegateCoordinated.h"

#if USE(COORDINATED_GRAPHICS)
#include "CoordinatedPlatformLayerBufferNativeImage.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerContentsDisplayDelegateCoordinated.h"
#include "ImageBuffer.h"
#include "NativeImage.h"
#include "TextureMapperFlags.h"

namespace WebCore {

GraphicsLayerAsyncContentsDisplayDelegateCoordinated::GraphicsLayerAsyncContentsDisplayDelegateCoordinated(GraphicsLayer& layer)
    : m_delegate(GraphicsLayerContentsDisplayDelegateCoordinated::create())
{
    layer.setContentsDisplayDelegate(m_delegate.ptr(), GraphicsLayer::ContentsLayerPurpose::Canvas);
}

GraphicsLayerAsyncContentsDisplayDelegateCoordinated::~GraphicsLayerAsyncContentsDisplayDelegateCoordinated() = default;

bool GraphicsLayerAsyncContentsDisplayDelegateCoordinated::tryCopyToLayer(ImageBuffer& imageBuffer, bool)
{
    auto image = ImageBuffer::sinkIntoNativeImage(imageBuffer.clone());
    if (!image)
        return false;

    m_delegate->setDisplayBuffer(CoordinatedPlatformLayerBufferNativeImage::create(image.releaseNonNull(), nullptr));
    return true;
}

void GraphicsLayerAsyncContentsDisplayDelegateCoordinated::updateGraphicsLayer(GraphicsLayer& layer)
{
    layer.setContentsDisplayDelegate(m_delegate.ptr(), GraphicsLayer::ContentsLayerPurpose::Canvas);
}

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)
