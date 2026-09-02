#include <QtTest>

#include <QQueue>

#include "ai/supervisor/supervisor_runtime_adapter.h"

namespace {

const QString kProfileId = QStringLiteral("11111111-1111-4111-8111-111111111111");
const QString kCandidateId = QStringLiteral("22222222-2222-4222-8222-222222222222");

class FakeSupervisorTransport final : public SupervisorRuntimeTransport {
public:
    Result<SupervisorTransportResponse, DomainError> exchange(
        const SupervisorTransportRequest& request) override {
        requests.append(request);
        if (!failureCode.isEmpty()) {
            return Result<SupervisorTransportResponse, DomainError>::failure(
                domainError(failureCode, QStringLiteral("transport unavailable")));
        }
        if (responses.isEmpty()) {
            return Result<SupervisorTransportResponse, DomainError>::failure(
                domainError(QStringLiteral("TEST_RESPONSE_MISSING"),
                            QStringLiteral("fake response queue is empty")));
        }
        return Result<SupervisorTransportResponse, DomainError>::success(
            responses.dequeue());
    }

    void close() override { ++closeCalls; }

    void enqueue(QJsonObject payload) {
        SupervisorTransportResponse response;
        response.payload = std::move(payload);
        responses.enqueue(std::move(response));
    }

    QString failureCode;
    int closeCalls = 0;
    QList<SupervisorTransportRequest> requests;
    QQueue<SupervisorTransportResponse> responses;
};

RuntimeBootstrap bootstrap() {
    RuntimeBootstrap value;
    value.pipeName = QStringLiteral("runtime-test");
    value.bootstrapToken = QStringLiteral("one-time-secret");
    value.profileId = kProfileId;
    value.candidateId = kCandidateId;
    value.protocolVersion = 1;
    return value;
}

QJsonObject helloPayload() {
    return {
        {QStringLiteral("sessionToken"), QStringLiteral("connection-secret")},
        {QStringLiteral("runtimeSessionId"), QStringLiteral("session-trusted")},
        {QStringLiteral("profileId"), kProfileId},
        {QStringLiteral("candidateId"), kCandidateId},
        {QStringLiteral("producerCommit"), QString(40, QLatin1Char('a'))},
        {QStringLiteral("protocolVersion"), 1},
        {QStringLiteral("leaseMap"), QJsonObject{
            {QStringLiteral("network.http"), QStringLiteral("lease-network")}
        }}
    };
}

void initializeAdapter(FakeSupervisorTransport& transport,
                       SupervisorRuntimeAdapter& adapter) {
    transport.enqueue(helloPayload());
    const auto result = adapter.initialize(bootstrap());
    Q_ASSERT(result.isOk());
}

} // namespace

class TestSupervisorRuntimeAdapter : public QObject {
    Q_OBJECT

private slots:
    void initialize_whenHelloSucceeds_shouldBindTrustedRuntimeContext();
    void initialize_whenTransportUnavailable_shouldKeepAuthoritativeServicesClosed();
    void initialize_whenHelloMalformed_shouldCloseTransportAndRemainNotReady();
    void appendEvent_whenReceiptValid_shouldUpdateCacheAfterAcknowledgement();
    void queryState_whenUnknownSchemaReturned_shouldPreserveOpaqueDocument();
    void completeModel_whenRoleProvided_shouldForwardWithoutApiCredential();
    void invokeCapability_whenLeaseMapped_shouldReturnSanitizedPayload();
};

void TestSupervisorRuntimeAdapter::initialize_whenHelloSucceeds_shouldBindTrustedRuntimeContext() {
    FakeSupervisorTransport transport;
    transport.enqueue(helloPayload());
    SupervisorRuntimeAdapter adapter(&transport);

    const auto result = adapter.initialize(bootstrap());

    QVERIFY(result.isOk());
    QVERIFY(adapter.isReady());
    QCOMPARE(result.value().sessionId, QStringLiteral("session-trusted"));
    QCOMPARE(result.value().profileId, kProfileId);
    QCOMPARE(result.value().candidateId, kCandidateId);
    QCOMPARE(result.value().producerCommit, QString(40, QLatin1Char('a')));
    QCOMPARE(transport.requests.size(), 1);
    QCOMPARE(transport.requests.first().action, QStringLiteral("hello"));
    QCOMPARE(transport.closeCalls, 0);
}

