//! Mika AI 模块 —— 替代 C 版本的 characters/mika.c
//!
//! 实现 Mika 的日程表、理智度系统和房间访问控制。

use crate::engine::state::GameState;
use crate::engine::time;

/// 理智度级别
#[derive(Debug, Clone, Copy, PartialEq, Eq, serde::Serialize, serde::Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum MikaSanity {
    Normal = 0,
    Irritated = 1,
    Paranoid = 2,
    Broken = 3,
}

/// 日程表条目
#[derive(Debug, Clone, Copy)]
struct ScheduleEntry {
    start_time_units: u32,
    location_id: &'static str,
}

// 正常日程表
const NORMAL_SCHEDULE: &[ScheduleEntry] = &[
    ScheduleEntry { start_time_units: 0, location_id: "iwakura_mikas_room" },
    ScheduleEntry { start_time_units: 7 * 60 * 60 * 16, location_id: "iwakura_bathroom" },
    ScheduleEntry { start_time_units: 8 * 60 * 60 * 16, location_id: "iwakura_living_dining_kitchen" },
    ScheduleEntry { start_time_units: 9 * 60 * 60 * 16, location_id: "off_map" },
    ScheduleEntry { start_time_units: 17 * 60 * 60 * 16, location_id: "iwakura_mikas_room" },
    ScheduleEntry { start_time_units: 20 * 60 * 60 * 16, location_id: "iwakura_living_dining_kitchen" },
    ScheduleEntry { start_time_units: 22 * 60 * 60 * 16, location_id: "iwakura_mikas_room" },
];

// 偏执日程表
const PARANOID_SCHEDULE: &[ScheduleEntry] = &[
    ScheduleEntry { start_time_units: 0, location_id: "iwakura_mikas_room" },
    ScheduleEntry { start_time_units: 10 * 60 * 60 * 16, location_id: "iwakura_bathroom" },
    ScheduleEntry { start_time_units: 12 * 60 * 60 * 16, location_id: "shibuya_street" },
    ScheduleEntry { start_time_units: 16 * 60 * 60 * 16, location_id: "iwakura_lower_hallway" },
    ScheduleEntry { start_time_units: 18 * 60 * 60 * 16, location_id: "iwakura_mikas_room" },
];

/// Mika 模块
#[derive(Debug, Clone, PartialEq, serde::Serialize, serde::Deserialize)]
pub struct MikaModule {
    pub current_location_id: String,
    pub is_manually_positioned: bool,
    pub sanity_level: MikaSanity,
}

impl Default for MikaModule {
    fn default() -> Self {
        Self {
            current_location_id: "UNKNOWN_LOCATION".to_string(),
            is_manually_positioned: false,
            sanity_level: MikaSanity::Normal,
        }
    }
}

impl MikaModule {
    pub fn new() -> Self {
        Self::default()
    }

    pub fn set_sanity(&mut self, level: MikaSanity) {
        self.sanity_level = level;
    }

    pub fn move_to(&mut self, location_id: &str) {
        self.current_location_id = location_id.to_string();
        self.is_manually_positioned = true;
    }

    pub fn return_to_schedule(&mut self) {
        self.is_manually_positioned = false;
    }

    /// 检查 Mika 的房间是否可进入
    pub fn is_room_accessible(&self, game_state: &GameState) -> bool {
        // 钥匙覆盖一切
        for (item, _) in &game_state.player.inventory {
            if item == "key_mika_room" {
                return true;
            }
        }

        // 崩溃状态：永远锁着
        if self.sanity_level == MikaSanity::Broken {
            return false;
        }

        // 时间检查：17:00 - 21:00 开放
        let decoded = time::decode(game_state.time_of_day);
        let hour = (decoded.data / 16) / 3600 % 24;
        hour >= 17 && hour < 21
    }

    /// 基于时间计算日程位置
    pub fn calculate_scheduled_location(&self, time_units_in_day: u32) -> &'static str {
        if self.sanity_level == MikaSanity::Broken {
            return "iwakura_mikas_room";
        }

        let schedule = if self.sanity_level == MikaSanity::Paranoid {
            PARANOID_SCHEDULE
        } else {
            NORMAL_SCHEDULE
        };

        let mut location = "off_map";
        for entry in schedule.iter().rev() {
            if time_units_in_day >= entry.start_time_units {
                location = entry.location_id;
                break;
            }
        }
        location
    }

    /// 根据游戏状态更新位置
    pub fn update_location(&mut self, game_state: &GameState) -> &str {
        if self.is_manually_positioned {
            return &self.current_location_id;
        }

        let decoded = time::decode(game_state.time_of_day);
        let units_in_day = 24 * 60 * 60 * 16;
        let time_units_in_day = decoded.data % units_in_day;

        let new_loc = self.calculate_scheduled_location(time_units_in_day);
        self.current_location_id = new_loc.to_string();
        &self.current_location_id
    }

    /// 返回对话 action_id（由 executor 处理）
    pub fn on_talk(&self, game_state: &GameState) -> &'static str {
        if self.sanity_level == MikaSanity::Broken {
            return "talk_to_sister_broken";
        }

        match game_state.get_flag("sister_mood") {
            Some("cold") => "talk_to_sister_cold",
            Some("curious") => "talk_to_sister_curious",
            _ => "talk_to_sister_default",
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
    fn test_normal_schedule_morning() {
        let mika = MikaModule::new();
        // 8:00 应该在 living_dining_kitchen
        let loc = mika.calculate_scheduled_location(8 * 60 * 60 * 16);
        assert_eq!(loc, "iwakura_living_dining_kitchen");
    }

    #[test]
    fn test_normal_schedule_school_hours() {
        let mika = MikaModule::new();
        // 12:00 应该在 off_map（学校）
        let loc = mika.calculate_scheduled_location(12 * 60 * 60 * 16);
        assert_eq!(loc, "off_map");
    }

    #[test]
    fn test_broken_schedule() {
        let mut mika = MikaModule::new();
        mika.set_sanity(MikaSanity::Broken);
        // 任何时间都在房间
        assert_eq!(mika.calculate_scheduled_location(0), "iwakura_mikas_room");
        assert_eq!(mika.calculate_scheduled_location(12 * 60 * 60 * 16), "iwakura_mikas_room");
    }

    #[test]
    fn test_room_access_time() {
        let mika = MikaModule::new();
        let mut gs = GameState::default();

        // 18:00 (开放)
        gs.time_of_day = time::encode(18 * 3600 * 16);
        assert!(mika.is_room_accessible(&gs));

        // 22:00 (关闭)
        gs.time_of_day = time::encode(22 * 3600 * 16);
        assert!(!mika.is_room_accessible(&gs));
    }

    #[test]
    fn test_room_access_key_override() {
        let mika = MikaModule::new();
        let mut gs = GameState::default();
        gs.add_item("key_mika_room", 1);

        // 有钥匙，任何时间都开放
        gs.time_of_day = time::encode(22 * 3600 * 16);
        assert!(mika.is_room_accessible(&gs));
    }

    #[test]
    fn test_talk_dispatch() {
        let mika = MikaModule::new();
        let mut gs = GameState::default();

        assert_eq!(mika.on_talk(&gs), "talk_to_sister_default");

        gs.set_flag("sister_mood", "cold");
        assert_eq!(mika.on_talk(&gs), "talk_to_sister_cold");

        gs.set_flag("sister_mood", "curious");
        assert_eq!(mika.on_talk(&gs), "talk_to_sister_curious");
    }
}
