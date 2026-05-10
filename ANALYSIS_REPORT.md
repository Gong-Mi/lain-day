# lain-day 剧本与架构深度分析报告

> 分析范围：序章（Prologue）+ 第一章开头（Chapter 1 Intro）的 `.ssl` 场景文件、`strings_extra/*.json` 文本资产、C 引擎核心逻辑（`executor.c`, `render_utils.c`, `scenes.c`, `game_types.h`）。
> 分析日期：2026-05-09

---

## 一、执行摘要

`lain-day` 的 **技术骨架**（终端渲染、场景切换、时间系统、Navi 子系统）已经搭建完成，但 **剧本资产存在严重的结构性缺陷**。当前可玩的序章+第一章开头约包含 **20 个 `.ssl` 场景文件**，但 C 代码的 `story_transition_table` 中引用了 **52 个场景**，其中 **32 个场景的 `.ssl` 文件完全缺失**。

更深层的问题是：`.ssl` 格式本身不支持**条件分支对话**（conditional dialogue），导致一个本应基于玩家选择展示不同文本的场景（如母亲的分支），只能把两个分支的所有文本硬编码进去，让玩家无论如何选择都会看到全部内容。这是格式层面的设计缺陷，不是简单的"没写完"。

文本质量方面，622 条字符串资产**全部为单语中文**，没有真正的英文本地化；存在开发者注释泄露给玩家、技术时代错置（HTTP 状态码出现在 1998 年背景）、角色命名混淆（Mika vs Mira）等问题。

**评级：**
- 引擎架构：B+（功能完整，但有硬编码债务）
- 场景格式：D（不支持条件对话，description 用法混乱）
- 剧本文本：C-（有亮点，但角色一致性差、存在大量出戏元素）
- 整体完成度：~25%（仅序章有实质内容，第一章后大量留白）

---

## 二、致命缺陷（Critical Issues）

### 2.1 .ssl 格式不支持条件对话

**证据文件**：`data/scenes/story/02g_mom_reply_silent_endprologue.ssl`

```yaml
choices:
- action_id: mom_reply_fine
  text_id: TEXT_CHOICE_MOM_REPLY_FINE
- action_id: mom_reply_silent
  text_id: TEXT_CHOICE_MOM_REPLY_SILENT
dialogue:
- speaker: SPEAKER_NONE
  text_id: TEXT_TALK_TO_MOM_NORMAL_DESC1
# ... 接下来直接硬编码了 "结局A：日常" 和 "结局B：沉默" 的全部文本
- speaker: SPEAKER_NONE
  text_id: TEXT_MOM_REPLY_FINE_TITLE      # 结局A标题
- speaker: SPEAKER_NONE
  text_id: TEXT_MOM_REPLY_FINE_DESC1      # 结局A描述
# ...
- speaker: SPEAKER_NONE
  text_id: TEXT_MOM_REPLY_SILENT_TITLE    # 结局B标题
- speaker: SPEAKER_NONE
  text_id: TEXT_MOM_REPLY_SILENT_DESC1    # 结局B描述
```

**问题**：玩家选择了 `mom_reply_fine` 后，引擎按顺序打印所有 dialogue lines，包括结局B的文本。玩家被迫看完自己没有选择的结局。

**根因**：`.ssl` 的 `dialogue` 是一个扁平数组，没有 `if/branch` 语义。`parse_scenes.py` 生成的 C 代码也只是按顺序填充 `dialogue_lines[]`。

**修复方案**：
- 短期：为每个分支结局创建独立的 `.ssl` 文件（如 `02f_mom_reply_fine_endprologue.ssl`、`02g_mom_reply_silent_endprologue.ssl`），用 `target_scene` 跳转。
- 长期：在 `.ssl` 中引入 `branches` 语法：
  ```yaml
  branches:
    - condition: { flag: "mom_reply", value: "fine" }
      dialogue: [ ... ]
    - condition: { flag: "mom_reply", value: "silent" }
      dialogue: [ ... ]
  ```

