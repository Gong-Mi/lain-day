/*
 * =====================================================================================
 *
 *       Filename:  project_status.h
 *
 *    Description:  This file documents the current development status of the lain-day C port.
 *                  The project now employs a **blended data-driven and code-driven design**.
 *                  While core game content like map structures, items, and general actions remain data-driven,
 *                  narrative sequences, character interactions, and complex state changes are increasingly
 *                  managed directly within the C engine code to achieve stronger narrative control and structural integrity.
 *                  该文件用于记录 lain-day C语言移植项目的当前开发状态。
 *                  项目目前采用**数据驱动与代码驱动相结合的设计**。
 *                  虽然像地图结构、物品和通用动作等核心游戏内容仍是数据驱动的，
 *                  但叙事序列、角色交互和复杂的状态变化正越来越多地直接在 C 引擎代码中管理，
 *                  以实现更强的叙事控制和结构完整性。
 *
 * =====================================================================================
 */

#ifndef PROJECT_STATUS_H
#define PROJECT_STATUS_H

// =====================================================================================
// --- 已完成功能 (COMPLETED FEATURES) ---
// =====================================================================================

//  [✓] 1. 项目结构与编译系统 (Project Scaffolding & Build System)
//      - 使用 CMake 作为构建系统 (CMakeLists.txt)。
//      - 创建了 'src/' 和 'include/' 目录结构。

//  [✓] 2. 第三方库集成 (3rd Party Library Integration)
//      - 成功集成了 cJSON (v1.7.17) 用于解析所有 .json 数据文件。

//  [✓] 3. 核心数据结构 (Core Data Structures)
//      - 在 'game_types.h' 中定义了游戏所需的所有核心数据结构。
//      - (GameState, PlayerState, Location, Item, Action, StoryScene, etc.)

//  [✓] 4. 数据加载模块 (Data Loading Module)
//      - 实现了从 'character.json' 加载玩家和初始游戏状态。
//      - 实现了从 'items.json' 加载所有物品定义。
//      - 实现了从 'actions.json' 加载所有动作定义 (将 payload 存为原始 cJSON 对象以保证灵活性)。

//  [✓] 5. 地图系统加载 (Map System Loading)
//      - 实现了遍历 'map/' 目录并加载每个地点的详细信息 (name, description, poi, connections)。
//      - 能够处理不同地点 .json 文件中的数据结构差异 (例如 poi 是对象数组或字符串数组)。

//  [✓] 6. 故事解析器 (Story Parser)
//      - 实现了 'story_parser.c'，能够读取 .md 格式的故事文件。
//      - 能够解析 YAML-like front-matter (例如 'location: ...')。
//      - 能够解析并分离叙事文本和玩家选项 (例如 '- [Text](action:id)')。

//  [✓] 7. 游戏主循环 (Main Game Loop)
//      - 在 'main.c' 中实现了核心游戏循环。
//      - 能够根据当前状态加载和渲染场景。
//      - 能够接收并区分玩家输入的“数字选项”和“文本命令”。
//      - 实现了基于 'scene_changed' 标志的智能渲染，避免在执行信息展示类命令后重复刷新屏幕。

//  [✓] 8. 动作与命令执行器 (Action & Command Executor)
//      - 在 'executor.c' 中实现了执行器框架。
//      - 已实现基础的动作类型，如 'story_change', 'location_change', 'acquire_item'。
//      - 已实现基础的命令，如 'arls', 'inventory', 'help'。

//  [✓] 9. 状态保存 (State Saving)
//      - 实现了 'save_game_state' 函数，可在游戏退出时将玩家位置、物品、命令等状态写回 'character.json'。

//  [✓] 10. 角色生命周期控制 (Character Life Cycle Control)
//      - 通过 CMake 选项实现了角色的编译时“生死”控制 (CHARACTER_NAME_ALIVE)。
//      - 采用“条件代码/数据”方案 (Option B)，允许在编译时根据选项决定是否包含角色相关内容。


// =====================================================================================
// --- 未完成功能 (UNIMPLEMENTED FEATURES / TODO) ---
// =====================================================================================

//  [ ] 1. 完整的动作执行逻辑 (Full Action Execution Logic)
//      - 当前的 'executor.c' 需要扩展以支持 'actions.json' 中的所有动作类型，特别是：
//        - `*_and_set_flags`: 所有带标志设置的复合动作。
//        - `conditional_story_change`: 实现条件判断逻辑。
//        - `enter_story` / `exit_story`: 实现支线故事的进入和返回。
//        - `toggle_protocol`: 实现对 'network_status' 的修改。

//  [ ] 2. 完整的游戏内标志系统 (Generic Game Flag System)
//      - 需要一个通用的系统来管理 `GameState` 中的任意标志（例如，一个哈希表或链表）。
//      - 当前 `set_flags` 动作的实现非常初级，仅为占位符。

//  [ ] 3. 高级模拟终端命令 (Advanced Terminal Commands)
//      - `examine <poi>`: 查看兴趣点的详细描述。
//      - `mail`: 查看邮件。
//      - `ls`, `cd`, `cat`: 与 `world/` 模拟文件系统交互的命令。

//  [ ] 4. UI/渲染增强 (UI/Rendering Enhancements)
//      - 实现 'character.json' 中定义的 `typewriter_delay` 打字机效果。
//      - [进行中] 为说话人姓名实现 ANSI 颜色高亮功能。
//      - [ ] 实现 ANSI Block Art 的渲染功能。


// =====================================================================================
// --- 需要的测试 (NEEDED TESTS) ---
// =====================================================================================

//  [ ] 1. 单元测试 (Unit Tests)
//      - 为每个模块 (data_loader, map_loader, story_parser, executor) 编写独立的测试用例。

//  [ ] 2. 健壮性测试 (Robustness / Edge Case Testing)
//      - 测试当数据文件 (JSON, MD) 格式损坏或内容不符合预期时程序的反应。
//      - 测试非常规的用户输入 (例如，超长字符串、特殊字符)。

//  [ ] 3. 内存审计 (Full Memory Audit)
//      - 在支持的环境中使用 Valgrind 或 ASan (AddressSanitizer) 进行全面的内存泄漏和错误检查。

//  [ ] 4. 完整游戏流程测试 (Full Gameplay-Through)
//      - 在所有功能实现后，进行从头到尾的完整游戏测试，以确保所有分支和逻辑都正确无误。


#endif // PROJECT_STATUS_H
