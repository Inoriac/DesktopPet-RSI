#include "supervisor_runtime_adapter.h"

#include <QJsonDocument>
#include <QJsonValue>
#include <QUuid>

#include <algorithm>

namespace {

bool hasText(const QJsonObject& object, const QString& key) {
    return object.value(key).isString() && !object.value(key).toString().isEmpty();
}

QJsonArray stringArray(const QStringList& values) {
    QJsonArray result;
    for (const QString& value : values) result.append(value);
    return result;
}

} // namespace

SupervisorRuntimeAdapter::SupervisorRuntimeAdapter(
    SupervisorRuntimeTransport* transport)
    : m_transport(transport) {}

Result<QJsonObject, DomainError> SupervisorRuntimeAdapter::exchange(
    const QString& action, QJsonObject payload, bool requiresReady) {
    if (!m_transport || (requiresReady && !m_context.has_value())) {
        return Result<QJsonObject, DomainError>::failure(
            domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                        QStringLiteral("trusted supervisor runtime is not initialized")));
    }
    if (requiresReady) {
        payload.insert(QStringLiteral("sessionToken"), m_context->sessionToken);
    }
    const auto exchanged = m_transport->exchange({action, std::move(payload)});
    if (!exchanged.isOk()) {
        if (!requiresReady) {
            return Result<QJsonObject, DomainError>::failure(
                domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                            QStringLiteral("trusted supervisor transport is unavailable"),
                            {{QStringLiteral("transportCode"), exchanged.error().code}}));
        }
        close();
        return Result<QJsonObject, DomainError>::failure(
            domainError(QStringLiteral("LEDGER_UNAVAILABLE"),
                        QStringLiteral("trusted supervisor transport was disconnected"),
                        {{QStringLiteral("transportCode"), exchanged.error().code}}));
    }
    const SupervisorTransportResponse& response = exchanged.value();
    if (response.error.has_value()) {
        return Result<QJsonObject, DomainError>::failure(
            supervisorDomainError(*response.error));
    }
    return Result<QJsonObject, DomainError>::success(response.payload);
}

DomainError SupervisorRuntimeAdapter::malformedResponse(const QString& message) {
    close();
    return domainError(QStringLiteral("PROTOCOL_INVALID"), message);
}

Result<RuntimeContext, DomainError> SupervisorRuntimeAdapter::initialize(
    const RuntimeBootstrap& bootstrap) {
    resetState();
    if (bootstrap.protocolVersion != 1 || bootstrap.bootstrapToken.isEmpty()
        || bootstrap.profileId.isEmpty() || bootstrap.candidateId.isEmpty()) {
        return Result<RuntimeContext, DomainError>::failure(
            domainError(QStringLiteral("PROTOCOL_INVALID"),
                        QStringLiteral("runtime bootstrap is incomplete")));
    }
    QJsonObject payload{
        {QStringLiteral("bootstrapToken"), bootstrap.bootstrapToken},
        {QStringLiteral("profileId"), bootstrap.profileId},
        {QStringLiteral("candidateId"), bootstrap.candidateId},
        {QStringLiteral("claimedRole"), QStringLiteral("runtime")},
        {QStringLiteral("clientNonce"),
         QUuid::createUuid().toString(QUuid::WithoutBraces)},
        {QStringLiteral("protocolVersion"), bootstrap.protocolVersion}
    };
    const auto exchanged = exchange(QStringLiteral("hello"), std::move(payload), false);
    if (!exchanged.isOk()) {
        return Result<RuntimeContext, DomainError>::failure(exchanged.error());
    }
    const QJsonObject& object = exchanged.value();
    if (!hasText(object, QStringLiteral("sessionToken"))
        || !hasText(object, QStringLiteral("runtimeSessionId"))
        || !hasText(object, QStringLiteral("profileId"))
        || !hasText(object, QStringLiteral("candidateId"))
        || !hasText(object, QStringLiteral("producerCommit"))
        || object.value(QStringLiteral("protocolVersion")).toInt() != 1
        || object.value(QStringLiteral("profileId")).toString() != bootstrap.profileId
        || object.value(QStringLiteral("candidateId")).toString() != bootstrap.candidateId) {
        return Result<RuntimeContext, DomainError>::failure(
            malformedResponse(QStringLiteral("supervisor hello response is invalid")));
    }
    RuntimeContext context;
    context.sessionToken = object.value(QStringLiteral("sessionToken")).toString();
    context.sessionId = object.value(QStringLiteral("runtimeSessionId")).toString();
    context.profileId = object.value(QStringLiteral("profileId")).toString();
    context.candidateId = object.value(QStringLiteral("candidateId")).toString();
    context.producerCommit = object.value(QStringLiteral("producerCommit")).toString();
    context.protocolVersion = object.value(QStringLiteral("protocolVersion")).toInt();
    context.heartbeatIntervalMs =
        object.value(QStringLiteral("heartbeatIntervalMs")).toInt();
    context.maxFrameBytes = object.value(QStringLiteral("maxFrameBytes")).toInt();
    const QJsonObject leases = object.value(QStringLiteral("leaseMap")).toObject();
    for (auto it = leases.begin(); it != leases.end(); ++it) {
        if (it.value().isString() && !it.value().toString().isEmpty()) {
            context.leaseMap.insert(it.key(), it.value().toString());
        }
    }
    m_context = context;
    return Result<RuntimeContext, DomainError>::success(std::move(context));
}

