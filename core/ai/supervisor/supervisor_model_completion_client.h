#ifndef DESKTOP_PET_SUPERVISOR_MODEL_COMPLETION_CLIENT_H
#define DESKTOP_PET_SUPERVISOR_MODEL_COMPLETION_CLIENT_H

#include <functional>

#include "ai/model/model_router.h"

class SupervisorRuntimeAdapter;

class SupervisorModelCompletionClient final : public ModelCompletionClient {
public:
    using RoleResolver = std::function<ModelRole(const ModelRouteConfig&)>;

    explicit SupervisorModelCompletionClient(
        SupervisorRuntimeAdapter* adapter,
        RoleResolver roleResolver = {});

    void completeOnce(const ModelRouteConfig& route,
                      const QList<ChatMessage>& messages,
                      const QJsonArray& tools,
                      LlmCompletionHandler callback,
                      const QString& petName) override;

private:
    SupervisorRuntimeAdapter* m_adapter = nullptr;
    RoleResolver m_roleResolver;
};

#endif // DESKTOP_PET_SUPERVISOR_MODEL_COMPLETION_CLIENT_H
