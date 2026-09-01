# DesktopPet RSI 代码级自主进化实现上下文

## 需求来源

- 上游需求：`.evo/tasks/desktop-pet-rsi-code-evolution/design.md`
- 系分：`.evo/tasks/desktop-pet-rsi-code-evolution/桌宠RSI代码级自主进化-桌面端系分.md`

## 决策记录

- [2026-09-01] macOS 环境不验证 ONNX 与 Windows 原生 Service/Token/AppContainer/Job/ACL/Named Pipe peer token/Authenticode；平台无关单测照常执行，Windows-only 验证在实现总结中明确列为未验证。
- [2026-09-01] 优先运行 Task 相关单测，不默认执行全量测试；仅在 Wave 依赖或共享构建文件改动确有必要时扩大范围。
- [2026-09-01] 每个 §3.X Task 完成、审查和确认后立即形成独立 Git 提交；跨双仓 Task 分别提交。
- [code-impl Task 1 sanity check] §3.2.5 的 restricted token、Low Integrity、AppContainer、Job Object、completion port 和 canonical path handle 锚点已在 TCB 实现中对应；非 Windows 生产后端明确 fail-closed，未引入 `QProcess`/`Popen` 降级路径。
- [code-impl Task 1 验证] 定向 QtTest 18/18 通过；Service、Session Host 与平台无关沙箱代码通过 `-Wall -Wextra -Wpedantic` 编译；非 Windows 两个入口均以状态 1 拒绝运行。当前机器无 CMake 和 Windows SDK，因此未验证 CMake configure、`_WIN32` 编译及真实 Token/AppContainer/ACL/Job 行为。
- [code-impl Task 1 工程取舍] 当前以可运行的最小边界为准：信任签名安装/准备流程预置并保护目录 ACL，运行时对声明根目录执行 fail-closed 校验；不在 Task 1 预建递归 Authz 扫描、动态 ACL 租约或独立墙钟计时系统。真实 Windows 集成或后续健康监控暴露具体问题时再按证据收敛。
- [code-impl Task 2 sanity check] §3.3.5 的 4-byte big-endian framing 参照已确认，Task 1 的受控 PID/AppContainer SID/角色与 `ProcessSandboxService::confirmAuthenticated` 契约可直接用于 `Launching -> Running` 认证。
- [code-impl Task 2 验证] FrameCodec 7/7、TrustedIpcGateway 10/10，合计 17/17 定向单测通过；IPC 与关联 Task 1 源码通过 `-Wall -Wextra -Wpedantic` 编译。当前无 CMake/Windows SDK，未验证 `_WIN32` Named Pipe/Token/DACL；本机 arm64 Qt `neon` 不兼容，定向测试沿用 x86_64 Qt 6.8.3。
- [code-impl Task 2 code review] 不可信 Pipe 客户端的 DACL 权限已从 `GENERIC_ALL` 收紧为 `FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE`，并由共用 SDK 常量和回归测试排除 `FILE_CREATE_PIPE_INSTANCE/WRITE_DAC/WRITE_OWNER`。

## 设计偏差

- [code-impl Task 1 sanity check] §3.2.5 现有参照行号偏差：系分原写 `launcher/main.py:334` 为直接 `subprocess.Popen`，实际调用位于 `:391`、`:396`；已回溯修正系分锚点。
- [code-impl Task 1 sanity check] §3.2.5 要求 Service/Session Host 共用 `src/protocol/host_messages.h`，初版 impl-plan Task 1 文件清单遗漏该文件；已补入 Task 1，不改变接口契约。
- [code-impl Task 1 code review] §3.2 原接口把 native process 创建成功等同于 IPC 认证完成，导致未握手进程被标记 Running；已回溯增加 `confirmAuthenticated(sandboxId, trustedPeerPid)`，明确 `Launching -> Running` 只能由 §3.3 的内核 peer PID 校验触发。
- [code-impl Task 2 sanity check] Task 2 的 IPC 源码与测试必须接入 TCB 构建，初版 impl-plan 文件清单遗漏 `DesktopPet-RSI-Supervisor/CMakeLists.txt`；已补入 Task 2，不改变 §3.3 接口契约。

## 待回溯设计的发现

- [resolved] Task 1 的 launcher 行号和 `host_messages.h` 文件边界已在系分与 impl-plan 修正。
- [resolved] Task 1 与 Task 2 的启动认证边界已通过 `confirmAuthenticated` 契约补齐。
- [pending] Task 2 当前为内存 session/幂等记录；Task 3 引入 SQLite/LifeLedger 后，需在最终 Service composition 中补齐 `runtime_session`、24 小时持久幂等与可信审计，不得以当前内存实现作为最终交付。
