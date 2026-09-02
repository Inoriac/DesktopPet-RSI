#include "supervisor_event_ledger.h"

#include "supervisor_runtime_adapter.h"

SupervisorEventLedger::SupervisorEventLedger(SupervisorRuntimeAdapter* adapter)
    : m_adapter(adapter) {}

Result<EventRecord, DomainError> SupervisorEventLedger::append(
    const EventDraft& draft) {
    if (!m_adapter) {
        return Result<EventRecord, DomainError>::failure(
            domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                        QStringLiteral("supervisor event ledger is unavailable")));
    }
    const auto appended = m_adapter->appendEvent(draft, {});
    if (!appended.isOk()) {
        return Result<EventRecord, DomainError>::failure(appended.error());
    }
    return Result<EventRecord, DomainError>::success(appended.value().event);
}

Result<QList<EventRecord>, DomainError> SupervisorEventLedger::readAfter(
    qint64 sequence, const EventFilter& filter, int limit) const {
    Q_UNUSED(sequence)
    Q_UNUSED(filter)
    Q_UNUSED(limit)
    return Result<QList<EventRecord>, DomainError>::failure(
        domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                    QStringLiteral("trusted event reads require a state projection")));
}