### 2.2 32 个被引用的场景缺失 `.ssl` 文件

**证据**：`src/executor.c` 第 141-206 行的 `story_transition_table`。

**缺失的关键场景包括**：
- `SCENE_02F_MOM_REPLY_FINE_ENDPROLOGUE` —— 母亲分支 A（被 02g 硬编码替代）
- `SCENE_06_TRAIN_SCENE` / `SCENE_07_CLASSROOM` —— 第一章核心场景
- `SCENE_08B_ASK_TEACHER` / `SCENE_08C_ASK_PROXY` / `SCENE_08D_ASK_CHISA` —— 学校调查
- `SCENE_09_CYBERIA` / `SCENE_09A_PERSUASION` —— Cyberia 俱乐部
- `SCENE_19_COLD_OPEN_CH2` ~ `SCENE_22D_REPLY_IS_ME` —— 整個第二章

**后果**：任何试图进入第一章后续内容的玩家都会遭遇**场景加载失败**或**空场景**（因为 `scenes.c` 的 dispatch table 中也没有这些场景的 init 函数注册）。

### 2.3 strings.json 为空，所有文本散落在 strings_extra/

**证据**：`data/strings.json` 包含 0 个键。全部 622 条字符串分布在 24 个 `strings_extra/*.json` 中。

**问题**：
- `generate_string_ids.py` 的构建逻辑必须遍历所有 `strings_extra/*.json`，但命名约定混乱（`strings1.json`, `strings_map.json`, `strings_navi.json`, `strings_scenes_prologue.json`...）。
- 没有统一的文本 ID 命名空间管理，容易出现冲突。
- 国际化（i18n）只是纸上谈兵：所有 JSON 的值都是纯字符串（`str`），不是 `{ "en": "...", "zh": "..." }` 对象。如果游戏切换语言，没有英文文本可加载。

---

## 三、严重剧本问题（Major Narrative Issues）

### 3.1 开发者注释直接暴露给玩家

**证据**：`data/strings_extra/strings_scenes_downstairs.json`

```json
"TEXT_DAD_ASK_HELP_DESC3": "【开发者说明：此处进入游戏的帮助/教程系统。系统将以伪终端的形式，介绍核心的游戏命令，例如 `connect`, `scan`, `read`, `whoami` 等。教程结束后，玩家将返回客厅。】"
```

**问题**：这段文本会在游戏中直接显示给玩家。这是开发阶段的占位符，但当前被当作最终文本编译进二进制。

### 3.2 技术时代错置（Anachronism）

**证据**：`data/strings_extra/strings_navi.json`

```json
"TEXT_NAVI_CONNECT_STATUS_CODE": "> Status: 403 Forbidden"
"TEXT_NAVI_CONNECT_GHOST_QUOTE2": "不是'404 Not Found'，就还有希望..."
"TEXT_NAVI_REBOOT_GHOST_QUOTE4": "是吗？我软链接我自己吗？……不，并没有。我们都只是空文件而已。"
```

**问题**：
- Lain PS1 的故事背景是 1998 年。HTTP 状态码（403/404）虽然在 1998 年已经存在，但把它们作为**叙事核心隐喻**（"结局I：403 Forbidden"）是**程序员视角的投射**，不是 1998 年日本少女的视角。
- "软链接"（symbolic link）、"空文件" 是 Unix 文件系统概念。让"幽灵"说出这种话，完全破坏了角色的神秘感和世界观的一致性。
- **建议**：如果要保留技术隐喻，应使用 1998 年语境下的网络术语（如 "Connection Refused"、"DNS Error"、"Dial-up failed"），或更抽象的 Wired 原生术语。

### 3.3 角色命名混淆：Mika vs Mira

