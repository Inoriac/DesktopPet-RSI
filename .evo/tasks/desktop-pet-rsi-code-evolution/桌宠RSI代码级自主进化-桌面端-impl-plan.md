# 桌宠 RSI 代码级自主进化实现计划

**系分文档**: `.evo/tasks/desktop-pet-rsi-code-evolution/桌宠RSI代码级自主进化-桌面端系分.md`

## 实施边界

- `PET`：当前仓库 `/Users/huang/Desktop/DesktopPet-RSI`。
- `TCB`：新建同级独立仓库 `/Users/huang/Desktop/DesktopPet-RSI-Supervisor`，不得放入 PET 仓库、submodule 或 Builder 输入。
- 每个 Task 串行实施并单独提交；若同时修改两个仓库，则分别提交并在报告中记录对应 commit。
- macOS 只运行相关 Qt/Python 单测和纯平台无关逻辑；不运行 ONNX、Windows Service、Token/AppContainer、Job Object、ACL、Named Pipe peer token 或 Authenticode 集成验证。
- Windows-only 代码必须使用 `_WIN32` 条件编译与明确的 fail-closed 非 Windows实现，不以普通 `QProcess` 或弱权限方案替代。
- TCB 首次引入测试，沿用 PET 的根级 `tests/test_*.cpp` + QtTest 约定。

---

## Wave 1

## Task 1: Windows Service 与进程沙箱

- **关联设计**: §3.2 [TCB] Windows Service 与进程沙箱
- **状态**: [ ]
- **Wave**: 1
- **依赖**: 无

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/CMakeLists.txt`
- 实现: `DesktopPet-RSI-Supervisor/src/domain/supervisor_result.h`
- 实现: `DesktopPet-RSI-Supervisor/src/sandbox/sandbox_types.h`
- 实现: `DesktopPet-RSI-Supervisor/src/sandbox/process_sandbox_service.h`
- 实现: `DesktopPet-RSI-Supervisor/src/sandbox/process_sandbox_service.cpp`
- 实现: `DesktopPet-RSI-Supervisor/src/sandbox/windows_process_sandbox.cpp`
- 实现: `DesktopPet-RSI-Supervisor/src/session_host/main.cpp`
- 实现: `DesktopPet-RSI-Supervisor/src/service/main.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_process_sandbox.cpp`

**测试用例**:

ProcessSandboxService.launch:
- `launch_whenSealedObjectAndPolicyValid_shouldCreateBoundSandbox`（happy path）
- `launch_whenObjectIsUnsealed_shouldFailClosed`
- `launch_whenCallerSuppliesRawPath_shouldRejectRequest`

ProcessSandboxService.terminate:
- `terminate_whenSandboxRunning_shouldTerminateWholeInstance`（happy path）

ProcessSandboxService.inspect:
- `inspect_whenSandboxExists_shouldReturnTrustedSnapshot`（happy path）
- `inspect_whenSandboxMissing_shouldReturnNotFound`

## Wave 2

## Task 2: 可信 IPC 与会话认证

- **关联设计**: §3.3 [TCB] 可信 IPC 与会话认证
- **状态**: [ ]
- **Wave**: 2
- **依赖**: Task 1

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/ipc/protocol_v1.h`
- 实现: `DesktopPet-RSI-Supervisor/src/ipc/frame_codec.*`
- 实现: `DesktopPet-RSI-Supervisor/src/ipc/trusted_ipc_gateway.*`
- 实现: `DesktopPet-RSI-Supervisor/src/ipc/windows_peer_identity.*`
- 实现: `DesktopPet-RSI-Supervisor/sdk/include/desktop_pet_supervisor/protocol_v1.h`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_frame_codec.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_trusted_ipc_gateway.cpp`

**测试用例**:

TrustedIpcGateway.authenticate:
- `authenticate_whenPeerAndBootstrapMatch_shouldIssueConnectionSession`（happy path）
- `authenticate_whenBootstrapIsReplayed_shouldReject`
- `authenticate_whenPeerRoleMismatches_shouldReject`

TrustedIpcGateway.dispatch:
- `dispatch_whenRequestIsValid_shouldCallAllowedHandler`（happy path）
- `dispatch_whenRequestIdRepeats_shouldReturnSameReceipt`
- `dispatch_whenActionIsNotAllowedForPipe_shouldReject`
- `dispatch_whenFrameOrJsonExceedsLimits_shouldRejectBeforeHandler`

TrustedIpcGateway.close:
- `close_whenSessionExists_shouldInvalidateConnectionToken`（happy path）

## Wave 3

## Task 3: LifeLedger 与终身状态投影

- **关联设计**: §3.4 [TCB] LifeLedger 与终身状态投影
- **状态**: [ ]
- **Wave**: 3
- **依赖**: Task 2

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/storage/sqlite_database.*`
- 实现: `DesktopPet-RSI-Supervisor/src/storage/migrations/001_initial.sql`
- 实现: `DesktopPet-RSI-Supervisor/src/ledger/life_ledger_service.*`
- 实现: `DesktopPet-RSI-Supervisor/src/ledger/life_event_types.h`
- 实现: `DesktopPet-RSI-Supervisor/src/ledger/state_document_store.*`
- 实现: `DesktopPet-RSI-Supervisor/src/ledger/legacy_importer.*`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_life_ledger.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_legacy_importer.cpp`

**测试用例**:

LifeLedgerService.append:
- `append_whenEventAndMutationValid_shouldAdvanceHashAndStateAtomically`（happy path）
- `append_whenExpectedRevisionConflicts_shouldWriteNothing`
- `append_whenEvidenceCrossesProfile_shouldReject`
- `append_whenExistingEventIsUpdatedOrDeleted_shouldBeBlockedByDatabase`

LifeLedgerService.queryState:
- `queryState_whenDocumentsVisible_shouldReturnCurrentRevisions`（happy path）
- `queryState_whenSchemaUnknown_shouldPreserveAndMarkUnunderstood`

LifeLedgerService.importLegacy:
- `importLegacy_whenPlanMatchesSourceDigest_shouldCreateEventsAndDocuments`（happy path）
- `importLegacy_whenRepeated_shouldRemainIdempotent`

## Wave 4

## Task 4: Capability、秘密与 Model Gateway

- **关联设计**: §3.5 [TCB] Capability、秘密与 Model Gateway
- **状态**: [ ]
- **Wave**: 4
- **依赖**: Task 2, Task 3

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/broker/capability_broker.*`
- 实现: `DesktopPet-RSI-Supervisor/src/broker/capability_types.h`
- 实现: `DesktopPet-RSI-Supervisor/src/broker/model_gateway.*`
- 实现: `DesktopPet-RSI-Supervisor/src/secrets/secret_store.*`
- 实现: `DesktopPet-RSI-Supervisor/src/secrets/windows_secret_store.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_capability_broker.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_model_gateway.cpp`

