#ifndef DESKTOP_PET_SUPERVISOR_PROTOCOL_TYPES_H
#define DESKTOP_PET_SUPERVISOR_PROTOCOL_TYPES_H

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

#include "ai/domain/domain_result.h"
#include "ai/event/event_types.h"
#include "ai_types.h"

struct RuntimeBootstrap {
    QString pipeName;
    QString bootstrapToken;
    QString profileId;
    QString candidateId;
    int protocolVersion = 1;
};

struct RuntimeContext {
    QString sessionToken;
    QString sessionId;
    QString profileId;
    QString candidateId;
    QString producerCommit;
    int protocolVersion = 0;
    int heartbeatIntervalMs = 0;
    int maxFrameBytes = 0;
    QHash<QString, QString> leaseMap;
};

enum class StateMutationOperation {
    Upsert,
    Suppress
};

struct StateMutation {
    QString domain;
    QString documentId;
    int schemaVersion = 1;
    qint64 expectedRevision = 0;
    StateMutationOperation operation = StateMutationOperation::Upsert;
    std::optional<QJsonObject> body;
};

struct AppendReceipt {
    EventRecord event;
    QString integrityHash;
    QHash<QString, qint64> stateRevisions;
};

struct StateQuery {
    QString domain;
    QStringList documentIds;
    std::optional<qint64> atOrBeforeSequence;
    QString cursor;
    int limit = 50;
};

struct StateDocument {
    QString domain;
    QString documentId;
    int schemaVersion = 0;
    qint64 revision = 0;
    qint64 updatedSequence = 0;
    bool understood = false;
    QJsonObject body;
};

struct StatePage {
    QList<StateDocument> documents;
    QString nextCursor;
};

struct ModelRouteRequest {
    ModelRole role = ModelRole::Dialogue;
    QList<ChatMessage> messages;
    QJsonArray tools;
    int maxOutputTokens = 0;
    QString modelPolicyId;
    QString privacyClass;
    QString idempotencyKey;
};

struct CapabilityInvocation {
    QString type;
    QString operation;
    QString scopeRef;
    QJsonObject arguments;
    QString idempotencyKey;
};

struct CapabilityResult {
    QString resultDigest;
    QJsonObject observedUsage;
    QJsonObject sanitizedData;
};

struct SupervisorProtocolError {
    QString code;
    QString message;
    bool retryable = false;
    QString correlationId;
    QJsonObject details;
};

struct SupervisorTransportRequest {
    QString action;
    QJsonObject payload;
};

struct SupervisorTransportResponse {
    QJsonObject payload;
    std::optional<SupervisorProtocolError> error;
};

class SupervisorRuntimeTransport {
public:
    virtual ~SupervisorRuntimeTransport() = default;
    virtual Result<SupervisorTransportResponse, DomainError> exchange(
        const SupervisorTransportRequest& request) = 0;
    virtual void close() {}
};

enum class RuntimeAuthorityMode {
    LegacyDevelopment,
    SupervisorRequired
};

struct RuntimeCompositionPolicy {
    RuntimeAuthorityMode authorityMode = RuntimeAuthorityMode::LegacyDevelopment;
    SupervisorRuntimeTransport* supervisorTransport = nullptr;
};

inline RuntimeCompositionPolicy compiledRuntimeCompositionPolicy() {
    RuntimeCompositionPolicy policy;
#if defined(DESKTOP_PET_ENABLE_LEGACY_RUNTIME) && DESKTOP_PET_ENABLE_LEGACY_RUNTIME
    policy.authorityMode = RuntimeAuthorityMode::LegacyDevelopment;
#else
    policy.authorityMode = RuntimeAuthorityMode::SupervisorRequired;
#endif
    return policy;
}

QString supervisorModelRoleName(ModelRole role);
QJsonObject supervisorChatMessageToJson(const ChatMessage& message);
QJsonObject supervisorStateMutationToJson(const StateMutation& mutation);
DomainError supervisorDomainError(const SupervisorProtocolError& error);

#endif // DESKTOP_PET_SUPERVISOR_PROTOCOL_TYPES_H
