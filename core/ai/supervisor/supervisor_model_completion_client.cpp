#include "supervisor_model_completion_client.h"

#include "supervisor_runtime_adapter.h"

SupervisorModelCompletionClient::SupervisorModelCompletionClient(
    SupervisorRuntimeAdapter* adapter, RoleResolver roleResolver)
    : m_adapter(adapter)
    , m_roleResolver(std::move(roleResolver)) {}

void SupervisorModelCompletionClient::completeOnce(
    const ModelRouteConfig& route,
    const QList<ChatMessage>& messages,
    const QJsonArray& tools,
    LlmCompletionHandler callback,
    const QString& petName) {
    Q_UNUSED(petName)
    if (!callback) return;
    if (!m_adapter || !m_roleResolver) {
        callback(false, {}, QStringLiteral(
            "SUPERVISOR_NOT_READY: trusted model route is unavailable"));
        return;
    }
    ModelRouteRequest request;
    request.role = m_roleResolver(route);
    request.messages = messages;
    request.tools = tools;
    request.maxOutputTokens = route.llm.maxTokens;
    request.modelPolicyId = route.routeId;
    const auto completed = m_adapter->completeModel(request);
    if (!completed.isOk()) {
        callback(false, {}, completed.error().code + QStringLiteral(": ")
            + completed.error().message);
        return;
    }
    callback(true, completed.value().response, {});
}