**测试用例**:

CapabilityBroker.invoke:
- `invoke_whenLeaseAndScopeMatch_shouldReturnSanitizedResult`（happy path）
- `invoke_whenScopeRedirectsOutsideAllowlist_shouldReject`

CapabilityBroker.issueLease:
- `issueLease_whenScopeDoesNotExpand_shouldBindCandidateAndExpiry`（happy path）
- `issueLease_whenCapabilityTypeUnknown_shouldRequireTrustedUpgrade`

CapabilityBroker.revokeLease:
- `revokeLease_whenActive_shouldDenySubsequentCalls`（happy path）

ModelGateway.complete:
- `complete_whenBudgetAvailable_shouldReserveAndSettleProviderUsage`（happy path）
- `complete_whenEvolutionBudgetEmpty_shouldNotConsumeDialogueBudget`
- `complete_whenProviderFails_shouldNotExposeCredential`

## Wave 5

## Task 5: Supervisor Runtime Adapter

- **关联设计**: §3.6 [PET] Supervisor Runtime Adapter
- **状态**: [ ]
- **Wave**: 5
- **依赖**: Task 2, Task 3, Task 4

**文件**:
- 实现: `core/ai/supervisor/supervisor_protocol_types.*`
- 实现: `core/ai/supervisor/supervisor_runtime_adapter.*`
- 实现: `core/ai/supervisor/supervisor_event_ledger.*`
- 实现: `core/ai/supervisor/supervisor_model_completion_client.*`
- 实现: `core/ai/supervisor/supervisor_capability_client.*`
- 修改: `core/ai/runtime/agent_runtime_services.*`
- 修改: `core/ai/runtime/agent_bootstrap.*`
- 修改: `CMakeLists.txt`
- 测试: `tests/test_supervisor_runtime_adapter.cpp`
- 测试: `tests/test_agent_runtime_services.cpp`

**测试用例**:

SupervisorRuntimeAdapter.initialize:
- `initialize_whenHelloSucceeds_shouldBindTrustedRuntimeContext`（happy path）
- `initialize_whenTransportUnavailable_shouldKeepAuthoritativeServicesClosed`

SupervisorRuntimeAdapter.appendEvent:
- `appendEvent_whenReceiptValid_shouldUpdateCacheAfterAcknowledgement`（happy path）

SupervisorRuntimeAdapter.queryState:
- `queryState_whenUnknownSchemaReturned_shouldPreserveOpaqueDocument`（happy path）

SupervisorRuntimeAdapter.completeModel:
- `completeModel_whenRoleProvided_shouldForwardWithoutApiCredential`（happy path）

