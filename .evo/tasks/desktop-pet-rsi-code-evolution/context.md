# DesktopPet RSI 代码级自主进化实现上下文

## 需求来源

- 上游需求：`.evo/tasks/desktop-pet-rsi-code-evolution/design.md`
- 系分：`.evo/tasks/desktop-pet-rsi-code-evolution/桌宠RSI代码级自主进化-桌面端系分.md`

## 决策记录

- [2026-09-01] macOS 环境不验证 ONNX 与 Windows 原生 Service/Token/AppContainer/Job/ACL/Named Pipe peer token/Authenticode；平台无关单测照常执行，Windows-only 验证在实现总结中明确列为未验证。
- [2026-09-01] 优先运行 Task 相关单测，不默认执行全量测试；仅在 Wave 依赖或共享构建文件改动确有必要时扩大范围。
- [2026-09-01] 每个 §3.X Task 完成、审查和确认后立即形成独立 Git 提交；跨双仓 Task 分别提交。
- [2026-09-02] Task 5 采用分阶段接入：先实现 transport-neutral Supervisor adapter、真实 runtime composition 注入点和受监督模式 fail-closed 门禁；Windows bootstrap handle、Named Pipe transport、Session Host 参数及 launcher `start_pet` 在 Task 11 接通。legacy 直启仅属于显式开发构建，不是 RSI 发布运行时 fallback。
- [code-impl Task 1 sanity check] §3.2.5 的 restricted token、Low Integrity、AppContainer、Job Object、completion port 和 canonical path handle 锚点已在 TCB 实现中对应；非 Windows 生产后端明确 fail-closed，未引入 `QProcess`/`Popen` 降级路径。
- [code-impl Task 1 验证] 定向 QtTest 18/18 通过；Service、Session Host 与平台无关沙箱代码通过 `-Wall -Wextra -Wpedantic` 编译；非 Windows 两个入口均以状态 1 拒绝运行。当前机器无 CMake 和 Windows SDK，因此未验证 CMake configure、`_WIN32` 编译及真实 Token/AppContainer/ACL/Job 行为。
- [code-impl Task 1 工程取舍] 当前以可运行的最小边界为准：信任签名安装/准备流程预置并保护目录 ACL，运行时对声明根目录执行 fail-closed 校验；不在 Task 1 预建递归 Authz 扫描、动态 ACL 租约或独立墙钟计时系统。真实 Windows 集成或后续健康监控暴露具体问题时再按证据收敛。
- [code-impl Task 2 sanity check] §3.3.5 的 4-byte big-endian framing 参照已确认，Task 1 的受控 PID/AppContainer SID/角色与 `ProcessSandboxService::confirmAuthenticated` 契约可直接用于 `Launching -> Running` 认证。
- [code-impl Task 2 验证] FrameCodec 7/7、TrustedIpcGateway 10/10，合计 17/17 定向单测通过；IPC 与关联 Task 1 源码通过 `-Wall -Wextra -Wpedantic` 编译。当前无 CMake/Windows SDK，未验证 `_WIN32` Named Pipe/Token/DACL；本机 arm64 Qt `neon` 不兼容，定向测试沿用 x86_64 Qt 6.8.3。
- [code-impl Task 2 code review] 不可信 Pipe 客户端的 DACL 权限已从 `GENERIC_ALL` 收紧为 `FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE`，并由共用 SDK 常量和回归测试排除 `FILE_CREATE_PIPE_INSTANCE/WRITE_DAC/WRITE_OWNER`。
- [code-impl Task 3 sanity check] §3.4.5 的事件接口、事务、schema、人格、记忆和现有 SQLite 六类 PET 锚点均已验证存在且语义一致；TCB 写入由独立 SQLite 事务实现，不复用 PET 可写数据库。
- [code-impl Task 3 验证] LifeLedger 12/12、LegacyImporter 4/4 定向 QtTest 通过；两个目标均通过 C++20 `-Wall -Wextra -Wpedantic` 编译且无 warning。当前机器无 CMake，未验证 CMake configure；本 Task 不含 Windows/ONNX 平台行为。
- [code-impl Task 3 code review] 补齐稳定 personality body 的 revision/trait 范围校验、非法 mutation operation 拒绝、事件与状态 mutation 语义绑定，以及加密 provider 失败统一映射 `LEDGER_UNAVAILABLE`；这些均由先失败后通过的回归测试覆盖。
- [code-impl Task 3 工程取舍] `PayloadProtector` 与 `ProducerCommitResolver` 采用强制注入且缺失即 fail-closed；测试只使用确定性 protector。真实 DPAPI/CNG provider 和 `candidateId -> gitCommit` 可信绑定留给 Secret Store 与最终 Service composition，不提供明文或伪加密生产降级。
- [code-impl Task 4 sanity check] §3.5.5 的 PET `ModelCompletionClient`、`ToolRuntime` 与 `PolicyEngine` 锚点均已验证存在且语义一致；TCB 侧复用可信 `AuthContext`、SQLite 单写事务和 LifeLedger 接线边界。
- [code-impl Task 4 验证] CapabilityBroker 9/9、ModelGateway 9/9 定向 QtTest 通过；两个目标均通过 C++20 `-Wall -Wextra -Wpedantic` 增量编译且无 warning。按降温要求未重复运行全量测试；Windows Credential Manager、DPAPI/CNG 未在 macOS 验证。
- [code-impl Task 4 code review] 修复 provider credential 回显在 usage 落库后才拒绝、耗尽的首个预算 lease 阻断后续有效 lease、DNS 地址解析 fail-open、模型 token 预占低估和 provider 输出超请求上限五个可复现问题；均由 RED→GREEN 回归覆盖。
- [code-impl Task 4 工程取舍] 当前仅实现 `network.http` 可执行能力，其余已知类型 fail-closed；网络执行器和模型 provider 使用 TCB 内注入接口，不在本 Task 引入完整 HTTP 栈或生产明文 secret fallback。非 Windows SecretStore 明确 fail-closed。
- [code-impl Task 5 sanity check] §3.6.5 的 `AgentRuntimeServices`/`AgentBootstrap`、`EventLedger::append`、人格/记忆 repository、`ModelCompletionClient`、`ToolRuntime` 和 launcher 锚点均已验证存在；Task 5 只接 transport-neutral adapter 与构建期门禁，Windows transport/launcher 仍按分阶段决定留给 Task 11。
- [code-impl Task 5 验证] SupervisorRuntimeAdapter 定向 QtTest 9/9 通过；Task 5 新源码和 composition 改造通过 C++20 `-Wall -Wextra -Wpedantic` 检查，两项 AgentRuntimeServices 门禁测试通过语法编译。当前机器无 CMake，未链接运行体量较大的既有 AgentRuntimeServicesTests；未验证 Windows bootstrap/Named Pipe、ONNX 或 Task 11 端到端接线。
- [code-impl Task 5 code review] 修复 malformed hello 发生在认证上下文建立前时 `close()` 未实际关闭 transport 的安全边界问题；新增 RED→GREEN 回归，确保畸形协议响应返回 `PROTOCOL_INVALID`、断开 transport 且 adapter 保持 not ready，同时正常首次 initialize 不被误关。
- [code-impl Task 5 工程取舍] legacy 本地权威 runtime 只在显式 `DESKTOP_PET_ENABLE_LEGACY_RUNTIME=ON` 构建中启用，默认构建要求 Supervisor；本阶段即使注入 transport 也保持 `SUPERVISOR_NOT_READY`，不伪造未完成的 Windows 可信 composition，也不回退本地 SQLite、直连模型或本地系统能力。
- [code-impl Wave 5 验证] 单 Task 无文件冲突；impl-plan 8 个用例均有对应测试且含 happy path，测试位于既有 `tests/test_*.cpp` 目录；§3.6.5 锚点与 sanity 留痕齐全。当前机器无 CMake，且按用户低负载约束未跑全量链接测试；使用已通过的 SupervisorRuntimeAdapter 9/9 定向测试和 AgentRuntimeServices 门禁语法编译收口。
- [code-impl Task 6 sanity check] §3.7.5 的 `AgentScheduler` deadline 调度、`DaydreamTriggerPolicy`、`DaydreamSleepAdapter::consolidateAsync`、`SleepCycleCoordinator`、人格投影及 Supervisor runtime state 接口锚点均已验证存在且可承接本切片。
- [code-impl Task 6 验证] EvolutionIntent 2/2、EvolutionOpportunityPolicy 5/5 业务用例定向复跑通过；新源码与调度/Daydream 改动已通过 C++20 `-Wall -Wextra -Wpedantic` 定向检查，既有 sleep-cycle 源码仅有原有 aggregate initializer warning。按低负载约束未重编或运行全量测试；未验证 CMake configure、Windows idle/power 或 ONNX。
- [code-impl Task 6 code review] 未发现需返工的可复现运行故障、数据丢失风险或直接安全边界违规。PET 只做 fail-safe 前置判断，evidence/profile/origin 校验、可信时间、预算、冷却和 candidate/request 唯一性仍由 TCB 在原子操作中重验；Daydream wrapper 解析失败会沿用原有 preserve 路径，不阻断记忆整理。
- [code-impl Task 6 工程取舍] 本阶段提供低频事件 deadline、自然窗口策略和注入式 Supervisor client，不提前实现 Windows idle/power 读取或最终 runtime composition。PET 配置最小休整即使被压低也不能越过 TCB 权威策略；生产 client 与 Daydream intent callback 未接通时保持无自主候选副作用。
- [code-impl Wave 6 验证] 单 Task 无文件冲突；impl-plan 7 个用例均有对应测试且含各核心方法 happy path，测试位于既有 `tests/test_*.cpp` 目录；§3.7.5 锚点与 sanity 留痕齐全。按用户低负载约束以 EvolutionIntent 2/2、EvolutionOpportunityPolicy 5/5 定向测试代替全量重跑，CMake、Windows idle/power 与 ONNX 仍未验证。

