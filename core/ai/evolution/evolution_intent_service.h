#ifndef DESKTOP_PET_EVOLUTION_INTENT_SERVICE_H
#define DESKTOP_PET_EVOLUTION_INTENT_SERVICE_H

#include <functional>

#include "evolution_opportunity_policy.h"

class EvolutionSupervisorClient {
public:
    virtual ~EvolutionSupervisorClient() = default;
    // These calls cross the PET trust boundary. The TCB implementation must
    // revalidate evidence, time, budget, cooldown and uniqueness atomically.
    virtual Result<IntentReceipt, DomainError> persistIntent(
        const EvolutionIntentDraft& draft) = 0;
    virtual Result<EvolutionOpportunityState, DomainError> opportunityState(
        const IntentId& intentId) = 0;
    virtual Result<OpportunityDecision, DomainError> createCandidate(
        const IntentId& intentId, const OpportunityEstimate& estimate) = 0;
    virtual Result<ExtraOpportunityState, DomainError> extraOpportunityState(
        const IntentId& intentId) = 0;
    virtual Result<HelpOrBudgetRequest, DomainError> createExtraOpportunity(
        const IntentId& intentId, const QString& motivation) = 0;
};

using EvolutionDeadlineHandler =
    std::function<void(const QString& key, const QDateTime& deadline)>;

class EvolutionIntentService {
public:
    explicit EvolutionIntentService(
        EvolutionSupervisorClient* supervisor,
        EvolutionOpportunityPolicy policy = {},
        EvolutionDeadlineHandler deadlineHandler = {});

    Result<IntentReceipt, DomainError> recordIntent(
        const EvolutionIntentDraft& draft);
    Result<OpportunityDecision, DomainError> requestOpportunity(
        const IntentId& intentId, const OpportunityEstimate& estimate);
    Result<HelpOrBudgetRequest, DomainError> requestExtraOpportunity(
        const IntentId& intentId, const QString& motivation);

private:
    DomainError unavailableError() const;
    void scheduleNext(const IntentId& intentId, const QDateTime& deadline) const;

    EvolutionSupervisorClient* m_supervisor = nullptr;
    EvolutionOpportunityPolicy m_policy;
    EvolutionDeadlineHandler m_deadlineHandler;
};

#endif // DESKTOP_PET_EVOLUTION_INTENT_SERVICE_H
