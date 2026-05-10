//! 命令模式（Command Pattern）
//!
//! 核心设计：Executor 不直接修改 GameState，而是返回一组 Command。
//! 主循环负责按顺序执行这些 Command。
//!
//! 好处：
//! 1. 消除递归/重入问题（on_talk 内部调 execute_action）
//! 2. 可测试：可以 assert_eq!(commands, vec![...]) 而不需要构造完整状态
//! 3. 可回放：Command 列表可以序列化、保存、回放

use crate::engine::state::GameState;

/// 一条世界修改命令
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Command {
    /// 切换场景
    TransitionTo(String),
    /// 切换玩家位置
    MoveTo(String),
    /// 设置标志
    SetFlag { key: String, value: String },
    /// 获取物品
    AcquireItem { item_id: String, quantity: i32 },
    /// 解锁命令
    UnlockCommand(String),
    /// 推进时间
    AdvanceTime(u32), // units
    /// 进入 NAVI 子系统
    EnterNavi,
    /// 进入 NAVI mini
    EnterNaviMini,
    /// 显示瞬态消息
    TransientMessage(String),
    /// 什么都不做
    NoOp,
}

/// 批量执行命令
pub fn apply_commands(commands: &[Command], gs: &mut GameState) {
    for cmd in commands {
        apply_single(cmd, gs);
    }
}

fn apply_single(cmd: &Command, gs: &mut GameState) {
    use Command::*;
    match cmd {
        TransitionTo(scene) => gs.current_scene = scene.clone(),
        MoveTo(loc) => gs.player.location = loc.clone(),
        SetFlag { key, value } => { gs.set_flag(key.clone(), value.clone()); }
        AcquireItem { item_id, quantity } => gs.add_item(item_id.clone(), *quantity),
        UnlockCommand(cmd) => gs.unlock_command(cmd.clone()),
        AdvanceTime(units) => {
            let decoded = crate::engine::time::decode(gs.time_of_day);
            gs.time_of_day = crate::engine::time::encode(decoded.data + units);
        }
        EnterNavi | EnterNaviMini => {
            // 子系统切换由主循环处理
        }
        TransientMessage(_) => {
            // 瞬态消息由渲染层处理
        }
        NoOp => {}
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_apply_transition() {
        let mut gs = GameState::default();
        apply_commands(&[Command::TransitionTo("SCENE_01".into())], &mut gs);
        assert_eq!(gs.current_scene, "SCENE_01");
    }

    #[test]
    fn test_apply_set_flag_and_item() {
        let mut gs = GameState::default();
        let cmds = vec![
            Command::SetFlag { key: "sister_mood".into(), value: "cold".into() },
            Command::AcquireItem { item_id: "milk".into(), quantity: 1 },
        ];
        apply_commands(&cmds, &mut gs);
        assert_eq!(gs.get_flag("sister_mood"), Some("cold"));
        assert_eq!(gs.player.inventory, vec![("milk".to_string(), 1)]);
    }

    #[test]
    fn test_command_equality() {
        // 可以直接比较命令列表，无需构造完整状态
        let a = Command::SetFlag { key: "k".into(), value: "v".into() };
        let b = Command::SetFlag { key: "k".into(), value: "v".into() };
        assert_eq!(a, b);
    }
}
