/*
 * Copyright (C) 2021 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RTCDataChannelRemoteHandler.h"

#if ENABLE(WEB_RTC)

#include "ProcessQualified.h"
#include "RTCDataChannelHandlerClient.h"
#include "RTCDataChannelRemoteHandlerConnection.h"
#include "ScriptExecutionContextIdentifier.h"
#include "SharedBuffer.h"
#include <wtf/TZoneMallocInlines.h>

namespace WebCore {

WTF_MAKE_TZONE_ALLOCATED_IMPL(RTCDataChannelRemoteHandler);

std::unique_ptr<RTCDataChannelRemoteHandler> RTCDataChannelRemoteHandler::create(RTCDataChannelIdentifier remoteIdentifier, RefPtr<RTCDataChannelRemoteHandlerConnection>&& connection)
{
    if (!connection)
        return nullptr;
    return makeUnique<RTCDataChannelRemoteHandler>(remoteIdentifier, connection.releaseNonNull());
}

RTCDataChannelRemoteHandler::RTCDataChannelRemoteHandler(RTCDataChannelIdentifier remoteIdentifier, Ref<RTCDataChannelRemoteHandlerConnection>&& connection)
    : m_remoteIdentifier(remoteIdentifier)
    , m_connection(WTFMove(connection))
{
}

RTCDataChannelRemoteHandler::~RTCDataChannelRemoteHandler() = default;

void RTCDataChannelRemoteHandler::didChangeReadyState(RTCDataChannelState state)
{
    m_client->didChangeReadyState(state);
}

void RTCDataChannelRemoteHandler::didReceiveStringData(String&& text)
{
    m_client->didReceiveStringData(text);
}

void RTCDataChannelRemoteHandler::didReceiveRawData(std::span<const uint8_t> data)
{
    m_client->didReceiveRawData(data);
}

void RTCDataChannelRemoteHandler::didDetectError(Ref<RTCError>&& error)
{
    m_client->didDetectError(WTFMove(error));
}

void RTCDataChannelRemoteHandler::bufferedAmountIsDecreasing(size_t amount)
{
    m_client->bufferedAmountIsDecreasing(amount);
}

void RTCDataChannelRemoteHandler::readyToSend()
{
    m_isReadyToSend = true;

    for (auto& message : m_pendingMessages)
        m_connection->sendData(m_remoteIdentifier, message.isRaw, message.buffer->makeContiguous()->span());
    m_pendingMessages.clear();

    if (m_isPendingClose)
        m_connection->close(m_remoteIdentifier);
}

void RTCDataChannelRemoteHandler::setClient(RTCDataChannelHandlerClient& client, std::optional<ScriptExecutionContextIdentifier> contextIdentifier)
{
    m_client = &client;
    m_connection->connectToSource(*this, contextIdentifier, *m_localIdentifier, m_remoteIdentifier);
}

bool RTCDataChannelRemoteHandler::sendStringData(const CString& text)
{
    if (!m_isReadyToSend) {
        m_pendingMessages.append(Message { false, SharedBuffer::create(text.span()) });
        return true;
    }
    m_connection->sendData(m_remoteIdentifier, false, byteCast<uint8_t>(text.span()));
    return true;
}

bool RTCDataChannelRemoteHandler::sendRawData(std::span<const uint8_t> data)
{
    if (!m_isReadyToSend) {
        m_pendingMessages.append(Message { true, SharedBuffer::create(data) });
        return true;
    }
    m_connection->sendData(m_remoteIdentifier, true, data);
    return true;
}

void RTCDataChannelRemoteHandler::close()
{
    if (!m_isReadyToSend) {
        m_isPendingClose = true;
        return;
    }
    m_connection->close(m_remoteIdentifier);
}

} // namespace WebCore

#endif // ENABLE(WEB_RTC)
