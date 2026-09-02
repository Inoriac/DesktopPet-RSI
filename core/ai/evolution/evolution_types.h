#ifndef DESKTOP_PET_EVOLUTION_TYPES_H
#define DESKTOP_PET_EVOLUTION_TYPES_H

#include <QDateTime>
#include <QString>
#include <QStringList>

#include "ai/domain/domain_result.h"

using IntentId = QString;

enum class EvolutionIntentOrigin {
    Conversation,
    Reflection,
    Feedback,
    PersonalityInterest,
    EnvironmentObservation,
    LeisureCreation
};

enum class EvolutionIntentState {
    Recorded,
    WaitingWindow,
    CandidateCreated
};

struct OpportunityEstimate {
    int calls = 0;
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
    qint64 costMicros = 0;
    int changeScale = 0;
};

struct EvolutionIntentDraft {
    EvolutionIntentOrigin origin = EvolutionIntentOrigin::Conversation;
    QStringList evidenceIds;
    QString goal;
    QString expectedValue;
    OpportunityEstimate estimatedBudget;
};

struct IntentReceipt {
    IntentId intentId;
    EvolutionIntentState state = EvolutionIntentState::Recorded;
};

struct BudgetAvailability {
    int calls = 0;
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
    qint64 costMicros = 0;

    bool covers(const OpportunityEstimate& estimate) const;
};

struct EvolutionOpportunityState {
    QDateTime now;
    QDateTime restStartedAt;
    QDateTime idleStartedAt;
    int failureCount = 0;
    int userIdleSeconds = -1;
    bool idleStateKnown = false;
    bool userInteractionActive = false;
    bool dialogueCallActive = false;
    bool activeCandidate = false;
    bool systemResourcesAvailable = false;
    BudgetAvailability evolutionBudget;
    BudgetAvailability dialogueBudget;
};

struct OpportunityDecision {
    bool granted = false;
    QString candidateId;
    QDateTime nextEligibleAt;
    QString reason;
};

struct ExtraOpportunityState {
    QDateTime now;
    QDateTime lastRequestedAt;
    QDateTime activeRequestExpiresAt;
};

struct HelpOrBudgetRequest {
    bool created = false;
    QString requestId;
    QDateTime expiresAt;
    QDateTime nextEligibleAt;
};

QString evolutionIntentOriginName(EvolutionIntentOrigin origin);

#endif // DESKTOP_PET_EVOLUTION_TYPES_H
