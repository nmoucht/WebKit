/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#include "Connection.h"
#include "MessageReceiver.h"
#include "MessageSender.h"
#include "RetrieveRecordResponseBodyCallbackIdentifier.h"
#include "ServiceWorkerFetchTask.h"
#include <WebCore/ExceptionOr.h>
#include <WebCore/FetchIdentifier.h>
#include <WebCore/NavigationPreloadState.h>
#include <WebCore/ProcessIdentifier.h>
#include <WebCore/PushPermissionState.h>
#include <WebCore/PushSubscriptionData.h>
#include <WebCore/SWServer.h>
#include <pal/SessionID.h>
#include <wtf/CheckedRef.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/TZoneMalloc.h>
#include <wtf/WeakPtr.h>

namespace IPC {
class FormDataReference;

template<> struct AsyncReplyError<WebCore::ExceptionOr<bool>> {
    static WebCore::ExceptionOr<bool> create() { return WebCore::Exception { WebCore::ExceptionCode::TypeError, "Internal error"_s }; }
};

}

namespace WebCore {
class ServiceWorkerRegistrationKey;
struct CookieChangeSubscription;
struct ClientOrigin;
struct ExceptionData;
struct MessageWithMessagePorts;
struct ServiceWorkerClientData;
struct ServiceWorkerRoute;
}

namespace WebKit {

class NetworkConnectionToWebProcess;
class NetworkProcess;
class NetworkResourceLoader;
class ServiceWorkerFetchTask;

struct NetworkResourceLoadParameters;
struct SharedPreferencesForWebProcess;

class WebSWServerConnection final : public WebCore::SWServer::Connection, public IPC::MessageSender, public IPC::MessageReceiver {
    WTF_MAKE_TZONE_ALLOCATED(WebSWServerConnection);
public:
    static Ref<WebSWServerConnection> create(NetworkConnectionToWebProcess&, WebCore::SWServer&, IPC::Connection&, WebCore::ProcessIdentifier);
    WebSWServerConnection(const WebSWServerConnection&) = delete;
    ~WebSWServerConnection() final;

    void ref() const final { WebCore::SWServer::Connection::ref(); }
    void deref() const final { WebCore::SWServer::Connection::deref(); }

    USING_CAN_MAKE_WEAKPTR(WebCore::SWServer::Connection);

    IPC::Connection& ipcConnection() const { return m_contentConnection.get(); }

    void didReceiveMessage(IPC::Connection&, IPC::Decoder&) final;

    std::optional<SharedPreferencesForWebProcess> sharedPreferencesForWebProcess() const;

    NetworkSession* session();
    CheckedPtr<NetworkSession> checkedSession();
    PAL::SessionID sessionID() const;

    RefPtr<ServiceWorkerFetchTask> createFetchTask(NetworkResourceLoader&, const WebCore::ResourceRequest&);
    void fetchTaskTimedOut(WebCore::ServiceWorkerIdentifier);

    void transferServiceWorkerLoadToNewWebProcess(NetworkResourceLoader&, WebCore::SWServerRegistration&, const WebCore::ResourceRequest&);
    std::optional<WebCore::SWServer::GatheredClientData> gatherClientData(WebCore::ScriptExecutionContextIdentifier);

    void registerServiceWorkerClient(WebCore::ClientOrigin&&, WebCore::ServiceWorkerClientData&&, const std::optional<WebCore::ServiceWorkerRegistrationIdentifier>&, String&& userAgent);
    void registerServiceWorkerClientInternal(WebCore::ClientOrigin&&, WebCore::ServiceWorkerClientData&&, const std::optional<WebCore::ServiceWorkerRegistrationIdentifier>&, String&& userAgent, WebCore::SWServer::IsBeingCreatedClient);
    void unregisterServiceWorkerClient(const WebCore::ScriptExecutionContextIdentifier&);

#if ENABLE(CONTENT_EXTENSIONS)
    void reportNetworkUsageToWorkerClient(WebCore::ScriptExecutionContextIdentifier, uint64_t bytesTransferredOverNetworkDelta);
#endif

private:
    WebSWServerConnection(NetworkConnectionToWebProcess&, WebCore::SWServer&, IPC::Connection&, WebCore::ProcessIdentifier);

