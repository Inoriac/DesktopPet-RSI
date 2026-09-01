# DesktopPet RSI 代码级自主进化实现上下文

## 需求来源

- 上游需求：`.evo/tasks/desktop-pet-rsi-code-evolution/design.md`
- 系分：`.evo/tasks/desktop-pet-rsi-code-evolution/桌宠RSI代码级自主进化-桌面端系分.md`

## 决策记录

- [2026-09-01] macOS 环境不验证 ONNX 与 Windows 原生 Service/Token/AppContainer/Job/ACL/Named Pipe peer token/Authenticode；平台无关单测照常执行，Windows-only 验证在实现总结中明确列为未验证。
- [2026-09-01] 优先运行 Task 相关单测，不默认执行全量测试；仅在 Wave 依赖或共享构建文件改动确有必要时扩大范围。
- [2026-09-01] 每个 §3.X Task 完成、审查和确认后立即形成独立 Git 提交；跨双仓 Task 分别提交。

## 设计偏差

- 无。

## 待回溯设计的发现

- 无。
