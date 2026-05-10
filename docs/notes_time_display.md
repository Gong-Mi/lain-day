# 时间显示分析笔记

> 2026-05-10

## 现状

游戏**有**时间显示，但**不是常驻状态栏**。

实现位置：`src/render.rs:193-207`

```
[20:00] [U:3916800]
========================================
Location: iwakura_upper_hallway
========================================
```

- `[HH:MM]` 黄色，提取自 ECC 解码后的 24-bit 时间数据
- `[U:xxxxxx]` 灰色，原始时间单位值
- 双比特错误时显示红色 `[##:##]`
- **只在场景切换时打印一次**，平时命令行输入时不显示

## CMakeLists.txt 相关发现

### 编译时角色开关（生死选项）
```cmake
option(CHARACTER_ALICE_ALIVE "Include Alice" ON)
option(CHARACTER_CHISA_ALIVE "Include Chisa" ON)
option(CHARACTER_FATHER_ALIVE "Include Father" ON)
option(CHARACTER_FUYUKO_MIRA_ALIVE "Include Fuyuko Mira (Doctor)" ON)
option(CHARACTER_LAINS_SISTER_MIRA_ALIVE "Include Mira (Lain's Sister)" ON)
```

→ 这意味着角色存在与否是**编译时决定**的，不是运行时 flag。

### 渲染效果开关
```cmake
option(ENABLE_TYPEWRITER_EFFECT "打字机效果" OFF)
option(ENABLE_CLEAR_SCREEN "每次场景渲染清屏" ON)
option(ENABLE_FULL_GRAPHICS "ANSI/RGB 全屏图像查看器" ON)
```

→ `ENABLE_TYPEWRITER_EFFECT` 和 `is_takeover` 是两套不同的文本显示控制。

### Logo 生成
```cmake
set(LOGO_IMAGE_FILE "${PROJECT_SOURCE_DIR}/experiments/ansi_renderer/e.jpeg")
```

→ 启动 Logo 从 `e.jpeg` 自动生成 `logo_raw_data.h`。

## 问题

1. **时间不是常驻显示** —— 只在场景切换时出现
2. **终端文本流限制** —— 要做"手机左上角"式状态栏，需要 ANSI escape sequences 固定顶行，会干扰正常滚动
3. **ECC glitch 未触发叙事** —— 双比特错误只变红色 `[##:##]`，没有进入设计文档提到的"时间故障叙事场景"

## 选项

| 方案 | 描述 | 复杂度 |
|------|------|--------|
| A 现状 | 保持场景切换时显示 | 无 |
| B 命令提示符 | `[20:00] > ` 每次输入前显示 | 低 |
| C 固定顶行 | ANSI escape 刷新顶行 | 高 |
| D 条件显示 | Wired 层显示，物理层隐藏 | 中 |

待你决定。