**证据**：
- `CMakeLists.txt`：`CHARACTER_LAINS_SISTER_MIRA_ALIVE`
- `src/render_utils.c`：`LAINS_SISTER_MIRA_COLOR`
- `data/strings_extra/strings_scenes_chapter1.json`：`"TEXT_SISTER_FIXED_DESC1": "你走到门口，姐姐 Mika 看着你。"`

**问题**：原作中 Lain 的姐姐叫 **美香（Mika）**。`MIRA` 似乎是项目内部人员或合作者的名字被错误地混入了代码和编译时选项中。这会导致玩家困惑，也暴露开发过程的混乱。

### 3.4 母亲角色性格矛盾

**证据**：
- `data/scenes/story/02d_talk_to_mom_normal.ssl`：
  > "美穗（母亲）甚至没有抬头。她只是机械地询问着你的情况，完全没有察觉到眼前的女儿已经被另一个意识占据了。"
- `data/strings_extra/strings_scenes_downstairs.json`（`TEXT_TALK_TO_MOM_NORMAL_DESC1`）：
  > "你走进厨房，妈妈停下了手中的活，转过身来，脸上带着温柔的微笑。"

**问题**：同一个角色在同一条路径中呈现出**完全相反的性格**（冷漠机械 vs 温柔关切）。原作中母亲美穗是一个**功能性关怀**的角色——她关心 Lain 是因为"母亲应该关心"，而不是真正的情感连接。她的温柔是**社会角色扮演**，而非发自内心的亲密。剧本需要统一这个层次。

### 3.5 姐姐场景全部没有玩家选择

**证据**：`04a_talk_to_sister_cold.ssl`、`04b_talk_to_sister_curious.ssl`、`04c_talk_to_sister_default.ssl`

```yaml
choices: []
```

**问题**：这三个场景是第一章开头的核心叙事节点——姐姐给 Lain 手机，引出千砂（Chisa）的邮件。但玩家**没有任何选择**，只能被动观看。这与视觉小说/交互小说的基本设计原则相悖。

**更严重的问题**：`sister_mood` 这个 flag 在 executor.c 中被设置（`navi_shutdown` → `curious`，`get_milk` → `normal`，`talk_to_figure` → `cold`），但三个姐姐场景中没有任何条件判断来根据 `sister_mood` 选择显示哪个场景。`04a/b/c` 是三个独立的 `.ssl` 文件，但引擎逻辑中没有任何代码根据 `sister_mood` 切换它们。`executor.c` 的 `story_transition_table` 中有：
```c
{"talk_to_sister_cold", "SCENE_04A_TALK_TO_SISTER_COLD", NULL, NULL},
{"talk_to_sister_curious", "SCENE_04B_TALK_TO_SISTER_CURIOUS", NULL, NULL},
{"talk_to_sister_default", "SCENE_04C_TALK_TO_SISTER_DEFAULT", NULL, NULL},
```
但没有任何 action_id 会触发这些 transition。`03_chapter_one_intro.ssl` 的唯一选择是 `TEXT_CHOICE_TALK_TO_SISTER`（`action_id: talk_to_sister`），而这个 action_id 在 `story_transition_table` 中**不存在**。

### 3.6 "停火"一词严重出戏

**证据**：`data/strings_extra/strings_scenes_prologue.json`

```json
"TEXT_TALK_FIGURE_TITLE": "序章结束（结局G：停火）"
"TEXT_TALK_FIGURE_GHOST_QUOTE3": "不是入侵，这一切都是自动。我也没有选择……要不，我们暂时停火好吗？"
```

**问题**："停火"（ceasefire）是军事术语。在 Lain 的内心世界/卧室中，两个意识的谈判用"停火"来形容，极其不协调。Lain PS1 的核心美学是**冷漠的技术语言与私人情感的错位**，但"停火"不是技术语言，而是战争语言，它引入了一个完全无关的语义场。

### 3.7 视角混乱：第二人称、第一人称、系统提示混用

