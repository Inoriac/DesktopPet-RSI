#ifndef DESKTOP_PET_EVOLUTION_OPPORTUNITY_POLICY_H
#define DESKTOP_PET_EVOLUTION_OPPORTUNITY_POLICY_H

#include "evolution_types.h"

struct EvolutionOpportunityPolicyConfig {
    qint64 minimumRestSeconds = 24 * 60 * 60;
    qint64 maximumRestSeconds = 7 * 24 * 60 * 60;
    int minimumIdleSeconds = 20 * 60;
    qint64 naturalWindowSeconds = 30 * 60;
    qint64 extraRequestCooldownSeconds = 48 * 60 * 60;
};

struct ExtraOpportunityDecision {
    bool allowed = false;
    QDateTime nextEligibleAt;
};

class EvolutionOpportunityPolicy {
public:
    EvolutionOpportunityPolicy(
        EvolutionOpportunityPolicyConfig config = {});

    Result<OpportunityDecision, DomainError> decide(
        const EvolutionOpportunityState& state,
        const OpportunityEstimate& estimate) const;
    ExtraOpportunityDecision decideExtra(
        const ExtraOpportunityState& state) const;

private:
    QDateTime restEligibleAt(const EvolutionOpportunityState& state) const;
    OpportunityDecision deferred(const QDateTime& nextEligibleAt,
                                 const QString& reason) const;

    EvolutionOpportunityPolicyConfig m_config;
};

#endif // DESKTOP_PET_EVOLUTION_OPPORTUNITY_POLICY_H