void TestSupervisorRuntimeAdapter::initialize_whenTransportUnavailable_shouldKeepAuthoritativeServicesClosed() {
    FakeSupervisorTransport transport;
    transport.failureCode = QStringLiteral("TRANSPORT_UNAVAILABLE");
    SupervisorRuntimeAdapter adapter(&transport);

    const auto initialized = adapter.initialize(bootstrap());
    const auto append = adapter.appendEvent(EventDraft{}, {});

    QVERIFY(!initialized.isOk());
    QCOMPARE(initialized.error().code, QStringLiteral("SUPERVISOR_NOT_READY"));
    QVERIFY(!adapter.isReady());
    QVERIFY(!append.isOk());
    QCOMPARE(append.error().code, QStringLiteral("SUPERVISOR_NOT_READY"));
}

void TestSupervisorRuntimeAdapter::initialize_whenHelloMalformed_shouldCloseTransportAndRemainNotReady() {
    FakeSupervisorTransport transport;
    QJsonObject malformed = helloPayload();
    malformed.remove(QStringLiteral("sessionToken"));
    transport.enqueue(malformed);
    SupervisorRuntimeAdapter adapter(&transport);

    const auto result = adapter.initialize(bootstrap());

    QVERIFY(!result.isOk());
    QCOMPARE(result.error().code, QStringLiteral("PROTOCOL_INVALID"));
    QVERIFY(!adapter.isReady());
    QCOMPARE(transport.closeCalls, 1);
}

void TestSupervisorRuntimeAdapter::appendEvent_whenReceiptValid_shouldUpdateCacheAfterAcknowledgement() {
    FakeSupervisorTransport transport;
    SupervisorRuntimeAdapter adapter(&transport);
    initializeAdapter(transport, adapter);
    transport.enqueue({
        {QStringLiteral("eventId"), QStringLiteral("33333333-3333-4333-8333-333333333333")},
        {QStringLiteral("sequence"), 42},
        {QStringLiteral("createdAt"), QStringLiteral("2026-09-02T03:00:00.000Z")},
        {QStringLiteral("stateRevisions"), QJsonObject{
            {QStringLiteral("personality/current"), 7}
        }}
    });
    EventDraft draft;
    draft.type = QStringLiteral("PersonalityChanged");
    draft.schemaVersion = 1;
    draft.payload = {{QStringLiteral("reason"), QStringLiteral("reflection")}};
    StateMutation mutation;
    mutation.domain = QStringLiteral("personality");
    mutation.documentId = QStringLiteral("current");
    mutation.schemaVersion = 1;
    mutation.expectedRevision = 6;
    mutation.operation = StateMutationOperation::Upsert;
    mutation.body = {{QStringLiteral("version"), 7}};

    const auto result = adapter.appendEvent(draft, {mutation});

    QVERIFY(result.isOk());
    QCOMPARE(result.value().event.sequence, 42);
    QCOMPARE(adapter.lastAcknowledgedSequence(), 42);
    QCOMPARE(adapter.cachedRevision(QStringLiteral("personality"),
                                    QStringLiteral("current")), 7);
    QCOMPARE(transport.requests.last().payload.value(QStringLiteral("stateMutations"))
                 .toArray().first().toObject().value(QStringLiteral("expectedRevision")).toInt(), 6);
}