## 设计偏差

- [code-impl Task 1 sanity check] §3.2.5 现有参照行号偏差：系分原写 `launcher/main.py:334` 为直接 `subprocess.Popen`，实际调用位于 `:391`、`:396`；已回溯修正系分锚点。
- [code-impl Task 1 sanity check] §3.2.5 要求 Service/Session Host 共用 `src/protocol/host_messages.h`，初版 impl-plan Task 1 文件清单遗漏该文件；已补入 Task 1，不改变接口契约。
- [code-impl Task 1 code review] §3.2 原接口把 native process 创建成功等同于 IPC 认证完成，导致未握手进程被标记 Running；已回溯增加 `confirmAuthenticated(sandboxId, trustedPeerPid)`，明确 `Launching -> Running` 只能由 §3.3 的内核 peer PID 校验触发。
- [code-impl Task 2 sanity check] Task 2 的 IPC 源码与测试必须接入 TCB 构建，初版 impl-plan 文件清单遗漏 `DesktopPet-RSI-Supervisor/CMakeLists.txt`；已补入 Task 2，不改变 §3.3 接口契约。
- [code-impl Task 3 sanity check] Task 3 的 SQLite/LifeLedger 源码与测试必须接入 TCB 构建，初版 impl-plan 文件清单遗漏 `DesktopPet-RSI-Supervisor/CMakeLists.txt`；已补入 Task 3，不改变 §3.4 接口契约。
- [code-impl Task 3 sanity check] §3.4 接口期望可信 `auth.producerCommit`，但 Task 2 的实际 `AuthContext` 仅含 `candidateId`；Task 3 改为注入 `ProducerCommitResolver` 从受信 candidate 记录解析 commit，禁止接受 PET 自报 commit。最终绑定在 Service composition 完成。
- [code-impl Task 4 preflight] §3.5.4 明确新增 capability/budget/usage 数据，且 Task 4 源码和测试必须接入 TCB 构建；初版 impl-plan 文件清单遗漏 `CMakeLists.txt` 与 SQLite v1 migration，已补入，不改变 §3.5 接口契约。
- [code-impl Task 5 preflight] §3.6 原流程同时要求 adapter 接入与 launcher/Windows transport 完成，但初版 Task 5 文件清单遗漏实际 `ui/petwindow_ai.cpp` composition，launcher 又排在 Task 11。已按用户确认改为分阶段交付，并补齐 Task 5 composition 文件与 Task 11 最终接线文件。
- [code-impl Task 5 sanity check] §3.6 原写 SDK error 映射到 `DomainError{code,message,retryable,details}`，实际共享类型为 `{code,message,details}`。为避免扩大公共接口影响面，已修正为 `code/message` 直接映射，`retryable/correlationId` 以固定同名键保存在 `details`。