**证据**：
- `TEXT_PROLOGUE_LINE_1`："你站在走廊上。"（第二人称，元叙事/系统视角）
- `TEXT_CH1_INTRO_YOU_QUOTE1`："……闹钟？"（第一人称，Lain 自己说话）
- `TEXT_CH1_INTRO_NAVI_QUOTE1`："【你昨天把手机借给姐姐了。】"（系统提示，方括号）

**问题**：游戏在三种视角之间无过渡地切换。第二人称在序章中营造了一种"你被观察"的疏离感（符合"旧约"美学），但第一章突然变成了传统视觉小说的第一人称+系统提示混用。这种切换如果没有明确的**叙事框架**来解释（例如：方括号内是 Navi 的提示，方括号外是 Lain 的感知，斜体/无引号是系统视角），只会让玩家困惑。

### 3.8 父亲角色过于哲学化

**证据**：`data/strings_extra/strings_scenes_downstairs.json`

```json
"TEXT_DAD_ASK_HELP_DAD_QUOTE2": "世界的基础是连接，而连接的工具就是命令。仔细看好……"
```

**问题**：岩仓康男（Yasuo Iwakura）在原作中是一个**冷漠的、将家庭视为研究对象**的父亲。他确实会说出技术化的话，但这句话过于**宏大叙事**（"世界的基础"），不像一个把女儿当作实验品的科学家的口吻。原作的康男更隐蔽、更操作化——他会说"Navi 的配置需要优化"或"Wired 的连接稳定性比现实更重要"，而不是直接宣讲哲学。

---

## 四、角色"边界协议"分析（基于项目翻译理论框架）

项目文档强调要重建每个角色的"边界协议"（boundary protocol）——即角色如何通过语言计算和维护人际距离。以下是对当前剧本的评估：

### 4.1 父亲（岩仓康男）—— 边界协议：失败

- **应然**：康男的边界是**单向玻璃**——他观察你，但你不能观察他。他的语言是**操作指令式**的，不带情感温度，但也不张扬。
- **实然**：当前剧本中，康男要么完全沉默（看报纸），要么突然宣讲"世界的基础是连接"。这破坏了单向玻璃的效果——他突然变得**可被理解**了。
- **修复方向**：康男的台词应该更**具体、更技术性、更回避**。例如，当 Lain 请求帮助时，他不应该说"也好"（这暗示了情感回应），而应该先沉默，然后说"Navi 的日志显示异常流量。你做了什么？"

### 4.2 母亲（岩仓美穗）—— 边界协议：矛盾

- **应然**：美穗的边界是**社会角色扮演**——她的关怀是真实的，但真实的是"扮演母亲"这个动作本身，而不是对 Lain 的爱。她害怕打破角色，因为打破角色意味着面对一个她无法理解的女儿。
- **实然**：当前剧本中，美穗在"机械询问"和"温柔微笑"之间摇摆。两个版本都过于表面——机械的版本中她没有恐惧，温柔的版本中没有表演的焦虑。
- **修复方向**：统一的母亲语音应该是**过度补偿式的关怀**。例如："（快速眨眼）……啊，玲音。饭、饭吃了吗？（笑，但嘴角有轻微抽搐）"——她在努力维持"正常母亲"的角色，但细微处泄露了不安。

### 4.3 姐姐（Mika）—— 边界协议：缺失

- **应然**：原作中 Mika 是一个**自我中心的、对 Lain 有微妙敌意**的姐姐。她把 Lain 视为家中的"异常元素"，影响了自己"正常女儿"的身份。她的边界是**排斥**——不是直接攻击，而是通过忽视和隐性竞争来维持距离。
- **实然**：当前剧本中，Mika 只是一个**功能性 NPC**——给手机、说几句台词、离开。她没有对 Lain 的任何情绪反应。
- **修复方向**：
  - `cold` 版本：Mika 不应该只是"给"手机。她应该**扔**手机，或者说"拿去，别烦我。"（用动作和省略来传递敌意）
  - `curious` 版本：Mika 的"好奇"不应该是对 Lain 的关心，而应该是**窥探后的轻蔑**。例如："哟，终于舍得下楼了？（停顿，上下打量）……千砂？那个跳楼的？你朋友品味真独特。"

