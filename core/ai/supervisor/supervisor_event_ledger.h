#ifndef DESKTOP_PET_SUPERVISOR_EVENT_LEDGER_H
#define DESKTOP_PET_SUPERVISOR_EVENT_LEDGER_H

#include "ai/event/event_ledger.h"

class SupervisorRuntimeAdapter;

class SupervisorEventLedger final : public EventLedger {
public:
    explicit SupervisorEventLedger(SupervisorRuntimeAdapter* adapter);

    Result<EventRecord, DomainError> append(const EventDraft& draft) override;
    Result<QList<EventRecord>, DomainError> readAfter(
        qint64 sequence, const EventFilter& filter, int limit) const override;

private:
    SupervisorRuntimeAdapter* m_adapter = nullptr;
};

#endif // DESKTOP_PET_SUPERVISOR_EVENT_LEDGER_H
