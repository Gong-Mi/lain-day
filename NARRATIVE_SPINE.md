# lain-day 剧情主线梳理与断裂点报告

> 基于 `data/scenes/`, `data/story/`, `data/strings_extra/`, `src/executor.c` 及现有 lore 文档整理
> 生成日期：2026-05-24

---

## 一、总览：目前已知的剧情时间线

游戏时间从**序章（第3天晚8点）**开始，经过第一章（第4天白天），进入第二章（时间点模糊）。

```
序章（Prologue）
├── 00_ENTRY（走廊）
│   ├── → 01_LAIN_ROOM_BROKEN（进房间）
│   │   ├── → PC_NAVI_DESKTOP（使用NAVI）
│   │   │   └── → 01B_NAVI_SHUTDOWN（关闭NAVI）
│   │   │   └── → 01D_NAVI_REBOOT（重启NAVI，见幽灵）
│   │   │   └── → 01E_NAVI_CONNECT（连接Wired，见幽灵）
│   │   └── → 01C_TALK_TO_FIGURE（与人影对话）→ 序章结束
│   ├── → 02_DOWNSTAIRS（下楼）
│   │   ├── → DAD_DAY_0 / DAD_HUB（与父亲对话）
│   │   │   ├── → 02B_DAD_REPLY_NO（拒绝帮助）
│   │   │   └── → 02C_DAD_ASK_HELP（请求帮助）
│   │   ├── → 02D_TALK_TO_MOM_NORMAL（与母亲对话）
│   │   │   ├── → 02G_MOM_REPLY_SILENT（沉默回应）→ 序章结束
│   │   │   └── → 02F_MOM_REPLY_FINE（正常回应）[.ssl缺失]
│   │   └── → 02J_GET_MILK（拿牛奶）→ 序章结束
│   └── → 00A_WAIT_ONE_MINUTE（等待1分钟）→ 序章结束
│
第一章（Chapter 1）
├── 03_CHAPTER_ONE_INTRO（房间，闹钟响）
│   └── → 04_TALK_TO_SISTER（与姐姐对话）[路由逻辑缺失]
│       ├── → 04A_TALK_TO_SISTER_COLD（冷漠版）
│       ├── → 04B_TALK_TO_SISTER_CURIOUS（好奇版）
│       └── → 04C_TALK_TO_SISTER_DEFAULT（默认版）
│           └── → 获取手机 + Chisa邮件
├── 06_TRAIN_SCENE（电车）[.ssl缺失，仅.md草稿]
│   └── → 主动过载 / 被动过载（分支）
├── 07_CLASSROOM（教室）[.ssl缺失，仅.md草稿]
│   ├── → 08B_ASK_TEACHER（问老师）[.ssl缺失]
│   ├── → 08C_ASK_PROXY（问代发）[.ssl缺失]
│   ├── → 08D_ASK_CHISA（问千砂）[.ssl缺失]
│   └── → 08E_ASK_ALICE_SCARED（问Alice是否害怕）[.ssl缺失]
│       └── → 09_CYBERIA（Cyberia邀请）[.ssl缺失]
│           ├── → 10A_GO_WITH_ALICE（一起去）→ 第一章完
│           ├── → 10B_ALONE_TOUKO（拒绝，想冬子医生）→ 第一章完
│           └── → 10C_ALONE_CHISA（拒绝，想千砂）→ 第一章完
│
第二章（Chapter 2）
├── 19_COLD_OPEN_CH2（冷开场：千砂自杀）[.ssl缺失，仅.md草稿]
├── 20_CHAPTER_TWO_INTRO（教室：水杯碎裂幻觉）[.ssl缺失，仅.md草稿]
│   ├── → 21A_HUG_ALICE（拥抱Alice）[.ssl缺失]
│   ├── → 21B_REPLY_FINE（回答没事）[.ssl缺失]
│   └── → CH2_ASK_WHO（问"那个是谁"）[.ssl缺失]
│       └── → 21C_ALICE_COMFORTS_LAIN（Alice给帽子）[.ssl缺失]
└── 22_CYBERIA_FLASHBACK（Cyberia闪回）[.ssl缺失，仅.md草稿]
    ├── → 22B_BOSS_INVITES_LAIN_TO_SING（老板邀唱歌）[.ssl缺失]
    └── → 22D_REPLY_IS_ME（"是我"）[.ssl缺失]
```