    // Implement SWServer::Connection (Messages to the client WebProcess)
    void rejectJobInClient(WebCore::ServiceWorkerJobIdentifier, const WebCore::ExceptionData&) final;
    void resolveRegistrationJobInClient(WebCore::ServiceWorkerJobIdentifier, const WebCore::ServiceWorkerRegistrationData&, WebCore::ShouldNotifyWhenResolved) final;
    void resolveUnregistrationJobInClient(WebCore::ServiceWorkerJobIdentifier, const WebCore::ServiceWorkerRegistrationKey&, bool unregistrationResult) final;
    void startScriptFetchInClient(WebCore::ServiceWorkerJobIdentifier, const WebCore::ServiceWorkerRegistrationKey&, WebCore::FetchOptions::Cache) final;
    void updateRegistrationStateInClient(WebCore::ServiceWorkerRegistrationIdentifier, WebCore::ServiceWorkerRegistrationState, const std::optional<WebCore::ServiceWorkerData>&) final;
    void updateWorkerStateInClient(WebCore::ServiceWorkerIdentifier, WebCore::ServiceWorkerState) final;
    void fireUpdateFoundEvent(WebCore::ServiceWorkerRegistrationIdentifier) final;
    void setRegistrationLastUpdateTime(WebCore::ServiceWorkerRegistrationIdentifier, WallTime) final;
    void setRegistrationUpdateViaCache(WebCore::ServiceWorkerRegistrationIdentifier, WebCore::ServiceWorkerUpdateViaCache) final;
    void notifyClientsOfControllerChange(const HashSet<WebCore::ScriptExecutionContextIdentifier>& contextIdentifiers, const std::optional<WebCore::ServiceWorkerData>& newController);
    void focusServiceWorkerClient(WebCore::ScriptExecutionContextIdentifier, CompletionHandler<void(std::optional<WebCore::ServiceWorkerClientData>&&)>&&) final;

    void scheduleJobInServer(WebCore::ServiceWorkerJobData&&);

    using UnregisterJobResult = Expected<bool, WebCore::ExceptionData>;
    void scheduleUnregisterJobInServer(WebCore::ServiceWorkerJobIdentifier, WebCore::ServiceWorkerRegistrationIdentifier, WebCore::ServiceWorkerOrClientIdentifier, CompletionHandler<void(UnregisterJobResult&&)>&&);

    void startFetch(ServiceWorkerFetchTask&, WebCore::SWServerWorker&);

    void matchRegistration(const WebCore::SecurityOriginData& topOrigin, const URL& clientURL, CompletionHandler<void(std::optional<WebCore::ServiceWorkerRegistrationData>&&)>&&);
    void whenRegistrationReady(const WebCore::SecurityOriginData& topOrigin, const URL& clientURL, CompletionHandler<void(std::optional<WebCore::ServiceWorkerRegistrationData>&&)>&&);
    void getRegistrations(const WebCore::SecurityOriginData& topOrigin, const URL& clientURL, CompletionHandler<void(const Vector<WebCore::ServiceWorkerRegistrationData>&)>&&);

    void terminateWorkerFromClient(WebCore::ServiceWorkerIdentifier, CompletionHandler<void()>&&);
    void whenServiceWorkerIsTerminatedForTesting(WebCore::ServiceWorkerIdentifier, CompletionHandler<void()>&&);

    void postMessageToServiceWorkerClient(WebCore::ScriptExecutionContextIdentifier destinationContextIdentifier, const WebCore::MessageWithMessagePorts&, WebCore::ServiceWorkerIdentifier sourceServiceWorkerIdentifier, const String& sourceOrigin) final;

    void contextConnectionCreated(WebCore::SWServerToContextConnection&) final;
    void updateBackgroundFetchRegistration(const WebCore::BackgroundFetchInformation&) final;

    bool isThrottleable() const { return m_isThrottleable; }
    bool hasMatchingClient(const WebCore::RegistrableDomain&) const;
    bool computeThrottleState(const WebCore::RegistrableDomain&) const;
    void setThrottleState(bool isThrottleable);
    void updateThrottleState();

    void subscribeToPushService(WebCore::ServiceWorkerRegistrationIdentifier, Vector<uint8_t>&& applicationServerKey, CompletionHandler<void(Expected<WebCore::PushSubscriptionData, WebCore::ExceptionData>&&)>&&);
    void unsubscribeFromPushService(WebCore::ServiceWorkerRegistrationIdentifier, WebCore::PushSubscriptionIdentifier,  CompletionHandler<void(Expected<bool, WebCore::ExceptionData>&&)>&&);
    void getPushSubscription(WebCore::ServiceWorkerRegistrationIdentifier, CompletionHandler<void(Expected<std::optional<WebCore::PushSubscriptionData>, WebCore::ExceptionData>&&)>&&);
    void getPushPermissionState(WebCore::ServiceWorkerRegistrationIdentifier, CompletionHandler<void(Expected<uint8_t, WebCore::ExceptionData>&&)>&&);

