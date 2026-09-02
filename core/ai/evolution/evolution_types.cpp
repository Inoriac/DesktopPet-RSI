#include "evolution_types.h"

bool BudgetAvailability::covers(const OpportunityEstimate& estimate) const {
    return estimate.calls > 0
        && estimate.inputTokens >= 0
        && estimate.outputTokens >= 0
        && estimate.costMicros >= 0
        && calls >= estimate.calls
        && inputTokens >= estimate.inputTokens
        && outputTokens >= estimate.outputTokens
        && costMicros >= estimate.costMicros;
}

QString evolutionIntentOriginName(EvolutionIntentOrigin origin) {
    switch (origin) {
    case EvolutionIntentOrigin::Conversation:
        return QStringLiteral("conversation");
    case EvolutionIntentOrigin::Reflection:
        return QStringLiteral("reflection");
    case EvolutionIntentOrigin::Feedback:
        return QStringLiteral("feedback");
    case EvolutionIntentOrigin::PersonalityInterest:
        return QStringLiteral("personality_interest");
    case EvolutionIntentOrigin::EnvironmentObservation:
        return QStringLiteral("environment_observation");
    case EvolutionIntentOrigin::LeisureCreation:
        return QStringLiteral("leisure_creation");
    }
    return {};
}
