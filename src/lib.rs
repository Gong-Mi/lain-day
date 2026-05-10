//! lain-day Rust 引擎 —— 终端交互式小说
//!
//! 核心设计：
//! 1. 命令模式（Command Pattern）：动作不直接修改状态，而是生成命令
//! 2. 纯函数条件判断：输入状态 → 输出布尔，无副作用
//! 3. Trait 边界：模块间通过 trait 交互，测试时可 mock
//! 4. 运行时 YAML 解析：serde_yaml 直接加载 .ssl，无需代码生成
//! 5. crossterm 跨平台终端控制

pub mod app;
pub mod assets;
pub mod bgm;
pub mod characters;
pub mod engine;
pub mod narrative;
pub mod render;
pub mod systems;
pub mod world;

// 重新导出常用类型
pub use engine::state::GameState;
pub use narrative::command::Command;
pub use narrative::executor::{resolve_action, resolve_numeric_choice};
