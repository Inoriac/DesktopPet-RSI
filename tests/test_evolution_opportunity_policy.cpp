#include <QtTest>

#include "ai/evolution/evolution_intent_service.h"

namespace {

const QDateTime kNow = QDateTime::fromString(
    QStringLiteral("2026-09-02T08:00:00Z"), Qt::ISODate);

class FakeEvolutionSupervisorClient final : public EvolutionSupervisorClient {
public:
    Result<IntentReceipt, DomainError> persistIntent(
        const EvolutionIntentDraft&) override {
        return Result<IntentReceipt, DomainError>::success({});
    }

    Result<EvolutionOpportunityState, DomainError> opportunityState(
        const IntentId&) override {
        return Result<EvolutionOpportunityState, DomainError>::success(opportunity);
    }

    Result<OpportunityDecision, DomainError> createCandidate(
        const IntentId& intentId, const OpportunityEstimate& estimate) override {
        ++candidateCalls;
        committedIntentId = intentId;
        committedEstimate = estimate;
        opportunity.activeCandidate = true;
        OpportunityDecision decision;
        decision.granted = true;
        decision.candidateId = QStringLiteral("candidate-1");
        return Result<OpportunityDecision, DomainError>::success(decision);
    }

    Result<ExtraOpportunityState, DomainError> extraOpportunityState(
        const IntentId&) override {
        return Result<ExtraOpportunityState, DomainError>::success(extra);
    }

    Result<HelpOrBudgetRequest, DomainError> createExtraOpportunity(
        const IntentId& intentId,
        const QString& motivation) override {
        ++extraCreateCalls;
        extraIntentId = intentId;
        extraMotivation = motivation;
        extraExpiresAt = extra.now.addSecs(24 * 60 * 60);
        HelpOrBudgetRequest request;
        request.created = true;
        request.requestId = QStringLiteral("approval-1");
        request.expiresAt = extraExpiresAt;
        return Result<HelpOrBudgetRequest, DomainError>::success(request);
    }

    EvolutionOpportunityState opportunity;
    ExtraOpportunityState extra;
    int candidateCalls = 0;
    int extraCreateCalls = 0;
    IntentId committedIntentId;
    OpportunityEstimate committedEstimate;
    IntentId extraIntentId;
    QString extraMotivation;
    QDateTime extraExpiresAt;
};

OpportunityEstimate estimate() {
    return {1, 800, 300, 1200, 1};
}

EvolutionOpportunityState eligibleState() {
    EvolutionOpportunityState state;
    state.now = kNow;
    state.restStartedAt = kNow.addDays(-2);
    state.idleStartedAt = kNow.addSecs(-25 * 60);
    state.userIdleSeconds = 25 * 60;
    state.idleStateKnown = true;
    state.systemResourcesAvailable = true;
    state.evolutionBudget = {2, 2000, 1000, 5000};
    state.dialogueBudget = {20, 20000, 10000, 50000};
    return state;
}

} // namespace

class TestEvolutionOpportunityPolicy : public QObject {
    Q_OBJECT

private slots:
    void requestOpportunity_whenIdleRestedAndBudgeted_shouldCreateSingleCandidate();
    void requestOpportunity_whenRestWindowOpen_shouldReturnNextEligibleAt();
    void requestOpportunity_whenEvolutionBudgetEmpty_shouldNotBorrowDialogueBudget();
    void requestExtraOpportunity_whenCooldownElapsed_shouldCreateOneOwnerRequest();
    void requestExtraOpportunity_whenCooldownActive_shouldAvoidRepeatedPrompt();
};

void TestEvolutionOpportunityPolicy::requestOpportunity_whenIdleRestedAndBudgeted_shouldCreateSingleCandidate() {
    FakeEvolutionSupervisorClient supervisor;
    supervisor.opportunity = eligibleState();
    EvolutionIntentService service(&supervisor);

    const auto result = service.requestOpportunity(QStringLiteral("intent-1"), estimate());
    const auto duplicate = service.requestOpportunity(
        QStringLiteral("intent-1"), estimate());

    QVERIFY(result.isOk());
    QVERIFY(result.value().granted);
    QVERIFY(duplicate.isOk());
    QVERIFY(!duplicate.value().granted);
    QCOMPARE(result.value().candidateId, QStringLiteral("candidate-1"));
    QCOMPARE(supervisor.candidateCalls, 1);
    QCOMPARE(supervisor.committedIntentId, QStringLiteral("intent-1"));
}