SupervisorRuntimeAdapter.invokeCapability:
- `invokeCapability_whenLeaseMapped_shouldReturnSanitizedPayload`（happy path）

## Wave 6

## Task 6: 进化意图与自然空窗调度

- **关联设计**: §3.7 [PET] 进化意图与自然空窗调度
- **状态**: [ ]
- **Wave**: 6
- **依赖**: Task 3, Task 4, Task 5

**文件**:
- 实现: `core/ai/evolution/evolution_types.*`
- 实现: `core/ai/evolution/evolution_intent_service.*`
- 实现: `core/ai/evolution/evolution_opportunity_policy.*`
- 修改: `core/ai/scheduler/agent_scheduler.*`
- 修改: `core/ai/reflection/daydream_sleep_adapter.*`
- 修改: `CMakeLists.txt`
- 测试: `tests/test_evolution_intent.cpp`
- 测试: `tests/test_evolution_opportunity_policy.cpp`

**测试用例**:

EvolutionIntentService.recordIntent:
- `recordIntent_whenConversationEvidenceExists_shouldPersistWithoutExtraModelCall`（happy path）
- `recordIntent_whenEvidenceMissing_shouldReject`

EvolutionIntentService.requestOpportunity:
- `requestOpportunity_whenIdleRestedAndBudgeted_shouldCreateSingleCandidate`（happy path）
- `requestOpportunity_whenRestWindowOpen_shouldReturnNextEligibleAt`
- `requestOpportunity_whenEvolutionBudgetEmpty_shouldNotBorrowDialogueBudget`

EvolutionIntentService.requestExtraOpportunity:
- `requestExtraOpportunity_whenCooldownElapsed_shouldCreateOneOwnerRequest`（happy path）
- `requestExtraOpportunity_whenCooldownActive_shouldAvoidRepeatedPrompt`

## Wave 7

## Task 7: Git、Builder 与 sealed artifact