---

## 二、序章（Prologue）详细走线

### 2.1 起点：走廊（SCENE_00_ENTRY）
- **时间**：第3天晚上8点（`序章大纲.txt`）
- **地点**：iwakura_upper_hallway（岩仓家二楼走廊）
- **状态**：Lain 察觉到世界与记忆不符，父母关系不亲密。
- **系统提示**："你站在走廊上。" → 房门上写着"lain"，下面有模糊的字。
- **强制延迟**：文本逐行出现，不可跳过（`is_takeover: true`），营造被系统控制的压迫感。

**玩家选择**：
1. **打开房门** → `SCENE_01_LAIN_ROOM_BROKEN`
2. **下楼** → `SCENE_02_DOWNSTAIRS`
3. **等待1分钟后下楼** → `SCENE_02_DOWNSTAIRS`（延迟60秒）
4. **隐藏：等2分钟**（`序章大纲.txt` 提及，但 `.ssl` 中未实现）

### 2.2 分支A：房间（SCENE_01_LAIN_ROOM_BROKEN）
- **氛围**：房间无灯，只有显示器绿光。墙上海报晃动，电子设备噪音刺耳。
- **核心文本**："这里的空气不再属于现实世界。你感觉到'束'正在你耳边低语。"
- **NAVI 对话**：NAVI 发出故障欢迎语，Lain 回应。

**玩家选择**：
1. **使用 NAVI** → `SCENE_PC_NAVI_DESKTOP`
2. **离开房间** → 回到走廊 `SCENE_00_ENTRY`

#### 2.2.1 NAVI 桌面（SCENE_PC_NAVI_DESKTOP）
- 功能菜单：浏览 Wired / 查看本地文件 / 打开邮件 / 注销
- 邮件、聊天室、系统设置等子系统存在，但与主线剧情**未有效衔接**。
- **退出时的隐藏分支**：如果已和姐姐对话，退出到主线；否则触发 `trigger_shutdown_story`（`data/story/side_stories/navi.md`）。

#### 2.2.2 NAVI 关机 / 重启 / 连接（序章结局分支）
- **01B_NAVI_SHUTDOWN**：关闭 NAVI，Lain 说"……就这样吧。" → 序章结束，进入第一章。
- **01D_NAVI_REBOOT**：重启后见到"幽灵"（`SPEAKER_GHOST`）。幽灵说"我只是被世界选中的载体"，Lain 与之对话。→ 序章结束，进入第一章。
- **01E_NAVI_CONNECT**：连接 Wired，出现 HTTP 403 状态码（"Status: 403 Forbidden"），幽灵说"不是'404 Not Found'，就还有希望"。→ 序章结束，进入第一章。

#### 2.2.3 与人影对话（SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE）
- **标题**："序章结束（结局G：停火）"
- **对话**：Lain 与"幽灵"谈判。幽灵说"不是入侵，这一切都是自动"，请求"暂时停火"。
- **唯一选择**：`start_chapter_one` → 进入第一章。
- **问题**："停火"一词军事化，与 Lain 内心世界氛围不协调。且"停火"后没有任何过渡解释，直接切到"闹钟响了"。

### 2.3 分支B：楼下（SCENE_02_DOWNSTAIRS）
- **氛围**：压抑。父亲看报纸，母亲在厨房背对忙碌。电视音量低，餐桌未收拾。
- **核心文本**："客厅里弥漫着一种压抑的气氛。"

**可互动对象**：
1. **父亲**（通过 `DAD_DAY_0` 或 `DAD_HUB`）
2. **母亲**（`02D_TALK_TO_MOM_NORMAL`）
3. **牛奶**（`02J_GET_MILK`，直接拿牛奶结束序章）

#### 2.3.1 父亲线
- **DAD_DAY_0**：直接对话场景。父亲问"怎么了"。
- **DAD_HUB**：选择 hub。
  - 请求帮助 → `02C_DAD_ASK_HELP`：父亲说"世界的基础是连接，而连接的工具就是命令"，随后进入开发者注释占位符（`【开发者说明：此处进入游戏的帮助/教程系统】`）。**开发者注释直接暴露给玩家。**
  - 拒绝 → `02B_DAD_REPLY_NO`：父亲沉默，继续看报纸。
  - 返回客厅 → 回到 `SCENE_02_DOWNSTAIRS`

