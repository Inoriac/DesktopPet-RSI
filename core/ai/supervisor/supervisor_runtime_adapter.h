#ifndef DESKTOP_PET_SUPERVISOR_RUNTIME_ADAPTER_H
#define DESKTOP_PET_SUPERVISOR_RUNTIME_ADAPTER_H

#include "supervisor_protocol_types.h"

class SupervisorRuntimeAdapter {
public:
    explicit SupervisorRuntimeAdapter(SupervisorRuntimeTransport* transport);
    virtual ~SupervisorRuntimeAdapter() = default;
    SupervisorRuntimeAdapter(const SupervisorRuntimeAdapter&) = delete;
    SupervisorRuntimeAdapter& operator=(const SupervisorRuntimeAdapter&) = delete;

    virtual Result<RuntimeContext, DomainError> initialize(
        const RuntimeBootstrap& bootstrap);
    virtual Result<AppendReceipt, DomainError> appendEvent(
        const EventDraft& event, const QList<StateMutation>& mutations);
    virtual Result<StatePage, DomainError> queryState(const StateQuery& query);
    virtual Result<ModelCompletion, DomainError> completeModel(
        const ModelRouteRequest& request);
    virtual Result<CapabilityResult, DomainError> invokeCapability(
        const CapabilityInvocation& invocation);

    bool isReady() const { return m_context.has_value(); }
    const std::optional<RuntimeContext>& context() const { return m_context; }
    qint64 lastAcknowledgedSequence() const { return m_lastAcknowledgedSequence; }
    qint64 lastObservedSequence() const { return m_lastObservedSequence; }
    qint64 cachedRevision(const QString& domain, const QString& documentId) const;
    void close();

private:
    Result<QJsonObject, DomainError> exchange(const QString& action,
                                              QJsonObject payload,
                                              bool requiresReady = true);
    DomainError malformedResponse(const QString& message);
    QString revisionKey(const QString& domain, const QString& documentId) const;
    void resetState();

    SupervisorRuntimeTransport* m_transport = nullptr;
    std::optional<RuntimeContext> m_context;
    qint64 m_lastAcknowledgedSequence = 0;
    qint64 m_lastObservedSequence = 0;
    QHash<QString, qint64> m_revisionCache;
};

#endif // DESKTOP_PET_SUPERVISOR_RUNTIME_ADAPTER_H
