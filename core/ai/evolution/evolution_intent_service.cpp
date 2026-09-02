#include "evolution_intent_service.h"

#include <utility>

namespace {

constexpr int kMaximumIntentTextLength = 8 * 1024;

} // namespace

EvolutionIntentService::EvolutionIntentService(
    EvolutionSupervisorClient* supervisor,
    EvolutionOpportunityPolicy policy,
    EvolutionDeadlineHandler deadlineHandler)
    : m_supervisor(supervisor),
      m_policy(std::move(policy)),
      m_deadlineHandler(std::move(deadlineHandler)) {}

Result<IntentReceipt, DomainError> EvolutionIntentService::recordIntent(
    const EvolutionIntentDraft& draft) {
    if (draft.evidenceIds.isEmpty()) {
        return Result<IntentReceipt, DomainError>::failure(
            domainError(QStringLiteral("EVIDENCE_INVALID"),
                        QStringLiteral("evolution intent requires evidence")));
    }
    for (const QString& evidenceId : draft.evidenceIds) {
        if (evidenceId.trimmed().isEmpty()) {
            return Result<IntentReceipt, DomainError>::failure(
                domainError(QStringLiteral("EVIDENCE_INVALID"),
                            QStringLiteral("evolution evidence id is empty")));
        }
    }
    if (draft.goal.trimmed().isEmpty()
        || draft.goal.size() > kMaximumIntentTextLength
        || draft.expectedValue.size() > kMaximumIntentTextLength) {
        return Result<IntentReceipt, DomainError>::failure(
            domainError(QStringLiteral("PROTOCOL_INVALID"),
                        QStringLiteral("evolution intent text is invalid")));
    }
    if (!m_supervisor) {
        return Result<IntentReceipt, DomainError>::failure(unavailableError());
    }
    return m_supervisor->persistIntent(draft);
}

Result<OpportunityDecision, DomainError>
EvolutionIntentService::requestOpportunity(
    const IntentId& intentId,
    const OpportunityEstimate& estimate) {
    if (!m_supervisor) {
        return Result<OpportunityDecision, DomainError>::failure(unavailableError());
    }
    const auto loaded = m_supervisor->opportunityState(intentId);
    if (!loaded.isOk()) {
        return Result<OpportunityDecision, DomainError>::failure(loaded.error());
    }
    EvolutionOpportunityState state = loaded.value();
    const auto policyDecision = m_policy.decide(state, estimate);
    if (!policyDecision.isOk()) return policyDecision;
    if (!policyDecision.value().granted) {
        scheduleNext(intentId, policyDecision.value().nextEligibleAt);
        return policyDecision;
    }

    const auto created = m_supervisor->createCandidate(intentId, estimate);
    if (created.isOk()) scheduleNext(intentId, {});
    return created;
}

Result<HelpOrBudgetRequest, DomainError>
EvolutionIntentService::requestExtraOpportunity(
    const IntentId& intentId,
    const QString& motivation) {
    if (!m_supervisor) {
        return Result<HelpOrBudgetRequest, DomainError>::failure(unavailableError());
    }
    if (motivation.trimmed().isEmpty()
        || motivation.size() > kMaximumIntentTextLength) {
        return Result<HelpOrBudgetRequest, DomainError>::failure(
            domainError(QStringLiteral("PROTOCOL_INVALID"),
                        QStringLiteral("extra opportunity motivation is invalid")));
    }

    const auto loaded = m_supervisor->extraOpportunityState(intentId);
    if (!loaded.isOk()) {
        return Result<HelpOrBudgetRequest, DomainError>::failure(loaded.error());
    }
    ExtraOpportunityState state = loaded.value();
    const ExtraOpportunityDecision decision = m_policy.decideExtra(state);
    if (!decision.allowed) {
        HelpOrBudgetRequest deferred;
        deferred.nextEligibleAt = decision.nextEligibleAt;
        scheduleNext(intentId, decision.nextEligibleAt);
        return Result<HelpOrBudgetRequest, DomainError>::success(deferred);
    }

    return m_supervisor->createExtraOpportunity(intentId, motivation);
}

DomainError EvolutionIntentService::unavailableError() const {
    return domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                       QStringLiteral("evolution supervisor is unavailable"));
}

void EvolutionIntentService::scheduleNext(
    const IntentId& intentId,
    const QDateTime& deadline) const {
    if (m_deadlineHandler) {
        m_deadlineHandler(QStringLiteral("evolution:") + intentId, deadline);
    }
}
