#include "evolution_opportunity_policy.h"

#include <QtGlobal>

#include <utility>

EvolutionOpportunityPolicy::EvolutionOpportunityPolicy(
    EvolutionOpportunityPolicyConfig config)
    : m_config(std::move(config)) {
    m_config.minimumRestSeconds = qMax<qint64>(1, m_config.minimumRestSeconds);
    m_config.maximumRestSeconds = qMax(
        m_config.minimumRestSeconds, m_config.maximumRestSeconds);
    m_config.minimumIdleSeconds = qMax(1, m_config.minimumIdleSeconds);
    m_config.naturalWindowSeconds = qMax<qint64>(1, m_config.naturalWindowSeconds);
    m_config.extraRequestCooldownSeconds = qMax<qint64>(1,
        m_config.extraRequestCooldownSeconds);
}

Result<OpportunityDecision, DomainError> EvolutionOpportunityPolicy::decide(
    const EvolutionOpportunityState& state,
    const OpportunityEstimate& estimate) const {
    const QDateTime now = state.now;
    if (!now.isValid()) {
        return Result<OpportunityDecision, DomainError>::failure(
            domainError(QStringLiteral("PROTOCOL_INVALID"),
                        QStringLiteral("trusted scheduling time is missing")));
    }

    if (state.activeCandidate) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred({}, QStringLiteral("active_candidate")));
    }
    if (state.userInteractionActive || state.dialogueCallActive) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred({}, QStringLiteral("conversation_active")));
    }
    if (!state.systemResourcesAvailable || !state.idleStateKnown) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred({}, QStringLiteral("natural_window_unavailable")));
    }

    const QDateTime restAt = restEligibleAt(state);
    if (!restAt.isValid() || now < restAt) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred(restAt, QStringLiteral("resting")));
    }

    const QDateTime idleAt = state.idleStartedAt.isValid()
        ? state.idleStartedAt.addSecs(m_config.minimumIdleSeconds)
        : QDateTime{};
    if (state.userIdleSeconds < m_config.minimumIdleSeconds
        || !idleAt.isValid() || now < idleAt) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred(idleAt, QStringLiteral("waiting_for_natural_window")));
    }
    if (now >= idleAt.addSecs(m_config.naturalWindowSeconds)) {
        return Result<OpportunityDecision, DomainError>::success(
            deferred({}, QStringLiteral("natural_window_missed")));
    }

    if (!state.evolutionBudget.covers(estimate)) {
        return Result<OpportunityDecision, DomainError>::failure(
            domainError(QStringLiteral("BUDGET_EXHAUSTED"),
                        QStringLiteral("evolution budget is insufficient")));
    }

    OpportunityDecision decision;
    decision.granted = true;
    decision.reason = QStringLiteral("eligible");
    return Result<OpportunityDecision, DomainError>::success(decision);
}

ExtraOpportunityDecision EvolutionOpportunityPolicy::decideExtra(
    const ExtraOpportunityState& state) const {
    ExtraOpportunityDecision decision;
    if (!state.now.isValid()) return decision;

    if (state.activeRequestExpiresAt.isValid()
        && state.now < state.activeRequestExpiresAt) {
        decision.nextEligibleAt = state.activeRequestExpiresAt;
        return decision;
    }

    const QDateTime cooldownAt = state.lastRequestedAt.isValid()
        ? state.lastRequestedAt.addSecs(m_config.extraRequestCooldownSeconds)
        : QDateTime{};
    if (cooldownAt.isValid() && state.now < cooldownAt) {
        decision.nextEligibleAt = cooldownAt;
        return decision;
    }

    decision.allowed = true;
    return decision;
}

QDateTime EvolutionOpportunityPolicy::restEligibleAt(
    const EvolutionOpportunityState& state) const {
    if (!state.restStartedAt.isValid()) return {};
    const int exponent = qBound(0, state.failureCount, 30);
    qint64 restSeconds = m_config.minimumRestSeconds;
    for (int i = 0; i < exponent && restSeconds < m_config.maximumRestSeconds; ++i) {
        restSeconds = qMin(m_config.maximumRestSeconds, restSeconds * 2);
    }
    return state.restStartedAt.addSecs(restSeconds);
}

OpportunityDecision EvolutionOpportunityPolicy::deferred(
    const QDateTime& nextEligibleAt,
    const QString& reason) const {
    OpportunityDecision decision;
    decision.nextEligibleAt = nextEligibleAt;
    decision.reason = reason;
    return decision;
}
