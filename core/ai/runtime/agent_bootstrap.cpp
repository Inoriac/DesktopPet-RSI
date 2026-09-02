#include "agent_bootstrap.h"

#include <QRegularExpression>
#include <QFileInfo>

#include "ai/ai_brain.h"
#include "ai/event/event_types.h"
#include "agent_runtime_services.h"
#include "profile_data_migrator.h"

namespace {

[[maybe_unused]] bool isSha256(const QString& value) {
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-f]{64}$"));
    return pattern.match(value).hasMatch();
}

} // namespace

Result<RuntimeStartReport, DomainError> AgentBootstrap::start(
    AgentRuntimeServices& services,
    const RuntimeStartRequest& request) {
#if !defined(DESKTOP_PET_ENABLE_LEGACY_RUNTIME) || !DESKTOP_PET_ENABLE_LEGACY_RUNTIME
    Q_UNUSED(services)
    Q_UNUSED(request)
    return Result<RuntimeStartReport, DomainError>::failure(
        domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                    QStringLiteral("legacy runtime is disabled in this build")));
#else
    if (services.isStarted() || !request.aiBrain || !request.uiBridge
        || !isCanonicalEventUuid(request.profile.profileId)
        || request.profile.name.trimmed().isEmpty()
        || request.profileMigration.profileId != request.profile.profileId
        || request.profileMigration.appDataRoot.trimmed().isEmpty()
        || request.profileMigration.legacyDatabasePath.trimmed().isEmpty()
        || request.profileMigration.legacyJsonPath.trimmed().isEmpty()
        || request.identityBaselineSchemaVersion <= 0
        || !isSha256(request.configHash)
        || !isSha256(request.identityBaselineHash)) {
        return Result<RuntimeStartReport, DomainError>::failure(
            domainError(QStringLiteral("RUNTIME_START_INVALID"),
                        QStringLiteral("agent bootstrap request is invalid")));
    }

    RuntimeStartRequest sanitizedRequest = request;
    const bool ownerDiaryPathInvalid = !request.ownerDiaryBootstrapPath.isEmpty()
        && !QFileInfo(request.ownerDiaryBootstrapPath).isAbsolute();
    if (ownerDiaryPathInvalid) sanitizedRequest.ownerDiaryBootstrapPath.clear();

    const ProfileMigrationResult migration =
        ProfileDataMigrator().migrateLegacyMemory(sanitizedRequest.profileMigration);
    const Result<void, DomainError> storage = request.aiBrain->initializeStorage({
        migration.activeDatabasePath,
        migration.activeJsonPath
    });
    if (!storage.isOk()) {
        return Result<RuntimeStartReport, DomainError>::failure(storage.error());
    }

    Result<RuntimeStartReport, DomainError> started =
        services.startAfterStorageReady(sanitizedRequest, migration);
    if (started.isOk() && ownerDiaryPathInvalid) {
        started.value().diagnostics.append(QStringLiteral(
            "owner diary bootstrap path must be absolute"));
    }
    if (started.isOk()) request.aiBrain->setRuntimeServices(&services);
    return started;
#endif
}

Result<RuntimeStartReport, DomainError> AgentBootstrap::start(
    AgentRuntimeServices& services,
    const RuntimeStartRequest& request,
    const RuntimeCompositionPolicy& composition,
    const RuntimeBootstrap& bootstrap) {
    Q_UNUSED(bootstrap)
    if (composition.authorityMode == RuntimeAuthorityMode::LegacyDevelopment) {
        return start(services, request);
    }
    if (!composition.supervisorTransport) {
        return Result<RuntimeStartReport, DomainError>::failure(
            domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                        QStringLiteral("supervisor transport is required")));
    }
    return Result<RuntimeStartReport, DomainError>::failure(
        domainError(QStringLiteral("SUPERVISOR_NOT_READY"),
                    QStringLiteral("trusted Windows runtime composition is not connected"),
                    {{QStringLiteral("integrationTask"), 11}}));
}