void TestSupervisorRuntimeAdapter::queryState_whenUnknownSchemaReturned_shouldPreserveOpaqueDocument() {
    FakeSupervisorTransport transport;
    SupervisorRuntimeAdapter adapter(&transport);
    initializeAdapter(transport, adapter);
    const QJsonObject opaqueBody{
        {QStringLiteral("futureTrait"), QJsonObject{{QStringLiteral("weight"), 0.8}}}
    };
    transport.enqueue({
        {QStringLiteral("documents"), QJsonArray{QJsonObject{
            {QStringLiteral("domain"), QStringLiteral("personality")},
            {QStringLiteral("documentId"), QStringLiteral("current")},
            {QStringLiteral("schemaVersion"), 99},
            {QStringLiteral("revision"), 8},
            {QStringLiteral("updatedSequence"), 43},
            {QStringLiteral("understood"), false},
            {QStringLiteral("body"), opaqueBody}
        }}},
        {QStringLiteral("nextCursor"), QStringLiteral("cursor-2")}
    });
    StateQuery query;
    query.domain = QStringLiteral("personality");
    query.limit = 10;

    const auto result = adapter.queryState(query);

    QVERIFY(result.isOk());
    QCOMPARE(result.value().documents.size(), 1);
    QVERIFY(!result.value().documents.first().understood);
    QCOMPARE(result.value().documents.first().body, opaqueBody);
    QCOMPARE(adapter.lastObservedSequence(), 43);
}

void TestSupervisorRuntimeAdapter::completeModel_whenRoleProvided_shouldForwardWithoutApiCredential() {
    FakeSupervisorTransport transport;
    SupervisorRuntimeAdapter adapter(&transport);
    initializeAdapter(transport, adapter);
    transport.enqueue({
        {QStringLiteral("completionId"), QStringLiteral("completion-1")},
        {QStringLiteral("model"), QStringLiteral("supervised-model")},
        {QStringLiteral("content"), QStringLiteral("reflection result")},
        {QStringLiteral("finishReason"), QStringLiteral("stop")},
        {QStringLiteral("usage"), QJsonObject{
            {QStringLiteral("inputTokens"), 10},
            {QStringLiteral("outputTokens"), 3}
        }}
    });
    ModelRouteRequest request;
    request.role = ModelRole::Consolidation;
    ChatMessage message;
    message.role = QStringLiteral("user");
    message.content = QStringLiteral("reflect");
    request.messages = {message};
    request.maxOutputTokens = 128;
    request.modelPolicyId = QStringLiteral("default");

    const auto result = adapter.completeModel(request);

    QVERIFY(result.isOk());
    QCOMPARE(result.value().response.content, QStringLiteral("reflection result"));
    const QJsonObject payload = transport.requests.last().payload;
    QCOMPARE(payload.value(QStringLiteral("role")).toString(),
             QStringLiteral("reflection"));
    QVERIFY(!payload.contains(QStringLiteral("apiKey")));
    QVERIFY(!payload.contains(QStringLiteral("credential")));
    QCOMPARE(payload.value(QStringLiteral("profileId")).toString(), kProfileId);
    QCOMPARE(payload.value(QStringLiteral("candidateId")).toString(), kCandidateId);
}

void TestSupervisorRuntimeAdapter::invokeCapability_whenLeaseMapped_shouldReturnSanitizedPayload() {
    FakeSupervisorTransport transport;
    SupervisorRuntimeAdapter adapter(&transport);
    initializeAdapter(transport, adapter);
    transport.enqueue({
        {QStringLiteral("resultDigest"), QString(64, QLatin1Char('b'))},
        {QStringLiteral("observedUsage"), QJsonObject{{QStringLiteral("bytes"), 12}}},
        {QStringLiteral("sanitizedData"), QJsonObject{{QStringLiteral("status"), 200}}}
    });
    CapabilityInvocation invocation;
    invocation.type = QStringLiteral("network.http");
    invocation.operation = QStringLiteral("get");
    invocation.scopeRef = QStringLiteral("docs-api");
    invocation.idempotencyKey = QStringLiteral("request-1");

    const auto result = adapter.invokeCapability(invocation);

    QVERIFY(result.isOk());
    QCOMPARE(result.value().sanitizedData.value(QStringLiteral("status")).toInt(), 200);
    QCOMPARE(transport.requests.last().payload.value(QStringLiteral("leaseId")).toString(),
             QStringLiteral("lease-network"));
}

QTEST_GUILESS_MAIN(TestSupervisorRuntimeAdapter)
#include "test_supervisor_runtime_adapter.moc"
