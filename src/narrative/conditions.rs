//! 条件判断系统 —— 纯函数，无副作用，极易测试

use serde::Deserialize;
use crate::engine::state::GameState;
use crate::engine::time;

#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct Condition {
    #[serde(default)]
    pub requires_flag: Option<String>,
    #[serde(default)]
    pub flag_value: Option<String>,
    #[serde(default = "default_neg1")]
    pub min_day: i32,
    #[serde(default = "default_neg1")]
    pub max_day: i32,
    #[serde(default = "default_neg1")]
    pub exact_day: i32,
    #[serde(default = "default_neg1")]
    pub hour_start: i32,
    #[serde(default = "default_neg1")]
    pub hour_end: i32,
    #[serde(default)]
    pub permission_mask: u8,
}

fn default_neg1() -> i32 { -1 }

impl Default for Condition {
    fn default() -> Self {
        Self {
            requires_flag: None,
            flag_value: None,
            min_day: -1,
            max_day: -1,
            exact_day: -1,
            hour_start: -1,
            hour_end: -1,
            permission_mask: 0,
        }
    }
}

impl Condition {
    /// 检查单个条件是否满足
    pub fn check(&self, gs: &GameState) -> bool {
        // 时间条件
        let decoded = time::decode(gs.time_of_day);
        let (current_day, current_hour) = {
            let total_seconds = decoded.data / 16;
            let total_minutes = total_seconds / 60;
            let hours = (total_minutes / 60) % 24;
            let days = total_minutes / (24 * 60);
            (days as i32, hours as i32)
        };

        if self.exact_day >= 0 && current_day != self.exact_day {
            return false;
        }
        if self.min_day >= 0 && current_day < self.min_day {
            return false;
        }
        if self.max_day >= 0 && current_day > self.max_day {
            return false;
        }
        if self.hour_start >= 0 && current_hour < self.hour_start {
            return false;
        }
        if self.hour_end >= 0 && current_hour > self.hour_end {
            return false;
        }

        // 权限条件
        if self.permission_mask != 0 {
            if !gs.player.persona_permissions.has(self.permission_mask) {
                return false;
            }
        }

        // 标志条件
        if let Some(ref flag_name) = self.requires_flag {
            let current = gs.get_flag(flag_name);
            if let Some(ref required) = self.flag_value {
                match current {
                    Some(v) if v == required => {}
                    _ => return false,
                }
            } else {
                // 只需要 flag 被设置（不为 "0" 或不存在）
                match current {
                    Some("0") | None => return false,
                    _ => {}
                }
            }
        }

        true
    }
}

/// 检查条件列表（AND 逻辑：全部满足才算通过）
pub fn check_all(conditions: &[Condition], gs: &GameState) -> bool {
    conditions.iter().all(|c| c.check(gs))
}

// =============================================================================
// 测试 —— 纯函数条件判断是最容易测试的部分
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::engine::state::{GameState, PersonaPermissions};

    fn make_state_with_flag(key: &str, value: &str) -> GameState {
        let mut gs = GameState::default();
        gs.set_flag(key, value);
        gs
    }

    #[test]
    fn test_empty_condition_always_true() {
        let gs = GameState::default();
        let cond = Condition::default();
        assert!(cond.check(&gs));
    }

    #[test]
    fn test_flag_exact_match() {
        let gs = make_state_with_flag("sister_mood", "cold");
        let cond = Condition {
            requires_flag: Some("sister_mood".into()),
            flag_value: Some("cold".into()),
            ..Default::default()
        };
        assert!(cond.check(&gs));

        let cond2 = Condition {
            requires_flag: Some("sister_mood".into()),
            flag_value: Some("normal".into()),
            ..Default::default()
        };
        assert!(!cond2.check(&gs));
    }

    #[test]
    fn test_flag_exists() {
        let mut gs = GameState::default();
        // flag 未设置
        let cond = Condition {
            requires_flag: Some("door_opened".into()),
            ..Default::default()
        };
        assert!(!cond.check(&gs));

        // flag 设置为 "0" 算未设置
        gs.set_flag("door_opened", "0");
        assert!(!cond.check(&gs));

        // flag 设置为非零值
        gs.set_flag("door_opened", "1");
        assert!(cond.check(&gs));
    }

    #[test]
    fn test_permission_mask() {
        let mut gs = GameState::default();
        let cond = Condition {
            permission_mask: PersonaPermissions::LAIN_EXEC,
            ..Default::default()
        };
        assert!(!cond.check(&gs));

        gs.player.persona_permissions.set(PersonaPermissions::LAIN_EXEC);
        assert!(cond.check(&gs));
    }

    #[test]
    fn test_hour_range() {
        let mut gs = GameState::default();
        // 明确设为早上8点（不依赖默认值）
        gs.time_of_day = time::encode(8 * 3600 * 16);

        let cond = Condition {
            hour_start: 20,
            hour_end: 23,
            ..Default::default()
        };
        assert!(!cond.check(&gs)); // 8点不在 20-23 范围内

        // 把时间调到晚上21点 (21 * 3600 * 16 units)
        gs.time_of_day = time::encode(21 * 3600 * 16);
        assert!(cond.check(&gs));
    }

    #[test]
    fn test_check_all_and_logic() {
        let gs = make_state_with_flag("a", "1");
        let conds = vec![
            Condition { requires_flag: Some("a".into()), flag_value: Some("1".into()), ..Default::default() },
            Condition { requires_flag: Some("b".into()), ..Default::default() }, // b 未设置 → false
        ];
        assert!(!check_all(&conds, &gs));
    }
}