### 4.4 "幽灵"/另一意识 —— 边界协议：过度解释

- **应然**：这个存在应该是**不可理解**的。它的语言不遵循日常逻辑，是碎片化的、镜像式的。它不是在和 Lain "谈判"，而是在**宣告**。
- **实然**：当前剧本中，"幽灵"说话像哲学论文（"我只是被世界选中的载体"、"这一切都是自动"），而且它还在**请求停火**（"要不，我们暂时停火好吗？"）。这让它变得**可被协商**，失去了恐怖感。
- **修复方向**：幽灵的台词应该更**碎片化、更重复、更非人**。例如：
  > "你。我。没有区别。\n月亮。眼睛。同一个。\n……你关不掉。"

### 4.5 Lain —— 边界协议：过于主动

- **应然**：原作中 Lain 的核心特质是**空白**——她是一个吸收周围人投射的空容器。她的语言极少，当她说话时，往往是在**重复别人**的话，或者说出**不应由她说出**的话。
- **实然**：当前剧本中，Lain 非常主动（"你这个幽灵……是想占据我的全部吗？"、"我的手机呢？"）。她有自己的情绪、自己的欲望、自己的对抗意识。这让她变成了一个**普通的视觉小说女主**。
- **修复方向**：Lain 的台词应该更**延迟、更重复、更不确定**。例如：
  - 不应该说"我的手机呢？"（主动追问）
  - 应该说"……手机？"（延迟反应，像是在回忆这个词的含义）
  - 面对幽灵时，不应该质问，而应该是**复述**或**模仿**。

---

## 五、架构与设计问题

### 5.1 `story_transition_table` 硬编码在 C 中

**位置**：`src/executor.c` 第 141-206 行。

**问题**：52 条场景跳转规则全部写在 C 数组中。添加一个新场景需要修改 C 代码并重新编译。这与 `.ssl` "数据驱动"的设计目标相矛盾。

**建议**：将 `story_transition_table` 迁移到 JSON/YAML，在启动时加载。

### 5.2 `scenes.c` 中的硬编码 `#include` 列表

**位置**：`src/scenes.c` 第 12-53 行。

**问题**：53 个 `#include "SCENE_XXX_data.h"` 和 53 个 dispatch table entries 全部是手写的。CMake 虽然能自动生成 `SCENE_XXX_data.c/h`，但 `scenes.c` 的注册表必须手动更新。这解释了为什么很多自动生成的场景没有被注册——开发者遗漏了。

**建议**：`parse_scenes.py` 应该同时生成一个 `scene_registry.inc` 文件，包含所有 `#include` 和 dispatch entries，然后 `scenes.c` 用 `#include "scene_registry.inc"` 引入。

### 5.3 `get_action_time_cost` 硬编码

**位置**：`src/executor.c` 第 80-114 行。

**问题**：所有动作的时间成本（移动 1 分钟、谈话 5 分钟、去涩谷 25 分钟）全部硬编码在 C 函数中。

**建议**：迁移到 `data/time_costs.json`。

### 5.4 `SPEAKER_MIKA` 显示英文名 "Mika"

**位置**：`src/render_utils.c` 第 119 行。

```c
{SPEAKER_MIKA, "Mika", LAINS_SISTER_MIRA_COLOR},
```

**问题**：其他角色显示中文名（"你"、"妈妈"、"爸爸"、"幽灵"），但 Mika 显示英文名 "Mika"。这与 `TEXT_SISTER_FIXED_DESC1` 中"姐姐 Mika"的混用命名一致。

---

## 六、具体修复清单（按优先级排序）