#### 2.3.2 母亲线
- **02D_TALK_TO_MOM_NORMAL**：描述中母亲"甚至没有抬头，只是机械地询问"。但 `strings_extra` 中同一场景又说"脸上带着温柔的微笑"。**性格矛盾**。
- **02G_MOM_REPLY_SILENT_ENDPROLOGUE**：`.ssl` 格式缺陷导致**两个分支（正常回应/沉默回应）的文本被硬编码在同一个文件中**。玩家无论选哪个，都会被迫看完两个结局。
- **02F_MOM_REPLY_FINE_ENDPROLOGUE**：`.ssl` 文件**缺失**，被硬编码进 `02G` 中替代。

#### 2.3.3 等待线（SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE）
- 在走廊等待1分钟后，姐姐 Mika 出现，说"你还不睡吗？" → 序章结束。
- **问题**：`序章大纲.txt` 中提到了"隐藏：等2分钟等 Lain 自己出来"，但该选项未在 `.ssl` 中实现。

### 2.4 序章总结
- **已实现**：走廊、房间、NAVI 桌面、楼下客厅、父亲对话、母亲对话（有缺陷）、等待结局。
- **缺失**：`02F_mom_reply_fine_endprologue.ssl`、隐藏等待选项、`.ssl` 条件分支语法支持。
- **核心断裂**：序章结局（停火/关机/拿牛奶）到第一章闹钟之间**没有任何叙事过渡**。

---

## 三、第一章（Chapter 1）详细走线

### 3.1 起点：闹钟（SCENE_03_CHAPTER_ONE_INTRO）
- **地点**：iwakura_lains_room
- **时间**：逻辑上应为第4天早上（但无明确文本说明）。
- **事件**：闹钟响。Lain 想起昨天把手机借给姐姐了。NAVI 提示手机事宜。
- **问题**：从序章的"停火"/"关机"/"幽灵低语"到"闹钟响"，**中间发生了什么完全没有交代**。Lain 是睡着了？昏迷了？还是"停火"意味着另一意识暂时接管了身体？玩家一无所知。

**唯一选择**：`TEXT_CHOICE_TALK_TO_SISTER` → `talk_to_sister`
- **致命缺陷**：`executor.c` 的 `story_transition_table` 中**不存在 `talk_to_sister` action_id**。代码中有 `talk_to_sister_cold`、`talk_to_sister_curious`、`talk_to_sister_default`，但没有任何逻辑根据 `sister_mood` 路由到对应场景。因此**姐姐场景实际上无法被正确触发**。

### 3.2 与姐姐对话（04A/B/C）
- **地点**：iwakura_upper_hallway
- **三种版本**：
  1. **Cold**：Mika 冷漠地给手机，说"记得带好。今天父母不在。"
  2. **Curious**：Mika 好奇 Lain 为什么下楼，提到千砂邮件（"连接世界"），说父母有事，准备了面包。
  3. **Default**：Mika 问"千砂是你男朋友吗？"，然后自顾自离开。
- **共同问题**：三个场景的 `choices: []` 都为空。玩家**没有任何选择**，只能被动观看。
- **sister_mood 的设定意图**：根据序章选择设置 flag（`navi_shutdown`→curious, `get_milk`→normal, `talk_to_figure`→cold），但该 flag **没有在代码中被读取**。

**剧情推进**：无论哪个版本，Mika 都会给 Lain 手机，引出**千砂（Chisa）的神秘邮件**。

### 3.3 电车场景（06_TRAIN_SCENE）
- **状态**：`.ssl` 文件**缺失**，仅存 `.md` 草稿。
- **内容**：电车上嘈杂，Lain 被人群包围。
  - 如果 `sister_mood == curious`：千砂邮件挥之不去，头痛欲裂。可选：尝试连接/询问 NAVI/删除邮件/忍耐。
  - 如果 `sister_mood == default`：脑海中出现神秘声音"你真的认识 lain 吗？"。可选：回应/忽略。
- **问题**：该场景是第一章承上启下的关键（从"收到邮件"到"精神过载"），但**无法在游戏中加载**。