Result<AppendReceipt, DomainError> SupervisorRuntimeAdapter::appendEvent(
    const EventDraft& event, const QList<StateMutation>& mutations) {
    QJsonObject eventObject{
        {QStringLiteral("eventType"), event.type},
        {QStringLiteral("schemaVersion"), event.schemaVersion},
        {QStringLiteral("payload"), event.payload},
        {QStringLiteral("privacy"), eventPrivacyToString(event.privacy)}
    };
    if (!event.eventId.isEmpty()) {
        eventObject.insert(QStringLiteral("eventId"), event.eventId);
    }
    QJsonArray mutationArray;
    for (const StateMutation& mutation : mutations) {
        mutationArray.append(supervisorStateMutationToJson(mutation));
    }
    eventObject.insert(QStringLiteral("stateMutations"), mutationArray);
    const auto exchanged = exchange(QStringLiteral("append_event"), std::move(eventObject));
    if (!exchanged.isOk()) {
        return Result<AppendReceipt, DomainError>::failure(exchanged.error());
    }
    const QJsonObject& object = exchanged.value();
    const qint64 sequence = object.value(QStringLiteral("sequence")).toInteger();
    const QDateTime createdAt = QDateTime::fromString(
        object.value(QStringLiteral("createdAt")).toString(), Qt::ISODateWithMs);
    if (!hasText(object, QStringLiteral("eventId")) || sequence <= 0
        || !createdAt.isValid()) {
        return Result<AppendReceipt, DomainError>::failure(
            malformedResponse(QStringLiteral("append receipt is invalid")));
    }
    AppendReceipt receipt;
    static_cast<EventDraft&>(receipt.event) = event;
    receipt.event.eventId = object.value(QStringLiteral("eventId")).toString();
    receipt.event.sequence = sequence;
    receipt.event.createdAt = createdAt.toUTC();
    receipt.integrityHash = object.value(QStringLiteral("integrityHash")).toString();
    const QJsonObject revisions = object.value(QStringLiteral("stateRevisions")).toObject();
    for (auto it = revisions.begin(); it != revisions.end(); ++it) {
        if (!it.value().isDouble() || it.value().toInteger() < 0) {
            return Result<AppendReceipt, DomainError>::failure(
                malformedResponse(QStringLiteral("append state revision is invalid")));
        }
        receipt.stateRevisions.insert(it.key(), it.value().toInteger());
    }
    m_lastAcknowledgedSequence = std::max(m_lastAcknowledgedSequence, sequence);
    m_lastObservedSequence = std::max(m_lastObservedSequence, sequence);
    for (auto it = receipt.stateRevisions.cbegin(); it != receipt.stateRevisions.cend(); ++it) {
        m_revisionCache.insert(it.key(), it.value());
    }
    return Result<AppendReceipt, DomainError>::success(std::move(receipt));
}

