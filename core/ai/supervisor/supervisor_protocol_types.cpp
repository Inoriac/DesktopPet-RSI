#include "supervisor_protocol_types.h"

#include <QSet>

QString supervisorModelRoleName(ModelRole role) {
    switch (role) {
    case ModelRole::Dialogue:
        return QStringLiteral("dialogue");
    case ModelRole::FastExtract:
    case ModelRole::Consolidation:
    case ModelRole::Diary:
    case ModelRole::Daydream:
        return QStringLiteral("reflection");
    case ModelRole::Vision:
        return QStringLiteral("dialogue");
    }
    return QStringLiteral("dialogue");
}

QJsonObject supervisorChatMessageToJson(const ChatMessage& message) {
    QJsonObject object{
        {QStringLiteral("role"), message.role},
        {QStringLiteral("content"), message.content}
    };
    if (!message.name.isEmpty()) object.insert(QStringLiteral("name"), message.name);
    if (!message.toolCallId.isEmpty()) {
        object.insert(QStringLiteral("toolCallId"), message.toolCallId);
    }
    if (!message.toolCalls.isEmpty()) {
        object.insert(QStringLiteral("toolCalls"), message.toolCalls);
    }
    if (!message.contentBlocks.isEmpty()) {
        object.insert(QStringLiteral("contentBlocks"), message.contentBlocks);
    }
    return object;
}

QJsonObject supervisorStateMutationToJson(const StateMutation& mutation) {
    QJsonObject object{
        {QStringLiteral("domain"), mutation.domain},
        {QStringLiteral("documentId"), mutation.documentId},
        {QStringLiteral("schemaVersion"), mutation.schemaVersion},
        {QStringLiteral("expectedRevision"), mutation.expectedRevision},
        {QStringLiteral("operation"),
         mutation.operation == StateMutationOperation::Suppress
             ? QStringLiteral("suppress") : QStringLiteral("upsert")}
    };
    if (mutation.body.has_value()) {
        object.insert(QStringLiteral("body"), *mutation.body);
    }
    return object;
}

DomainError supervisorDomainError(const SupervisorProtocolError& error) {
    static const QSet<QString> knownCodes{
        QStringLiteral("AUTH_FAILED"),
        QStringLiteral("PROTOCOL_INVALID"),
        QStringLiteral("RATE_LIMITED"),
        QStringLiteral("STATE_CONFLICT"),
        QStringLiteral("EVIDENCE_INVALID"),
        QStringLiteral("BUDGET_EXHAUSTED"),
        QStringLiteral("CAPABILITY_DENIED"),
        QStringLiteral("LEDGER_UNAVAILABLE"),
        QStringLiteral("GIT_REMOTE_UNCONFIRMED"),
        QStringLiteral("BUILD_FAILED"),
        QStringLiteral("ARTIFACT_UNTRUSTED"),
        QStringLiteral("HEALTH_HARD_FAILURE"),
        QStringLiteral("OWNER_APPROVAL_REQUIRED"),
        QStringLiteral("OWNER_REJECTED"),
        QStringLiteral("CHECKPOINT_INVALID"),
        QStringLiteral("SUPERVISOR_NOT_READY"),
        QStringLiteral("SUPERVISOR_ERROR"),
        QStringLiteral("INTERNAL_ERROR")
    };
    QJsonObject details = error.details;
    details.insert(QStringLiteral("retryable"), error.retryable);
    if (!error.correlationId.isEmpty()) {
        details.insert(QStringLiteral("correlationId"), error.correlationId);
    }
    QString code = error.code;
    if (!knownCodes.contains(code)) {
        details.insert(QStringLiteral("originalCode"), code);
        code = QStringLiteral("SUPERVISOR_ERROR");
    }
    return domainError(code, error.message, std::move(details));
}
