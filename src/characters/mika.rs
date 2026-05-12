//! Mika AI 模块 —— 行动轴（Action Axis）实现
//!
//! 四轴驱动：时间轴 + 空间轴 + 状态轴 + 响应轴
//! - 时间轴：日程表驱动位置变化
//! - 空间轴：房间访问控制
//! - 状态轴：Sanity 变化触发条件（核心新增）
//! - 响应轴：对 Lain 行为的跟踪与反应（第二阶段实现）

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

/// Mika 模块 —— 行动轴核心数据结构
#[derive(Debug, Clone, PartialEq, serde::Serialize, serde::Deserialize)]
pub struct MikaModule {
    // === 时间轴 + 空间轴（C 版已有）===
    pub current_location_id: String,
    pub is_manually_positioned: bool,
    pub sanity_level: MikaSanity,

    // === 状态轴（新增）===
    /// Normal → Irritated 累计点数（阈值：3）
    pub irritation_points: i32,
    /// Irritated → Paranoid 累计点数（阈值：4）
    pub paranoia_points: i32,
    /// 单次循环内是否已进入 Broken（不可逆）
    pub broken_triggered: bool,

    // === 响应轴（新增）===
    /// 最后看到 Lain 的地点
    pub last_seen_lain_location: Option<String>,
    /// 最后看到 Lain 的时间（ECC units）
    pub last_seen_lain_time: u32,
    /// 本日是否已向父母报告过
    pub has_reported_to_parents: bool,

    // === 循环轴（新增）===
    /// 跨循环：历史 Broken 次数
    pub broken_count: u32,
    /// 跨循环：记忆碎片（Paranoid 时说出）
    pub loop_memory_fragments: Vec<String>,

    // === 剧情状态（Day 3 设定）===
    /// Mika 是否持有 Lain 的手机（Day 3 中午借走）
    pub has_lain_phone: bool,
}

impl Default for MikaModule {
    fn default() -> Self {
        Self {
            // 时间轴 + 空间轴
            current_location_id: "UNKNOWN_LOCATION".to_string(),
            is_manually_positioned: false,
            sanity_level: MikaSanity::Normal,
            // 状态轴
            // Day 3: 整个高中部被牵连(+2) + 手机被没收(+1) = 接近阈值
            // 注意：集体措施降低了个人愤怒，但增加了集体焦虑
            irritation_points: 2,
            paranoia_points: 1, // 同校有人自杀
            broken_triggered: false,
            // 响应轴
            last_seen_lain_location: None,
            last_seen_lain_time: 0,
            has_reported_to_parents: false,
            // 循环轴
            broken_count: 0,
            loop_memory_fragments: Vec::new(),
            // 剧情状态
            has_lain_phone: true, // Day 3 中午借走，晚上 20:00 时仍持有
        }
    }
}

