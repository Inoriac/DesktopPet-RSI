#include <QtTest>

#include "ai/evolution/evolution_intent_service.h"

namespace {

class FakeEvolutionSupervisorClient final : public EvolutionSupervisorClient {
public:
    Result<IntentReceipt, DomainError> persistIntent(
        const EvolutionIntentDraft& draft) override {
        ++persistCalls;
        persistedDraft = draft;
        IntentReceipt receipt;
        receipt.intentId = QStringLiteral("intent-1");
        receipt.state = EvolutionIntentState::WaitingWindow;
        return Result<IntentReceipt, DomainError>::success(receipt);
    }

    Result<EvolutionOpportunityState, DomainError> opportunityState(
        const IntentId&) override {
        ++opportunityStateCalls;
        return Result<EvolutionOpportunityState, DomainError>::success({});
    }

    Result<OpportunityDecision, DomainError> createCandidate(
        const IntentId&, const OpportunityEstimate&) override {
        ++candidateCalls;
        return Result<OpportunityDecision, DomainError>::success({});
    }

    Result<ExtraOpportunityState, DomainError> extraOpportunityState(
        const IntentId&) override {
        ++extraStateCalls;
        return Result<ExtraOpportunityState, DomainError>::success({});
    }

    Result<HelpOrBudgetRequest, DomainError> createExtraOpportunity(
        const IntentId&, const QString&) override {
        ++extraCreateCalls;
        return Result<HelpOrBudgetRequest, DomainError>::success({});
    }

    int persistCalls = 0;
    int opportunityStateCalls = 0;
    int candidateCalls = 0;
    int extraStateCalls = 0;
    int extraCreateCalls = 0;
    EvolutionIntentDraft persistedDraft;
};

EvolutionIntentDraft conversationDraft() {
    EvolutionIntentDraft draft;
    draft.origin = EvolutionIntentOrigin::Conversation;
    draft.evidenceIds = {QStringLiteral("event-conversation-1")};
    draft.goal = QStringLiteral("make reminder confirmations clearer");
    draft.expectedValue = QStringLiteral("fewer misunderstood reminders");
    draft.estimatedBudget = {1, 800, 300, 1200, 1};
    return draft;
}

} // namespace

class TestEvolutionIntent : public QObject {
    Q_OBJECT

private slots:
    void recordIntent_whenConversationEvidenceExists_shouldPersistWithoutExtraModelCall();
    void recordIntent_whenEvidenceMissing_shouldReject();
};

void TestEvolutionIntent::recordIntent_whenConversationEvidenceExists_shouldPersistWithoutExtraModelCall() {
    FakeEvolutionSupervisorClient supervisor;
    EvolutionIntentService service(&supervisor);

    const auto result = service.recordIntent(conversationDraft());

    QVERIFY(result.isOk());
    QCOMPARE(result.value().intentId, QStringLiteral("intent-1"));
    QCOMPARE(supervisor.persistCalls, 1);
    QCOMPARE(supervisor.persistedDraft.evidenceIds,
             QStringList{QStringLiteral("event-conversation-1")});
    QCOMPARE(supervisor.opportunityStateCalls, 0);
    QCOMPARE(supervisor.candidateCalls, 0);
    QCOMPARE(supervisor.extraStateCalls, 0);
    QCOMPARE(supervisor.extraCreateCalls, 0);
}

void TestEvolutionIntent::recordIntent_whenEvidenceMissing_shouldReject() {
    FakeEvolutionSupervisorClient supervisor;
    EvolutionIntentService service(&supervisor);
    EvolutionIntentDraft draft = conversationDraft();
    draft.evidenceIds.clear();

    const auto result = service.recordIntent(draft);

    QVERIFY(!result.isOk());
    QCOMPARE(result.error().code, QStringLiteral("EVIDENCE_INVALID"));
    QCOMPARE(supervisor.persistCalls, 0);
}

QTEST_MAIN(TestEvolutionIntent)
#include "test_evolution_intent.moc"
