//! 游戏状态 —— 纯数据结构，无行为
//!
//! 关键设计：所有字段都是 pub，但修改权交给上层（Engine/Executor）。
//! 测试时可以 Clone 一份状态，执行命令后再 assert_eq!。

use std::collections::HashMap;

use crate::characters::mika::MikaModule;

/// 人格权限位掩码（RWX 模型）
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct PersonaPermissions(pub u8);

impl PersonaPermissions {
    pub const LAIN_READ: u8 = 1 << 0;
    pub const LAIN_WRITE: u8 = 1 << 1;
    pub const LAIN_EXEC: u8 = 1 << 2;
    pub const SHU_READ: u8 = 1 << 3;
    pub const SHU_WRITE: u8 = 1 << 4;
    pub const SHU_EXEC: u8 = 1 << 5;
    pub const SYSTEM_OV: u8 = 1 << 6;
    pub const GLITCH: u8 = 1 << 7;

    pub fn has(&self, mask: u8) -> bool {
        self.0 & mask == mask
    }

    pub fn set(&mut self, mask: u8) {
        self.0 |= mask;
    }
}

/// 玩家状态
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct PlayerState {
    pub location: String,
    pub credit_level: i32,
    pub inventory: Vec<(String, i32)>, // (item_id, quantity)
    pub unlocked_commands: Vec<String>,
    pub persona_permissions: PersonaPermissions,
}

/// 游戏主状态 —— 可以被 Clone 用于测试快照
#[derive(Debug, Clone, PartialEq)]
pub struct GameState {
    pub player: PlayerState,
    pub current_scene: String,
    pub time_of_day: u32, // ECC 编码的时间
    pub flags: HashMap<String, String>,
    pub typewriter_delay: f32,
    pub doll_state_lain_room: i8,
    pub doll_state_mika_room: i8,
    pub mika_sanity: i32,
    pub session_name: String,
    pub mika: MikaModule,
}

impl Default for GameState {
    fn default() -> Self {
        Self {
            player: PlayerState::default(),
            current_scene: "SCENE_00_ENTRY".to_string(),
            time_of_day: crate::engine::time::encode(8 * 3600 * 16), // 第0天早上8点
            flags: HashMap::new(),
            typewriter_delay: 0.04,
            doll_state_lain_room: 0,
            doll_state_mika_room: 0,
            mika_sanity: 0,
            session_name: "default".to_string(),
            mika: MikaModule::new(),
        }
    }
}

impl GameState {
    /// 快捷方法：设置标志
    pub fn set_flag(&mut self, key: impl Into<String>, value: impl Into<String>) {
        self.flags.insert(key.into(), value.into());
    }

    /// 快捷方法：获取标志
    pub fn get_flag(&self, key: &str) -> Option<&str> {
        self.flags.get(key).map(|s| s.as_str())
    }

    /// 快捷方法：添加物品
    pub fn add_item(&mut self, item_id: impl Into<String>, qty: i32) {
        let id = item_id.into();
        for (name, q) in &mut self.player.inventory {
            if *name == id {
                *q += qty;
                return;
            }
        }
        self.player.inventory.push((id, qty));
    }

    /// 快捷方法：解锁命令
    pub fn unlock_command(&mut self, cmd: impl Into<String>) {
        let c = cmd.into();
        if !self.player.unlocked_commands.contains(&c) {
            self.player.unlocked_commands.push(c);
        }
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_default_state() {
        let gs = GameState::default();
        assert_eq!(gs.current_scene, "SCENE_00_ENTRY");
        assert!(gs.player.inventory.is_empty());
    }

    #[test]
    fn test_flag_operations() {
        let mut gs = GameState::default();
        gs.set_flag("sister_mood", "cold");
        assert_eq!(gs.get_flag("sister_mood"), Some("cold"));
        gs.set_flag("sister_mood", "normal");
        assert_eq!(gs.get_flag("sister_mood"), Some("normal"));
    }

    #[test]
    fn test_inventory() {
        let mut gs = GameState::default();
        gs.add_item("milk", 1);
        gs.add_item("milk", 1);
        gs.add_item("key", 1);
        assert_eq!(gs.player.inventory.len(), 2);
        assert_eq!(gs.player.inventory[0], ("milk".to_string(), 2));
    }

    #[test]
    fn test_permissions() {
        let mut perms = PersonaPermissions::default();
        assert!(!perms.has(PersonaPermissions::LAIN_EXEC));
        perms.set(PersonaPermissions::LAIN_EXEC);
        assert!(perms.has(PersonaPermissions::LAIN_EXEC));
    }

    /// 关键测试：状态可以被 Clone，用于"执行前/后"对比
    #[test]
    fn test_state_snapshot() {
        let mut gs = GameState::default();
        let before = gs.clone();
        gs.set_flag("test", "1");
        assert_ne!(before.flags, gs.flags);
    }
}