- **关联设计**: §3.8 [TCB] Git、Builder 与 sealed artifact
- **状态**: [ ]
- **Wave**: 7
- **依赖**: Task 1, Task 2, Task 3, Task 4, Task 6

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/git/candidate_build_service.*`
- 实现: `DesktopPet-RSI-Supervisor/src/git/workspace_scanner.*`
- 实现: `DesktopPet-RSI-Supervisor/src/git/git_broker.*`
- 实现: `DesktopPet-RSI-Supervisor/src/artifact/artifact_sealer.*`
- 实现: `DesktopPet-RSI-Supervisor/src/build/trusted_build_orchestrator.*`
- 修改: `evolution-manifest.json`
- 新增: `state-schemas/v1/*.json`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_workspace_scanner.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_candidate_build_service.cpp`

**测试用例**:

CandidateBuildService.createWorkspace:
- `createWorkspace_whenStableCommitConfirmed_shouldExportWithoutGitOrSecrets`（happy path）

CandidateBuildService.commitAndPush:
- `commitAndPush_whenWorkspaceClean_shouldCreateDeterministicCommitReceipt`（happy path）
- `commitAndPush_whenPrivateOrReparseContentFound_shouldReject`
- `commitAndPush_whenRemoteUnavailable_shouldRemainPendingRemote`

CandidateBuildService.buildAndSeal:
- `buildAndSeal_whenBuildOutputsMatchPolicy_shouldCreateImmutableDigest`（happy path）
- `buildAndSeal_whenDigestChangesAfterSeal_shouldRejectArtifact`

## Wave 8

## Task 8: 性格 checkpoint 与用户恢复

- **关联设计**: §3.9 [TCB] 性格 checkpoint 与用户恢复
- **状态**: [ ]
- **Wave**: 8
- **依赖**: Task 3, Task 7

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/checkpoint/personality_checkpoint_service.*`
- 实现: `DesktopPet-RSI-Supervisor/src/checkpoint/personality_card_renderer.*`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_personality_checkpoint.cpp`

**测试用例**:

PersonalityCheckpointService.create:
- `create_whenSnapshotCurrent_shouldSealSnapshotCardAndManifest`（happy path）
- `create_whenArtifactCannotBeSealed_shouldNotInsertCheckpoint`

PersonalityCheckpointService.compare:
- `compare_whenCheckpointsValid_shouldReturnDeterministicTraitDiff`（happy path）

PersonalityCheckpointService.restore:
- `restore_whenOwnerAndRevisionMatch_shouldCreateNewPersonalityRevision`（happy path）
- `restore_whenRevisionChanged_shouldRejectWithoutOverwritingHistory`
- `restore_whenCheckpointTampered_shouldReject`

## Wave 9

## Task 9: Evaluator 与用户反馈评测

- **关联设计**: §3.10 [TCB+PET] Evaluator 与用户反馈评测
- **状态**: [ ]
- **Wave**: 9
- **依赖**: Task 3, Task 4, Task 7, Task 8

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/evaluation/candidate_evaluation_service.*`
- 实现: `DesktopPet-RSI-Supervisor/src/evaluation/fixed_gate_runner.*`
- 实现: `core/ai/evolution/evaluator_session.*`
- 修改: `CMakeLists.txt`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_candidate_evaluation.cpp`
- 测试: `tests/test_evaluator_session.cpp`

**测试用例**:

CandidateEvaluationService.runFixedGates:
- `runFixedGates_whenTrustedSuitePasses_shouldBindReportToArtifact`（happy path）
- `runFixedGates_whenCandidateClaimsSuccessButGateFails_shouldFailCandidate`

CandidateEvaluationService.runCognitiveEvaluation:
- `runCognitiveEvaluation_whenGatePassed_shouldUseIndependentRedactedContext`（happy path）

CandidateEvaluationService.recordFeedback:
- `recordFeedback_whenUserExplicitlyPraises_shouldBindRuntimeAndCommit`（happy path）
- `recordFeedback_whenSingleCloseObserved_shouldRemainLowConfidence`

## Wave 10

## Task 10: A/B 试运行、健康检查与代码回滚

- **关联设计**: §3.11 [TCB] A/B 试运行、健康检查与代码回滚
- **状态**: [ ]
- **Wave**: 10
- **依赖**: Task 1, Task 2, Task 3, Task 4, Task 7, Task 8, Task 9

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/release/release_controller.*`
- 实现: `DesktopPet-RSI-Supervisor/src/release/release_manifest.*`
- 实现: `DesktopPet-RSI-Supervisor/src/release/atomic_slot_pointer.*`
- 实现: `DesktopPet-RSI-Supervisor/src/health/health_monitor.*`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_release_controller.cpp`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_health_monitor.cpp`

**测试用例**:

ReleaseController.startTrial:
- `startTrial_whenCandidateEligible_shouldCheckpointAndUseInactiveSlot`（happy path）
- `startTrial_whenPreCheckpointFails_shouldNotLaunchCandidate`

ReleaseController.evaluateHealth:
- `evaluateHealth_whenOsAndHeartbeatHealthy_shouldAdvanceWindow`（happy path）
- `evaluateHealth_whenHeartbeatLost_shouldReturnHardFailure`

ReleaseController.promote:
- `promote_whenRiskCompatibleAndHealthy_shouldAtomicallyMarkNewStable`（happy path）
- `promote_whenCapabilityExpands_shouldRequireOwnerApproval`

ReleaseController.rollback:
- `rollback_whenTrialHardFails_shouldRestoreStableCodeAndKeepLifeState`（happy path）
- `rollback_whenPointerAndDatabaseDisagree_shouldChooseLastVerifiedStable`

## Wave 11

## Task 11: 独立恢复工具与所有者审批

- **关联设计**: §3.12 [TCB] 独立恢复工具与所有者审批
- **状态**: [ ]
- **Wave**: 11
- **依赖**: Task 2, Task 3, Task 4, Task 8, Task 10

**文件**:
- 实现: `DesktopPet-RSI-Supervisor/src/owner/owner_control_service.*`
- 实现: `DesktopPet-RSI-Supervisor/src/recovery_ui/main.cpp`
- 实现: `DesktopPet-RSI-Supervisor/src/recovery_ui/recovery_window.*`
- 修改: `launcher/supervisor_client.py`
- 修改: `launcher/main.py`
- 修改: `launcher/process_tracker.py`
- 修改: `CMakeLists.txt`
- 测试: `DesktopPet-RSI-Supervisor/tests/test_owner_control_service.cpp`
- 测试: `tests/test_supervisor_launcher_client.py`

**测试用例**:

OwnerControlService.getStatus:
- `getStatus_whenPetMissing_shouldStillReturnTrustedReleaseAndFailureState`（happy path）

OwnerControlService.decideApproval:
- `decideApproval_whenRevisionCurrent_shouldRecordOwnerDecision`（happy path）
- `decideApproval_whenRevisionStale_shouldRequireRefresh`

OwnerControlService.rollbackCode:
- `rollbackCode_whenStableTargetValid_shouldChangeCodeWithoutLifeState`（happy path）

OwnerControlService.restorePersonality:
- `restorePersonality_whenCheckpointValid_shouldCreateNewRevisionWithoutCodeChange`（happy path）

OwnerControlService.emergencyStop:
- `emergencyStop_whenJobsActive_shouldTerminateAndPauseRestart`（happy path）

SupervisorLauncherClient.startPet:
- `startPet_whenSupervisorAvailable_shouldRequestManagedLaunchWithoutPopen`（happy path）
- `startPet_whenSupervisorUnavailable_shouldFailClosedWithoutDirectLaunch`