### 3.4 教室场景（07_CLASSROOM）
- **状态**：`.ssl` 文件**缺失**，仅存 `.md` 草稿。
- **内容**：Alice 关心 Lain 的脸色，提到**全班都收到了千砂的邮件**，而且每次回复都有回信。讨论千砂之死。
- **可选调查**：
  - 问老师：老师知道但不在意，认为考不上大学的学生才会做这种事，但千砂成绩很好。
  - 问代发：设备是千砂的手机，但手机已被父母作为遗物取走，不可能代发。
  - 问千砂：成绩好、难以靠近、但认真回答问题的学生。
  - 问 Alice 是否害怕：Alice 表现出对毕业的恐惧和对事件的困惑，邀请 Lain 晚上一起吃饭/通宵。
- **问题**：调查分支只是获取信息，对后续剧情走向**无实质影响**。且 `.ssl` 缺失意味着这些内容当前不可玩。

### 3.5 Cyberia 邀请与第一章结尾
- **09_CYBERIA**：Alice 介绍 Cyberia 老板特立独行（卖牛奶咖啡，不禁止学生）。`.ssl` 缺失。
- **分支**：
  1. **10A_GO_WITH_ALICE**：一起去。Alice 开心，提议逛街买衣服。→ 第一章完 → 触发第二章冷开场。
  2. **10B_ALONE_TOUKO**：拒绝。Lain 独自回家，突然想到"冬子医生...自从上次的墨迹测试之后..."→ 第一章完。
     - **突兀问题**："冬子医生"和"墨迹测试"在**此前剧情中从未出现**，玩家没有任何上下文。
  3. **10C_ALONE_CHISA**：建议 Alice 陪朋友。Lain 独自想"千砂...你什么都没做，什么都没有说，却把我给困住了...就像魔女一样。"→ 第一章完。
- **问题**：三个分支对第二章开场**没有任何差异化影响**。第二章开头都是教室幻觉，仿佛第一章结尾从未发生。

### 3.6 第一章总结
- **已实现**：`03_chapter_one_intro.ssl`、`04a/b/c_talk_to_sister.ssl`（但无法正确路由）。
- **缺失**：`06_train_scene.ssl`、`07_classroom.ssl`、`08b~e.ssl`、`09_cyberia.ssl`、`10a/b/c.ssl`。这些场景的核心叙事仅存于 `.md` 草稿，未被集成进引擎。
- **核心断裂**：
  1. `sister_mood` 系统未实现路由。
  2. 电车、教室、Cyberia 等关键场景缺失。
  3. 第一章三种结尾对第二章开场**零影响**。

---

## 四、第二章（Chapter 2）详细走线

### 4.1 冷开场：千砂自杀（19_COLD_OPEN_CH2）
- **状态**：`.ssl` 文件**缺失**，仅存 `.md` 草稿。
- **文本**："让没有接触过生死的少女，见识一个生命挣扎在少女面前冷却，自杀。"
- **描写焦点**：众人的反应（逃窜或愣住），无人在意那个少女。少女内心是呼救、出神还是沉思。
- **问题**：
  1. **时间点不明**：千砂在第一章已经死了（邮件是定时发送或遗物手机发送），这里又写一次自杀。是 Lain 的回忆？幻觉？还是第二章的实时事件？
  2. **视角不明**："少女"指的是千砂还是 Lain？如果千砂已死，Lain 是否目睹了自杀？第一章没有给出 Lain 目睹自杀的信息。
  3. **叙事框架缺失**：没有提示告诉玩家这是"回忆"还是"Wired 中的投影"。

### 4.2 教室幻觉（20_CHAPTER_TWO_INTRO）
- **状态**：`.ssl` 文件**缺失**，仅存 `.md` 草稿。
- **地点**：school_classroom
- **事件**：Alice 呼唤 Lain，碰掉水杯。Lain 眼中，杯子碎片变成"支离破碎的尸体"。
- **选项**：
  1. "……我很好。" → `21B_REPLY_FINE`：Alice 摸 Lain 的头，说"像穿着小熊睡衣的小孩子"。Lain 回答"因为毛茸茸的，不扎人"。
  2. **抱紧爱丽丝** → `21A_HUG_ALICE`：Lain 拥抱 Alice，内心想"我……会不会也变成那个样子？"Alice 犹豫地拍背。
  3. "那个是……谁？" → `CH2_ASK_WHO`：后续文本未在现有文件中找到。