void TestEvolutionOpportunityPolicy::requestOpportunity_whenRestWindowOpen_shouldReturnNextEligibleAt() {
    FakeEvolutionSupervisorClient supervisor;
    supervisor.opportunity = eligibleState();
    supervisor.opportunity.restStartedAt = kNow.addSecs(-23 * 60 * 60);
    EvolutionIntentService service(&supervisor);

    const auto result = service.requestOpportunity(QStringLiteral("intent-1"), estimate());

    QVERIFY(result.isOk());
    QVERIFY(!result.value().granted);
    QCOMPARE(result.value().nextEligibleAt, kNow.addSecs(60 * 60));
    QCOMPARE(supervisor.candidateCalls, 0);
}

void TestEvolutionOpportunityPolicy::requestOpportunity_whenEvolutionBudgetEmpty_shouldNotBorrowDialogueBudget() {
    FakeEvolutionSupervisorClient supervisor;
    supervisor.opportunity = eligibleState();
    supervisor.opportunity.evolutionBudget = {};
    EvolutionIntentService service(&supervisor);

    const auto result = service.requestOpportunity(QStringLiteral("intent-1"), estimate());

    QVERIFY(!result.isOk());
    QCOMPARE(result.error().code, QStringLiteral("BUDGET_EXHAUSTED"));
    QCOMPARE(supervisor.candidateCalls, 0);
    QVERIFY(supervisor.opportunity.dialogueBudget.calls > 0);
}

void TestEvolutionOpportunityPolicy::requestExtraOpportunity_whenCooldownElapsed_shouldCreateOneOwnerRequest() {
    FakeEvolutionSupervisorClient supervisor;
    supervisor.extra.now = kNow;
    supervisor.extra.lastRequestedAt = kNow.addSecs(-49 * 60 * 60);
    EvolutionIntentService service(&supervisor);

    const auto result = service.requestExtraOpportunity(
        QStringLiteral("intent-1"), QStringLiteral("This improvement matters now"));

    QVERIFY(result.isOk());
    QVERIFY(result.value().created);
    QCOMPARE(supervisor.extraCreateCalls, 1);
    QCOMPARE(supervisor.extraIntentId, QStringLiteral("intent-1"));
    QCOMPARE(supervisor.extraExpiresAt, kNow.addSecs(24 * 60 * 60));
}

void TestEvolutionOpportunityPolicy::requestExtraOpportunity_whenCooldownActive_shouldAvoidRepeatedPrompt() {
    FakeEvolutionSupervisorClient supervisor;
    supervisor.extra.now = kNow;
    supervisor.extra.lastRequestedAt = kNow.addSecs(-2 * 60 * 60);
    EvolutionIntentService service(&supervisor);

    const auto first = service.requestExtraOpportunity(
        QStringLiteral("intent-1"), QStringLiteral("Please let me try"));
    const auto second = service.requestExtraOpportunity(
        QStringLiteral("intent-1"), QStringLiteral("Please let me try again"));

    QVERIFY(first.isOk());
    QVERIFY(second.isOk());
    QVERIFY(!first.value().created);
    QCOMPARE(first.value().nextEligibleAt, kNow.addSecs(46 * 60 * 60));
    QCOMPARE(second.value().nextEligibleAt, first.value().nextEligibleAt);
    QCOMPARE(supervisor.extraCreateCalls, 0);

    supervisor.extra.lastRequestedAt = kNow.addSecs(-49 * 60 * 60);
    supervisor.extra.activeRequestExpiresAt = kNow.addSecs(12 * 60 * 60);
    const auto activeRequest = service.requestExtraOpportunity(
        QStringLiteral("intent-1"), QStringLiteral("Do not duplicate this request"));
    QVERIFY(activeRequest.isOk());
    QVERIFY(!activeRequest.value().created);
    QCOMPARE(activeRequest.value().nextEligibleAt,
             supervisor.extra.activeRequestExpiresAt);
    QCOMPARE(supervisor.extraCreateCalls, 0);
}

QTEST_MAIN(TestEvolutionOpportunityPolicy)
#include "test_evolution_opportunity_policy.moc"