## 待回溯设计的发现

- [resolved] Task 1 的 launcher 行号和 `host_messages.h` 文件边界已在系分与 impl-plan 修正。
- [resolved] Task 1 与 Task 2 的启动认证边界已通过 `confirmAuthenticated` 契约补齐。
- [pending] Task 2 当前为内存 session/幂等记录；Task 3 引入 SQLite/LifeLedger 后，需在最终 Service composition 中补齐 `runtime_session`、24 小时持久幂等与可信审计，不得以当前内存实现作为最终交付。
- [pending] Task 3 的生产 composition 必须提供 Windows DPAPI/CNG `PayloadProtector`，并把已认证 `candidateId` 解析为受信 Git commit；任一依赖缺失时保持 `LEDGER_UNAVAILABLE`，不得降级为明文、非 keyed hash 或 PET 自报 commit。
- [pending] Task 4 的 capability lease/调用/拒绝/撤销审计当前保存在 append-only TCB audit 表；最终 Service composition 需同步形成 LifeEvent。`release.sign` 的 non-exportable CNG provider 由 sealed artifact/release Task 接入，接入前该能力保持不可签发。
- [resolved] Task 5 与 Task 11 的 runtime 接线边界已明确：Task 5 不依赖 Task 11 才能实现和测试，但正式 RSI 端到端启动需由 Task 11 完成 Windows transport 与 launcher 接线。
- [pending] Task 6 的 `EvolutionSupervisorClient` 生产实现、Scheduler 事件源及 Daydream intent callback 需在最终 Service/PET composition 中接到 TCB 原子接口；接通前不得用 PET 本地状态替代证据、可信时间、预算、冷却或唯一性判定。