- **21C_ALICE_COMFORTS_LAIN**：Lain 松开后，Alice 给了一顶米色绒毛帽子。"虽然现在没有小熊杯子赔给你，但这个……应该能让你打起精神。"
- **问题**：
  1. 从第一章结尾（去 Cyberia / 独自回家）到第二天教室，**中间无任何过渡**。
  2. 如果第一章选了"和 Alice 去 Cyberia"，为什么第二章开头 Alice 还在问"你怎么了"，仿佛昨晚什么都没发生？

### 4.3 Cyberia 闪回（22_CYBERIA_FLASHBACK）
- **状态**：`.ssl` 文件**缺失**，仅存 `.md` 草稿。
- **地点**：cyberia_club
- **事件**：Alice 问 Lain 喝什么。老板乔吉出现，说"我们是不是见过你。Lain...是你姐姐吗？"
- **分支**：
  - Lain 指自己摇头："我是第一次来哦。我在学校一直是直接回家的那个人。"
  - Alice 脸红："别说得像约会一样！"
  - 老板怀念微笑，然后突然提高音量："有请新人 Lain……" → `22B_BOSS_INVITES_LAIN_TO_SING`
- **22B_BOSS_INVITES_LAIN_TO_SING**：老板宣布请 Lain 唱歌。Alice 惊讶。舞台上有旧麦克风。
  - 选择：走上舞台 / 拒绝。
  - 提示：`examine old_mic`
- **问题**：
  1. 这是**闪回**还是**续接**？如果是闪回，应该在画面上给玩家提示（如画面扭曲、褪色、时间戳）。
  2. 如果是续接（第一章去了 Cyberia，这是当晚后续），那第二章开头的教室场景又成了第二天，但游戏没有给出"隔天"的提示。

### 4.4 第二章总结
- **已实现**：无 `.ssl` 文件，全部仅存 `.md` 草稿。
- **核心断裂**：
  1. 冷开场的时间点和叙事性质完全不明。
  2. 教室幻觉与第一章结尾脱节。
  3. Cyberia 场景的时间定位混乱（闪回？续接？）。

---

## 五、缺失场景清单（按优先级）

### P0：阻止剧情推进
| 缺失场景 ID | 对应 `.md` 文件 | 说明 |
|-------------|-----------------|------|
| `SCENE_02F_MOM_REPLY_FINE_ENDPROLOGUE` | 无 | 母亲分支A，被硬编码进 `02G` |
| `SCENE_06_TRAIN_SCENE` | `06_train_scene.md` | 电车过载，第一章核心转折 |
| `SCENE_07_CLASSROOM` | `07_classroom.md` | 教室调查，第一章主干 |
| `SCENE_08B_ASK_TEACHER` | `08b_ask_teacher.md` | 问老师 |
| `SCENE_08C_ASK_PROXY` | `08c_ask_proxy.md` | 问代发 |
| `SCENE_08D_ASK_CHISA` | `08d_ask_chisa.md` | 问千砂 |
| `SCENE_08E_ASK_ALICE_SCARED` | `08e_ask_alice_scared.md` | 问 Alice |
| `SCENE_09_CYBERIA` | `09_cyberia.md` | Cyberia 邀请 |
| `SCENE_09A_PERSUASION` | `09a_persuasion.md` | Alice 说服 |
| `SCENE_10A_GO_WITH_ALICE` | `10a_go_with_alice.md` | 一起去 |
| `SCENE_10B_ALONE_TOUKO` | `10b_alone_touko.md` | 独自想冬子 |
| `SCENE_10C_ALONE_CHISA` | `10c_alone_chisa.md` | 独自想千砂 |
| `SCENE_19_COLD_OPEN_CH2` | `19_cold_open_ch2.md` | 第二章冷开场 |
| `SCENE_20_CHAPTER_TWO_INTRO` | `20_chapter_two_intro.md` | 第二章教室 |
| `SCENE_21A_HUG_ALICE` | `21a_hug_alice.md` | 拥抱 Alice |
| `SCENE_21B_REPLY_FINE` | `21b_reply_fine.md` | 回答没事 |
| `SCENE_21C_ALICE_COMFORTS_LAIN` | `21c_alice_comforts_lain.md` | Alice 给帽子 |
| `SCENE_22_CYBERIA_FLASHBACK` | `22_cyberia_flashback.md` | Cyberia 闪回 |
| `SCENE_22B_BOSS_INVITES_LAIN_TO_SING` | `22b_boss_invites_lain_to_sing.md` | 老板邀唱 |
| `SCENE_22D_REPLY_IS_ME` | `22d_reply_is_me.md` | 指自己 |