Result<StatePage, DomainError> SupervisorRuntimeAdapter::queryState(
    const StateQuery& query) {
    QJsonObject payload{
        {QStringLiteral("domain"), query.domain},
        {QStringLiteral("documentIds"), stringArray(query.documentIds)},
        {QStringLiteral("cursor"), query.cursor},
        {QStringLiteral("limit"), query.limit}
    };
    if (query.atOrBeforeSequence.has_value()) {
        payload.insert(QStringLiteral("atOrBeforeSequence"),
                       *query.atOrBeforeSequence);
    }
    const auto exchanged = exchange(QStringLiteral("query_state"), std::move(payload));
    if (!exchanged.isOk()) {
        return Result<StatePage, DomainError>::failure(exchanged.error());
    }
    const QJsonValue documentsValue =
        exchanged.value().value(QStringLiteral("documents"));
    if (!documentsValue.isArray()) {
        return Result<StatePage, DomainError>::failure(
            malformedResponse(QStringLiteral("state page is invalid")));
    }
    StatePage page;
    page.nextCursor = exchanged.value().value(QStringLiteral("nextCursor")).toString();
    qint64 observedSequence = m_lastObservedSequence;
    QHash<QString, qint64> observedRevisions;
    for (const QJsonValue& value : documentsValue.toArray()) {
        if (!value.isObject()) {
            return Result<StatePage, DomainError>::failure(
                malformedResponse(QStringLiteral("state document is invalid")));
        }
        const QJsonObject object = value.toObject();
        if (!hasText(object, QStringLiteral("domain"))
            || !hasText(object, QStringLiteral("documentId"))
            || object.value(QStringLiteral("schemaVersion")).toInt() <= 0
            || object.value(QStringLiteral("revision")).toInteger() < 0
            || object.value(QStringLiteral("updatedSequence")).toInteger() < 0
            || !object.value(QStringLiteral("body")).isObject()) {
            return Result<StatePage, DomainError>::failure(
                malformedResponse(QStringLiteral("state document is invalid")));
        }
        StateDocument document;
        document.domain = object.value(QStringLiteral("domain")).toString();
        document.documentId = object.value(QStringLiteral("documentId")).toString();
        document.schemaVersion = object.value(QStringLiteral("schemaVersion")).toInt();
        document.revision = object.value(QStringLiteral("revision")).toInteger();
        document.updatedSequence =
            object.value(QStringLiteral("updatedSequence")).toInteger();
        document.understood = object.value(QStringLiteral("understood")).toBool(false);
        document.body = object.value(QStringLiteral("body")).toObject();
        observedSequence = std::max(observedSequence, document.updatedSequence);
        observedRevisions.insert(revisionKey(document.domain, document.documentId),
                                 document.revision);
        page.documents.append(std::move(document));
    }
    m_lastObservedSequence = observedSequence;
    for (auto it = observedRevisions.cbegin(); it != observedRevisions.cend(); ++it) {
        m_revisionCache.insert(it.key(), it.value());
    }
    return Result<StatePage, DomainError>::success(std::move(page));
}

