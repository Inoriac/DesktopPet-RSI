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

## 设计偏差

- [code-impl Task 1 sanity check] §3.2.5 现有参照行号偏差：系分原写 `launcher/main.py:334` 为直接 `subprocess.Popen`，实际调用位于 `:391`、`:396`；已回溯修正系分锚点。
- [code-impl Task 1 sanity check] §3.2.5 要求 Service/Session Host 共用 `src/protocol/host_messages.h`，初版 impl-plan Task 1 文件清单遗漏该文件；已补入 Task 1，不改变接口契约。
- [code-impl Task 1 code review] §3.2 原接口把 native process 创建成功等同于 IPC 认证完成，导致未握手进程被标记 Running；已回溯增加 `confirmAuthenticated(sandboxId, trustedPeerPid)`，明确 `Launching -> Running` 只能由 §3.3 的内核 peer PID 校验触发。

## 待回溯设计的发现

- [resolved] Task 1 的 launcher 行号和 `host_messages.h` 文件边界已在系分与 impl-plan 修正。
- [resolved] Task 1 与 Task 2 的启动认证边界已通过 `confirmAuthenticated` 契约补齐。
