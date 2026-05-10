# C 版 vs Rust 版架构对比

> 2026-05-10

## C 版（main 分支）文件规模

| 文件 | 行数 | 职责 |
|------|------|------|
| `src/executor.c` | 768 | **核心执行器** — 场景跳转、命令解析、Navi调用、火车系统、自动事件、条件检查、物品获取、Flag设置、时间成本 |
| `src/render_utils.c` | 546 | **渲染层** — 普通/接管模式、光标管理、图像自适应、时间显示、选项渲染 |
| `src/scenes.c` | 137 | 场景管理、注册表 |
| `src/systems/embedded_navi.c` | ~240 | Navi 嵌入式系统 |
| `src/systems/navi_mini.c` | ~126 | Navi Mini |
| `src/systems/navi_pro.c` | ~13 | Navi Pro（骨架） |
| `src/systems/navi_shell.c` | ~265 | Navi Shell |
| `src/systems/train_system.c` | ~213 | 火车系统 |
| `src/systems/mail_system.c` | ~363 | 邮件系统 |
| `src/systems/mystery_system.c` | ~327 | 谜题系统 |
| `src/characters/mika.c` | ~182 | Mika AI 调度 |
| `src/main.c` | ~271 | 入口、主循环 |

**C 版特点**：
- 两个超大文件（executor.c 768行 + render_utils.c 546行）承载了 90% 游戏逻辑
- 场景跳转全硬编码在 executor.c 的 if-else 链中
- 渲染层直接操作 ANSI escape sequences

---

## Rust 版（rust-rewrite 分支）文件规模

| 文件 | 行数 | 职责 |
|------|------|------|
| `src/app.rs` | 561 | **主应用循环** — 初始化、渲染、输入、执行、切换 |
| `src/render.rs` | 359 | 渲染层（刚补上时间原地更新 + flush_input） |
| `src/narrative/executor.rs` | 307 | 纯函数执行器（state + action_id → Command 列表） |
| `src/narrative/conditions.rs` | 202 | 条件判断系统 |
| `src/narrative/command.rs` | 106 | Command 模式定义 |
| `src/systems/mail.rs` | 427 | 邮件系统 |
| `src/systems/mystery.rs` | 305 | 谜题系统 |
| `src/systems/browser.rs` | 280 | 浏览器 |
| `src/systems/navi_mini.rs` | 111 | Navi Mini |
| `src/systems/train.rs` | 79 | 火车系统（骨架） |
| `src/characters/mika.rs` | ~71 | Mika 角色 |
| `src/engine/state.rs` | 156 | 游戏状态 |
| `src/engine/time.rs` | 198 | ECC 时间编码 |
| `src/world/location.rs` | ? | 地点/地图 |
| `src/assets.rs` | ? | 资产加载 |

**Rust 版特点**：
- 按职责拆分清晰（narrative/, systems/, engine/, world/）
- Executor 纯函数设计：不直接改 state，返回 Command 列表
- 类型安全，比 C 版更易测试

---

## Rust 版缺失的功能（相对 C 版）

| 功能 | C 版 | Rust 版 | 状态 |
|------|------|---------|------|
| 原地更新时间 | ✅ `update_time_display_inplace` | ✅ 刚补上 | **完成** |
| 清空输入缓冲 | ✅ `tcflush` + `flush_input_buffer` | ✅ 刚补上 | **完成** |
| Takeover 光标管理 | ✅ `\033[s/u/L` 精确控制 | ⚠️ 简化版 | 基本可用 |
| 图像自适应渲染 | ✅ `ioctl(TIOCGWINSZ)` 缩放 | ❌ 无 | 待实现 |
| Navi Pro/Alpha/Embedded | ✅ 三个独立系统 | ❌ 只有 Mini | 待实现 |
| 火车系统 | ✅ 完整购票流程 | ⚠️ 骨架 | 待完善 |
| 自动事件触发 | ✅ `check_and_trigger_auto_events` | ❌ 无 | 待实现 |
| 场景跳转表 | ✅ 硬编码 50+ 条 | ⚠️ 部分 | 需验证完整度 |
| `arls` 命令 | ✅ 完整实现 | ❓ 未知 | 需检查 |
| `exper` 命令 | ✅ 完整实现 | ❓ 未知 | 需检查 |

---

## 结论

**Rust 版的模块拆分方向是对的**，比 C 版两个超大文件好很多。

**但核心差距在 executor 和渲染层的功能完整度**，不是架构问题。

C 版 executor.c 的 768 行里，Rust 版用 `narrative/executor.rs` (307行) + `systems/*.rs` 拆分承接，这个设计**不需要大改架构**，只需要**补全缺失的功能**。