Result<ModelCompletion, DomainError> SupervisorRuntimeAdapter::completeModel(
    const ModelRouteRequest& request) {
    QJsonArray messages;
    for (const ChatMessage& message : request.messages) {
        messages.append(supervisorChatMessageToJson(message));
    }
    QJsonObject payload{
        {QStringLiteral("role"), supervisorModelRoleName(request.role)},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("tools"), request.tools},
        {QStringLiteral("maxOutputTokens"), request.maxOutputTokens},
        {QStringLiteral("modelPolicyId"), request.modelPolicyId},
        {QStringLiteral("privacyClass"), request.privacyClass},
        {QStringLiteral("idempotencyKey"), request.idempotencyKey}
    };
    payload.insert(QStringLiteral("profileId"), m_context ? m_context->profileId : QString());
    payload.insert(QStringLiteral("candidateId"),
                   m_context ? m_context->candidateId : QString());
    const auto exchanged = exchange(QStringLiteral("complete_model"), std::move(payload));
    if (!exchanged.isOk()) {
        return Result<ModelCompletion, DomainError>::failure(exchanged.error());
    }
    const QJsonObject& object = exchanged.value();
    if (!hasText(object, QStringLiteral("completionId"))
        || !object.value(QStringLiteral("content")).isString()) {
        return Result<ModelCompletion, DomainError>::failure(
            malformedResponse(QStringLiteral("model completion response is invalid")));
    }
    ModelCompletion completion;
    completion.response.id = object.value(QStringLiteral("completionId")).toString();
    completion.response.model = object.value(QStringLiteral("model")).toString();
    completion.response.content = object.value(QStringLiteral("content")).toString();
    completion.response.reasoningContent =
        object.value(QStringLiteral("reasoningContent")).toString();
    completion.response.finishReason =
        object.value(QStringLiteral("finishReason")).toString();
    const QJsonObject usage = object.value(QStringLiteral("usage")).toObject();
    completion.response.usage.promptTokens =
        usage.value(QStringLiteral("inputTokens")).toInteger();
    completion.response.usage.completionTokens =
        usage.value(QStringLiteral("outputTokens")).toInteger();
    completion.response.usage.totalTokens = completion.response.usage.promptTokens
        + completion.response.usage.completionTokens;
    const QJsonArray toolCalls = object.value(QStringLiteral("toolCalls")).toArray();
    for (const QJsonValue& value : toolCalls) {
        const QJsonObject tool = value.toObject();
        completion.response.toolCalls.append({
            tool.value(QStringLiteral("id")).toString(),
            QStringLiteral("function"),
            tool.value(QStringLiteral("name")).toString(),
            tool.value(QStringLiteral("arguments")).toObject()
        });
    }
    completion.dimensions.role = request.role;
    completion.dimensions.provider = QStringLiteral("supervisor");
    completion.dimensions.model = completion.response.model;
    completion.dimensions.routeId = request.modelPolicyId;
    return Result<ModelCompletion, DomainError>::success(std::move(completion));
}

Result<CapabilityResult, DomainError> SupervisorRuntimeAdapter::invokeCapability(
    const CapabilityInvocation& invocation) {
    if (!m_context.has_value()) {
        return Result<CapabilityResult, DomainError>::failure(
            domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                        QStringLiteral("trusted supervisor runtime is not initialized")));
    }
    const QString leaseId = m_context->leaseMap.value(invocation.type);
    if (leaseId.isEmpty()) {
        return Result<CapabilityResult, DomainError>::failure(
            domainError(QStringLiteral("CAPABILITY_DENIED"),
                        QStringLiteral("no active supervisor capability lease")));
    }
    QJsonObject payload{
        {QStringLiteral("leaseId"), leaseId},
        {QStringLiteral("capabilityType"), invocation.type},
        {QStringLiteral("operation"), invocation.operation},
        {QStringLiteral("scopeRef"), invocation.scopeRef},
        {QStringLiteral("arguments"), invocation.arguments},
        {QStringLiteral("idempotencyKey"), invocation.idempotencyKey}
    };
    const auto exchanged = exchange(QStringLiteral("invoke_capability"), std::move(payload));
    if (!exchanged.isOk()) {
        return Result<CapabilityResult, DomainError>::failure(exchanged.error());
    }
    const QJsonObject& object = exchanged.value();
    if (!hasText(object, QStringLiteral("resultDigest"))
        || !object.value(QStringLiteral("observedUsage")).isObject()
        || !object.value(QStringLiteral("sanitizedData")).isObject()) {
        return Result<CapabilityResult, DomainError>::failure(
            malformedResponse(QStringLiteral("capability response is invalid")));
    }
    return Result<CapabilityResult, DomainError>::success({
        object.value(QStringLiteral("resultDigest")).toString(),
        object.value(QStringLiteral("observedUsage")).toObject(),
        object.value(QStringLiteral("sanitizedData")).toObject()
    });
}

QString SupervisorRuntimeAdapter::revisionKey(
    const QString& domain, const QString& documentId) const {
    return domain + QLatin1Char('/') + documentId;
}

qint64 SupervisorRuntimeAdapter::cachedRevision(
    const QString& domain, const QString& documentId) const {
    return m_revisionCache.value(revisionKey(domain, documentId), -1);
}

void SupervisorRuntimeAdapter::close() {
    if (m_transport) m_transport->close();
    resetState();
}

void SupervisorRuntimeAdapter::resetState() {
    m_context.reset();
    m_lastAcknowledgedSequence = 0;
    m_lastObservedSequence = 0;
    m_revisionCache.clear();
}
