# C 版架构评价：复杂还是合理？

> 2026-05-10

## 一句话结论

**对于单人独立游戏项目，C 版的设计是务实的、合理的。但存在明显的技术债务，Rust 重构方向正确。**

---

## 合理的地方（做得对）

### 1. 按职责分目录
```
src/           — 引擎核心
src/systems/   — 子系统（Navi、火车、邮件）
src/characters/ — 角色 AI
data/          — 数据驱动（.ssl + .json）
```

→ 一个单人开发者能想清楚的分层，没有过度设计。

### 2. 数据驱动场景
场景文本、选项、跳转全部放在 `.ssl` 和 `.json` 里，C 代码只负责加载和执行。

→ **引擎和内容是分离的**，这很对。

### 3. 终端渲染精细
Takeover 模式的 ANSI escape 控制（光标 save/restore、插入行、raw mode）非常专业。

→ 不是随便写个 `printf` 就完事的，是真的懂终端。

### 4. 编译时角色开关
```c
option(CHARACTER_CHISA_ALIVE "Include Chisa" ON)
```

→ 用 CMake 选项控制角色是否存在，**零运行时开销**，对独立游戏很务实。

### 5. ECC 时间编码
24-bit + 5-bit Hamming + 1-bit 奇偶校验 = SECDED。

→ 把"时间 glitch"做成了真正的错误纠正码，不是假装的技术，**是认真的**。

---

## 不合理的地方（技术债务）

### 1. executor.c 过大（768行）

把场景跳转、命令解析、Navi 调用、火车系统、物品获取、Flag 设置、时间成本**全部塞在一个文件**。

**具体问题：**
- 50+ 个 `action_id → scene_id` 硬编码在 if-else 链里
- 新增一个场景跳转需要改 C 代码并重新编译
- `get_action_time_cost` 里所有动作的时间成本都是写死的

**为什么形成这样：**单人项目，硬编码是最快的实现方式。不是设计错误，是**缺少后期重构**。

### 2. scenes.c 手动注册表（53个 `#include`）

```c
#include "SCENE_00_ENTRY_data.h"
#include "SCENE_01_LAIN_ROOM_data.h"
// ... 53 个
```

→ 新增场景要手动加 `#include` 和 dispatch entry。**容易遗漏**（ANALYSIS 报告里说了这个问题）。

**修复方向：**`parse_scenes.py` 同时生成 `scene_registry.inc`，scenes.c 只 `#include "scene_registry.inc"`。

### 3. render_utils.c 职责过重（546行）

包含：普通渲染、takeover 渲染、光标管理、图像自适应缩放、时间显示、选项渲染、短暂消息。

→ 应该拆成 `render_normal.c`、`render_takeover.c`、`image_viewer.c`、`time_display.c`。

### 4. 字符串散落在 24 个 JSON 文件

`data/strings.json` 是空的 `{}`，全部 622 条字符串分散在 `strings_extra/*.json`。

→ 没有统一 ID 命名空间，容易冲突。但国际化（i18n）本来就没做，所以当前不是致命问题。

### 5. MIRA vs MIKA 命名混乱

CMake 选项叫 `CHARACTER_LAINS_SISTER_MIRA_ALIVE`，但角色设定是 Mika（美香）。

→ 开发过程中的名字混用，暴露了一人项目的随意性。

---

## 和 Rust 版的对比

| 维度 | C 版 | Rust 版 |
|------|------|---------|
| 模块拆分 | 目录合理，但文件过大 | 更细粒度，职责更清晰 |
| 场景跳转 | 硬编码在 C 中 | 纯函数 + Command 模式，更易测试 |
| 渲染层 | ANSI escape 手写，精细 | crossterm 封装，简化但功能不全 |
| 注册表 | 手动 `#include` | 可能自动生成（需验证） |
| 类型安全 | C 字符串比较 | Rust enum + 类型系统 |
| 可维护性 | 一人能 hold 住 | 更适合长期迭代 |

---

## 最终评价

**C 版不是"设计过度复杂"，而是"实现足够快、但缺少自动化工具"。**

对于一个要快速验证玩法的单人项目：
- ✅ 硬编码场景跳转 = 正确选择（快）
- ✅ 手写注册表 = 可接受（场景数量可控）
- ❌ 缺少 `scene_registry.inc` 自动生成 = 应该补
- ❌ executor.c 过大 = 应该拆

**Rust 重构的价值不是"C 版设计错了"，而是"C 版验证了的玩法，需要用更好的工程结构承载"。**

C 版是原型，Rust 版是生产代码。这个升级路径是对的。