### P0：阻止游戏正常运行的缺陷
1. [ ] **修复 `02g_mom_reply_silent_endprologue.ssl` 的分支硬编码**：拆分为 `02f_mom_reply_fine_endprologue.ssl` 和 `02g_mom_reply_silent_endprologue.ssl` 两个独立文件。
2. [ ] **创建缺失的 32 个 `.ssl` 场景文件**，或从 `story_transition_table` 中移除未实现的引用（防止运行时崩溃）。
3. [ ] **修复 `03_chapter_one_intro.ssl` → 姐姐场景的跳转**：添加 `talk_to_sister` action_id 到 `story_transition_table`，并根据 `sister_mood` flag 选择目标场景。

### P1：叙事完整性
4. [ ] **删除 `TEXT_DAD_ASK_HELP_DESC3` 中的开发者注释**，替换为实际的教程系统入口或合理的叙事文本。
5. [ ] **统一母亲角色的性格**：重写 `02d` 和 `02g` 中的母亲描述，确立"社会角色扮演+细微焦虑"的统一形象。
6. [ ] **重写"幽灵"台词**：去除"停火"、"载体"、"选择"等哲学化/军事化词汇，改用碎片化、镜像式、非人的语言。
7. [ ] **去除 Navi 文本中的程序员梗**："软链接"、"空文件"、"403 Forbidden" 应替换为 1998 年语境下的术语或抽象隐喻。

### P2：角色边界协议重建
8. [ ] **重写父亲台词**：从哲学宣讲改为技术性操作指令，恢复"单向玻璃"效果。
9. [ ] **重写姐姐场景**：为 Mika 添加敌意/轻蔑/窥探的层次，不是功能性 NPC。
10. [ ] **弱化 Lain 的主动性**：将质问式台词改为延迟反应、复述、模仿。

### P3：一致性与技术债务
11. [ ] **统一命名**：将所有 `MIRA` 改为 `MIKA`（CMake 选项、颜色宏、代码注释）。
12. [ ] **统一 `SPEAKER_MIKA` 的显示名为 "美香" 或 "姐姐"**。
13. [ ] **统一 `.ssl` 中 `description` 字段的用法**：全部改为引用 `text_id`（如 `description: TEXT_XXX`），或直接移除 `description` 统一使用 `dialogue`。
14. [ ] **生成 `scene_registry.inc`**，消除 `scenes.c` 中的手动 `#include` 和 dispatch table 维护。
15. [ ] **迁移 `story_transition_table` 到外部 JSON**。

### P4：国际化
16. [ ] **重新设计 `strings_extra/*.json` 的结构**：从纯字符串改为 `{ "en": "...", "zh": "..." }` 对象。
17. [ ] **补充英文翻译**：当前 622 条字符串全部只有中文。

---

## 七、亮点与可保留的设计

尽管存在上述大量问题，以下设计决策是**正确且值得保留**的：

1. ** timed text / `is_takeover` 模式**：序章中系统强制逐行显示文本、禁止跳过、精确控制 delay 的设计，成功营造了"被系统控制"的压迫感。这是 Lain PS1 "旧约"美学的正确移植。
2. **Navi 子系统的分层**（`navi_mini.c`, `navi_pro.c`, `navi_alpha.c`, `embedded_navi.c`）：将不同层次的 Navi 体验分离，符合原作中 Navi 从工具到身份延伸的叙事弧线。
3. **时间系统的 ECC 编码**：用错误纠正码编码游戏时间，在时间显示上制造" glitch "效果，是巧妙的技术-叙事融合。
4. **Persona Permission 模型**（RWX bits for Lain/Shu）：用 Unix 文件权限模型映射两个意识的控制权，是一个强有力且可扩展的元游戏机制。
5. **`sister_mood` flag 的设计意图**：通过序章中的不同选择影响第一章姐姐的态度，这是正确的分支设计思路。只是当前实现不完整。

---

*报告结束。建议优先处理 P0 和 P1 项，使序章到第一章的过渡成为可完整体验、无逻辑断裂的单元。*