/// Lain 的行为类型（用于状态轴更新）
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LainAction {
    /// 深夜使用 NAVI（23:00+）
    LateNightNavi,
    /// 连续不出门（每天 +1，由外部计数）
    StayHome,
    /// 进入 Mika 房间（无钥匙）
    EnterMikaRoom,
    /// 和 Dad 单独谈话
    TalkToDad,
    /// 在浴室超过 30 分钟
    LongBath,
    /// 获得"石头"类物品
    AcquireStone,
    /// 打破家中物品
    BreakItem,
    /// 和 Mika 对话时选择沉默
    SilentToMika,
    /// 循环回绕（Mika 感知到 déjà vu）
    LoopRewind,
    /// 使用"交换形式"（千砂语言）
    UseExchangeForm,
    /// 千砂邮件被 Mika 看到
    ChisaMailSeen,
    /// Lain 在循环中"消失"后重现
    LainReappear,
    /// 连续正常作息（可恢复 sanity）
    NormalRoutine,
    /// 送给 Mika 礼物/道歉
    GiftToMika,
    /// 父母介入家庭谈话
    FamilyIntervention,
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

    // =============================================================================
    // 状态轴：Sanity 变化（核心新增）
    // =============================================================================

    /// 记录看到 Lain（用于响应轴）
    pub fn record_lain_sighting(&mut self, location: &str, time: u32) {
        self.last_seen_lain_location = Some(location.to_string());
        self.last_seen_lain_time = time;
    }

    /// 根据 Lain 的行为更新 Sanity
    /// 
    /// 变化规则：
    /// - Normal → Irritated: irritation_points >= 3
    /// - Irritated → Paranoid: paranoia_points >= 4
    /// - Paranoid → Broken: broken_triggered（单次事件触发）
    /// - 恢复：特定行为可减少点数
    pub fn update_sanity(&mut self, action: LainAction, _current_time: u32) -> Option<MikaSanity> {
        let (delta_irritation, delta_paranoia, delta_broken) = Self::action_weights(action);

        // 应用变化
        self.irritation_points = (self.irritation_points + delta_irritation).max(0);
        self.paranoia_points = (self.paranoia_points + delta_paranoia).max(0);

        // Broken 触发检查
        if delta_broken > 0 {
            self.broken_triggered = true;
        }

        // 状态升级检查（单向，恢复需单独处理）
        let old_sanity = self.sanity_level;

        match self.sanity_level {
            MikaSanity::Normal => {
                if self.irritation_points >= 3 {
                    self.sanity_level = MikaSanity::Irritated;
                }
            }
            MikaSanity::Irritated => {
                if self.paranoia_points >= 4 {
                    self.sanity_level = MikaSanity::Paranoid;
                }
                // Irritated 可因积极行为恢复 Normal
                if self.irritation_points <= 0 {
                    self.sanity_level = MikaSanity::Normal;
                }
            }
            MikaSanity::Paranoid => {
                if self.broken_triggered {
                    self.sanity_level = MikaSanity::Broken;
                    self.broken_count += 1;
                }
                // Paranoid 可因积极行为恢复 Irritated
                if self.paranoia_points <= 0 {
                    self.sanity_level = MikaSanity::Irritated;
                }
            }
            MikaSanity::Broken => {
                // Broken 在单次循环内不可逆
            }
        }

        if self.sanity_level != old_sanity {
            Some(self.sanity_level)
        } else {
            None
        }
    }

    /// 行为权重表
    fn action_weights(action: LainAction) -> (i32, i32, i32) {
        // 返回：(irritation_delta, paranoia_delta, broken_trigger)
        match action {
            // Normal → Irritated 触发器
            LainAction::LateNightNavi => (2, 0, 0),
            LainAction::StayHome => (1, 0, 0),
            LainAction::EnterMikaRoom => (3, 0, 0),
            LainAction::TalkToDad => (1, 0, 0),
            LainAction::LongBath => (1, 0, 0),

            // Irritated → Paranoid 触发器
            LainAction::AcquireStone => (0, 2, 0),
            LainAction::BreakItem => (0, 3, 0),
            LainAction::SilentToMika => (0, 1, 0),
            LainAction::LoopRewind => (0, 2, 0),

            // Paranoid → Broken 触发器
            LainAction::UseExchangeForm => (0, 0, 5),
            LainAction::ChisaMailSeen => (0, 0, 3),
            LainAction::LainReappear => (0, 0, 4),

            // 恢复行为
            LainAction::NormalRoutine => (-1, 0, 0),
            LainAction::GiftToMika => (-2, -2, 0),
            LainAction::FamilyIntervention => (0, -1, 0),
        }
    }

    /// 生成向父母的报告（信息轴）
    /// 
    /// 返回值：None = 无报告，Some((内容, 扭曲程度))
    pub fn generate_report(&self, _lain_recent_actions: &[LainAction]) -> Option<String> {
        if self.has_reported_to_parents {
            return None; // 本日已报告
        }

        let report = match self.sanity_level {
            MikaSanity::Normal => {
                // 如实描述
                Some("Lain 今天很正常。".to_string())
            }
            MikaSanity::Irritated => {
                // 略带负面
                Some("Lain 最近总是躲在房间里。".to_string())
            }
            MikaSanity::Paranoid => {
                // 焦虑/夸张
                Some("Lain 半夜在说话，不知道在和谁。".to_string())
            }
            MikaSanity::Broken => {
                // 不报告
                None
            }
        };

        report
    }

    /// 标记已报告（每日重置由外部调用者处理）
    pub fn mark_reported(&mut self) {
        self.has_reported_to_parents = true;
    }

    /// 循环回绕时重置（保留跨循环数据）
    pub fn on_loop_rewind(&mut self) {
        self.sanity_level = MikaSanity::Normal;
        self.irritation_points = 0;
        self.paranoia_points = 0;
        self.broken_triggered = false;
        self.has_reported_to_parents = false;
        self.last_seen_lain_location = None;
        self.last_seen_lain_time = 0;
        // loop_memory_fragments 和 broken_count 保留
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

    // =============================================================================
    // 状态轴测试
    // =============================================================================

    #[test]
    fn test_sanity_normal_to_irritated() {
        let mut mika = MikaModule::new();
        // 重置为 Normal 初始状态（不依赖默认值）
        mika.irritation_points = 0;
        mika.paranoia_points = 0;
        assert_eq!(mika.sanity_level, MikaSanity::Normal);

        // 深夜用 NAVI (+2)
        mika.update_sanity(LainAction::LateNightNavi, 0);
        assert_eq!(mika.sanity_level, MikaSanity::Normal);
        assert_eq!(mika.irritation_points, 2);

        // 再深夜用 NAVI (+2)，累计 4 >= 3 → Irritated
        let changed = mika.update_sanity(LainAction::LateNightNavi, 0);
        assert_eq!(changed, Some(MikaSanity::Irritated));
        assert_eq!(mika.sanity_level, MikaSanity::Irritated);
    }

    #[test]
    fn test_sanity_irritated_to_paranoia() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Irritated;
        mika.irritation_points = 3;

        // 获得石头 (+2 paranoia)
        mika.update_sanity(LainAction::AcquireStone, 0);
        assert_eq!(mika.sanity_level, MikaSanity::Irritated);

        // 打破物品 (+3 paranoia)，累计 5 >= 4 → Paranoid
        let changed = mika.update_sanity(LainAction::BreakItem, 0);
        assert_eq!(changed, Some(MikaSanity::Paranoid));
        assert_eq!(mika.sanity_level, MikaSanity::Paranoid);
    }

    #[test]
    fn test_sanity_paranoia_to_broken() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Paranoid;
        mika.paranoia_points = 4;

        // 使用交换形式 (+5 broken_trigger)
        let changed = mika.update_sanity(LainAction::UseExchangeForm, 0);
        assert_eq!(changed, Some(MikaSanity::Broken));
        assert_eq!(mika.sanity_level, MikaSanity::Broken);
        assert_eq!(mika.broken_count, 1);
    }

    #[test]
    fn test_sanity_recovery_normal_routine() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Irritated;
        mika.irritation_points = 3;

        // 连续正常作息 (-1 irritation)
        mika.update_sanity(LainAction::NormalRoutine, 0);
        assert_eq!(mika.irritation_points, 2);

        // 再正常作息
        mika.update_sanity(LainAction::NormalRoutine, 0);
        assert_eq!(mika.irritation_points, 1);

        // 再正常作息，irritation <= 0 → 恢复 Normal
        let changed = mika.update_sanity(LainAction::NormalRoutine, 0);
        assert_eq!(changed, Some(MikaSanity::Normal));
        assert_eq!(mika.sanity_level, MikaSanity::Normal);
    }

    #[test]
    fn test_sanity_gift_recovery() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Paranoid;
        mika.irritation_points = 2;
        mika.paranoia_points = 2; // 初始 2，减 2 后 = 0 → 触发降级

        // 送礼物 (-2 irritation, -2 paranoia)
        let changed = mika.update_sanity(LainAction::GiftToMika, 0);
        assert_eq!(changed, Some(MikaSanity::Irritated));
        assert_eq!(mika.sanity_level, MikaSanity::Irritated);
        assert_eq!(mika.irritation_points, 0);
        assert_eq!(mika.paranoia_points, 0);
    }

    #[test]
    fn test_broken_is_irreversible_in_loop() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Broken;

        // 即使送礼物也无法恢复
        let changed = mika.update_sanity(LainAction::GiftToMika, 0);
        assert_eq!(changed, None);
        assert_eq!(mika.sanity_level, MikaSanity::Broken);
    }

    #[test]
    fn test_loop_rewind_resets_but_keeps_cross_loop_data() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Paranoid;
        mika.irritation_points = 3;
        mika.paranoia_points = 5;
        mika.broken_count = 2;
        mika.loop_memory_fragments.push("你昨晚也在".to_string());

        mika.on_loop_rewind();

        // 重置
        assert_eq!(mika.sanity_level, MikaSanity::Normal);
        assert_eq!(mika.irritation_points, 0);
        assert_eq!(mika.paranoia_points, 0);

        // 保留
        assert_eq!(mika.broken_count, 2);
        assert_eq!(mika.loop_memory_fragments.len(), 1);
    }

    // =============================================================================
    // 信息轴测试
    // =============================================================================

    #[test]
    fn test_report_normal() {
        let mika = MikaModule::new();
        let report = mika.generate_report(&[]);
        assert_eq!(report, Some("Lain 今天很正常。".to_string()));
    }

    #[test]
    fn test_report_irritated() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Irritated;
        let report = mika.generate_report(&[]);
        assert_eq!(report, Some("Lain 最近总是躲在房间里。".to_string()));
    }

    #[test]
    fn test_report_paranoia() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Paranoid;
        let report = mika.generate_report(&[]);
        assert_eq!(report, Some("Lain 半夜在说话，不知道在和谁。".to_string()));
    }

    #[test]
    fn test_report_broken_no_report() {
        let mut mika = MikaModule::new();
        mika.sanity_level = MikaSanity::Broken;
        let report = mika.generate_report(&[]);
        assert_eq!(report, None);
    }

    #[test]
    fn test_report_once_per_day() {
        let mut mika = MikaModule::new();
        assert!(mika.generate_report(&[]).is_some());
        mika.mark_reported();
        assert!(mika.generate_report(&[]).is_none());
    }
}