    void postMessageToServiceWorker(WebCore::ServiceWorkerIdentifier destination, WebCore::MessageWithMessagePorts&&, const WebCore::ServiceWorkerOrClientIdentifier& source);
    void controlClient(const NetworkResourceLoadParameters&, WebCore::SWServerRegistration&, const WebCore::ResourceRequest&, WebCore::ProcessIdentifier);

    using ExceptionOrVoidCallback = CompletionHandler<void(std::optional<WebCore::ExceptionData>&&)>;
    void enableNavigationPreload(WebCore::ServiceWorkerRegistrationIdentifier, ExceptionOrVoidCallback&&);
    void disableNavigationPreload(WebCore::ServiceWorkerRegistrationIdentifier, ExceptionOrVoidCallback&&);
    void setNavigationPreloadHeaderValue(WebCore::ServiceWorkerRegistrationIdentifier, String&&, ExceptionOrVoidCallback&&);
    using ExceptionOrNavigationPreloadStateCallback = CompletionHandler<void(Expected<WebCore::NavigationPreloadState, WebCore::ExceptionData>&&)>;
    void getNavigationPreloadState(WebCore::ServiceWorkerRegistrationIdentifier, ExceptionOrNavigationPreloadStateCallback&&);

    void retrieveRecordResponseBody(WebCore::BackgroundFetchRecordIdentifier, RetrieveRecordResponseBodyCallbackIdentifier);

    void addCookieChangeSubscriptions(WebCore::ServiceWorkerRegistrationIdentifier, Vector<WebCore::CookieChangeSubscription>&&, ExceptionOrVoidCallback&&);
    void removeCookieChangeSubscriptions(WebCore::ServiceWorkerRegistrationIdentifier, Vector<WebCore::CookieChangeSubscription>&&, ExceptionOrVoidCallback&&);
    using ExceptionOrCookieChangeSubscriptionsCallback = CompletionHandler<void(Expected<Vector<WebCore::CookieChangeSubscription>, WebCore::ExceptionData>&&)>;
    void cookieChangeSubscriptions(WebCore::ServiceWorkerRegistrationIdentifier, ExceptionOrCookieChangeSubscriptionsCallback&&);
    void addRoutes(WebCore::ServiceWorkerRegistrationIdentifier, Vector<WebCore::ServiceWorkerRoute>&&, CompletionHandler<void(Expected<void, WebCore::ExceptionData>&&)>&&);

#if ENABLE(WEB_PUSH_NOTIFICATIONS)
    void getNotifications(const URL& registrationURL, const String& tag, CompletionHandler<void(Expected<Vector<WebCore::NotificationData>, WebCore::ExceptionData>&&)>&&);
#endif

    void checkTopOrigin(const WebCore::SecurityOriginData&);

    URL clientURLFromIdentifier(WebCore::ServiceWorkerOrClientIdentifier);

    IPC::Connection* messageSenderConnection() const final { return m_contentConnection.ptr(); }
    uint64_t messageSenderDestinationID() const final { return 0; }
    
    template<typename U> static void sendToContextProcess(WebCore::SWServerToContextConnection&, U&& message);
    NetworkProcess& networkProcess();
    Ref<NetworkProcess> protectedNetworkProcess();

    bool isWebSWServerConnection() const final { return true; }

    WeakPtr<NetworkConnectionToWebProcess> m_networkConnectionToWebProcess;
    const Ref<IPC::Connection> m_contentConnection;
    HashMap<WebCore::ScriptExecutionContextIdentifier, WebCore::ClientOrigin> m_clientOrigins;
    HashMap<WebCore::ServiceWorkerJobIdentifier, CompletionHandler<void(UnregisterJobResult&&)>> m_unregisterJobs;
    bool m_isThrottleable { true };
};

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_BEGIN(WebKit::WebSWServerConnection)
    static bool isType(const WebCore::SWServer::Connection& connection) { return connection.isWebSWServerConnection(); }
SPECIALIZE_TYPE_TRAITS_END()