> 注：以上 20+ 个场景在 `story_transition_table` 中可能被引用，但 `data/scenes/story/` 中没有对应的 `.ssl` 文件。

### P1：逻辑修复
| 问题 | 修复方向 |
|------|----------|
| `talk_to_sister` action_id 未注册 | 在 `executor.c` 中根据 `sister_mood` flag 路由到 04A/B/C |
| `02G` 硬编码双分支 | 拆分为 `02F` 和 `02G` 两个独立 `.ssl`，用 `target_scene` 跳转 |
| 序章→第一章过渡 | 添加黑屏/日期变化/系统提示，解释"停火"后的时间流逝 |
| 冬子医生突兀出现 | 在序章或第一章前半添加至少一次铺垫（如提及"下周要去看医生"） |
| 第二章冷开场定位 | 添加叙事框架提示（如`[记忆碎片加载中]`或`[Wired投影]`） |

---

## 六、角色动线梳理

### Lain 的物理动线
```
序章：二楼走廊 → [房间 / 楼下客厅] → 序章结束
第一章：自己房间 → 二楼走廊（遇Mika）→ 电车 → 学校教室 → [Cyberia / 回家]
第二章：学校教室 → [Cyberia闪回]
```
- **问题**：动线过于简单，缺乏"世界探索感"。大量地图地点（涩谷、新宿、秋叶原等）在 `data/scenes/map/` 中有 `.ssl` 定义，但主线剧情从未经过。

### 千砂（Chisa）的信息动线
```
序章：无直接出场，仅通过"模糊的字"、"束"暗示
第一章：邮件（"连接世界"）→ 同学口述（成绩好、认真、开朗）→ 已死（跳楼）
第二章：冷开场（自杀现场）→ 教室幻觉中的隐喻（杯子=尸体）
```
- **问题**：信息全部为**二手转述**，玩家从未直接接触 Chisa 的深层动机（卡夫卡式压力、名字恐惧、深海/航天隐喻）。这些丰富的 lore 目前只是开发者的"内部文档"。

---

## 七、结论

### 能连成线的部分
1. **序章内部**：走廊 → 房间/楼下 → 结局，基本通顺（除母亲分支硬编码外）。
2. **第一章概念**：闹钟 → 姐姐给手机 → 电车 → 教室 → Cyberia，**概念上的逻辑链条存在**。
3. **第二章概念**：自杀冷开场 → 教室幻觉 → Cyberia 闪回，**氛围上的递进存在**。

### 无法连成线的部分
1. **物理缺失**：32 个 `.ssl` 场景缺失，导致第一章中段到第二章**完全不可玩**。
2. **逻辑断裂**：序章结局到第一章闹钟之间、第一章结尾到第二章开头之间，**缺乏叙事粘合剂**。
3. **系统未实现**：`sister_mood` 路由、`talk_to_sister` action_id、`.ssl` 条件分支语法，导致已写的剧情**无法被正确调用**。
4. **时间线模糊**：千砂自杀发生在何时？Lain 是否目睹？第二章的 Cyberia 是回忆还是续接？**没有统一的叙事时间轴**。

### 下一步建议
1. **先修工程**：补全 P0 缺失的 `.ssl` 文件，修复 `sister_mood` 路由和 `talk_to_sister` action_id。
2. **再写过渡**：为序章→第一章、第一章→第二章添加至少 1-2 个"粘合"场景（如"次日清晨"、"Wired 日志记录"）。
3. **最后渗透**：把 Chisa 的 lore（深海、挑战者号、名字恐惧）通过**可调查物品**（手机邮件碎片、同学不同口述、Wired 中的文件）逐步释放给玩家，而不是只存在文档里。
