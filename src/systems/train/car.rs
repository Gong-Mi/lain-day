//! 车厢系统 —— 车内事件、交易、任务
//!
//! 设计说明（由作者提供）：
//! - 车厢不是"黑屏加载"，而是完整的交互场景
//! - 乘车过程中可触发：对话/场景、商品交易、任务发布/接受
//! - "被遗弃车站的联系"是核心灵异剧情线，可能改变路线或触发超自然事件
//! - 06_train_scene.md（摇晃的电车）是车厢内事件的一种表现形式
//!
//! 当前实现：
//! - 定义了事件接口和数据结构
//! - 事件触发由 Journey 的 advance() 驱动
//! - 交易和任务系统预留了和 GameState 的集成点

use crate::engine::state::GameState;

/// 车厢事件类型
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum CarEventType {
    /// 对话/场景（如 06_train_scene.md）
    Dialogue {
        scene_id: String,
        /// 触发条件描述（用于调试）
        condition_note: String,
    },
    /// 商品交易
    Trade {
        shop_id: String,
    },
    /// 任务发布
    QuestOffer {
        quest_id: String,
    },
    /// 任务进展/完成
    QuestUpdate {
        quest_id: String,
        stage: String,
    },
    /// 被遗弃车站的联系（灵异事件）
    ///
    /// 触发条件：特定时间、特定路线、或持有特定 flag。
    /// 效果可能包括：
    /// - 车厢灯光闪烁
    /// - 广播出现杂音
    /// - 跳站到不存在的"被遗弃车站"
    /// - 修改 time_of_day（时间异常）
    AbandonedStationContact {
        station_id: String,
        effect: AbandonedEffect,
    },
    /// 异常事件（设备故障、突发停车等）
    Anomaly {
        description: String,
        /// 是否影响行程时间（分钟）
        delay_minutes: u32,
    },
}

/// 被遗弃车站事件的具体效果
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum AbandonedEffect {
    /// 时间异常（time_of_day 偏移）
    TimeShift { offset_seconds: i32 },
    /// 路线偏移（临时改变目的地）
    RouteDetour { new_destination: String },
    /// 纯粹叙事（无游戏机制影响）
    NarrativeOnly,
    /// 设置 flag
    SetFlag { key: String, value: String },
}

/// 车厢内事件
///
/// 每个事件关联一个触发时间窗口（乘车后经过的分钟数）。
/// 当 Journey::elapsed 落入窗口时，事件被触发一次。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CarEvent {
    pub event_type: CarEventType,
    /// 最早触发时间（上车后第 N 分钟）
    pub trigger_after_min: u32,
    /// 最晚触发时间（0 表示只在一个精确时间点）
    pub trigger_before_min: u32,
    /// 是否已触发（防止重复）
    pub triggered: bool,
    /// 触发概率（0-100），100 为必触发
    pub probability: u8,
    /// 需要满足的 flag 条件（AND 关系）
    pub required_flags: Vec<(String, String)>,
}

impl CarEvent {
    /// 创建一个必触发的叙事事件
    pub fn narrative(scene_id: impl Into<String>, after_min: u32) -> Self {
        Self {
            event_type: CarEventType::Dialogue {
                scene_id: scene_id.into(),
                condition_note: "narrative".into(),
            },
            trigger_after_min: after_min,
            trigger_before_min: after_min,
            triggered: false,
            probability: 100,
            required_flags: Vec::new(),
        }
    }

    /// 创建一个被遗弃车站事件
    pub fn abandoned_station(
        station_id: impl Into<String>,
        effect: AbandonedEffect,
        after_min: u32,
    ) -> Self {
        Self {
            event_type: CarEventType::AbandonedStationContact {
                station_id: station_id.into(),
                effect,
            },
            trigger_after_min: after_min,
            trigger_before_min: after_min + 1,
            triggered: false,
            probability: 30, // 低概率触发
            required_flags: Vec::new(),
        }
    }

    /// 检查是否应在当前 elapsed 分钟触发
    pub fn should_trigger(&self, elapsed_min: u32, game_state: &GameState) -> bool {
        if self.triggered {
            return false;
        }
        if elapsed_min < self.trigger_after_min {
            return false;
        }
        if self.trigger_before_min > 0 && elapsed_min > self.trigger_before_min {
            return false;
        }
        // 检查 flag 条件
        for (key, expected) in &self.required_flags {
            match game_state.get_flag(key) {
                Some(actual) if actual == expected => continue,
                _ => return false,
            }
        }
        true
    }
}

/// 车内商品
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CarShopItem {
    pub id: String,
    pub name: String,
    pub price: u32,
    /// 是否只能在车厢内购买
    pub train_exclusive: bool,
}

/// 车内商店
#[derive(Debug, Clone, Default)]
pub struct CarShop {
    pub items: Vec<CarShopItem>,
}

impl CarShop {
    pub fn buy(
        &self,
        item_id: &str,
        game_state: &mut GameState,
    ) -> Result<(), TradeError> {
        let item = self
            .items
            .iter()
            .find(|i| i.id == item_id)
            .ok_or(TradeError::ItemNotFound)?;

        if game_state.player.credit_level < item.price as i32 {
            return Err(TradeError::InsufficientFunds);
        }

        game_state.player.credit_level -= item.price as i32;
        game_state.add_item(item.id.clone(), 1);
        Ok(())
    }
}

/// 车内任务
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CarQuest {
    pub id: String,
    pub title: String,
    pub description: String,
    /// 任务发布车站
    pub issued_at: String,
    /// 目标车站（可能为 None，表示通用目标）
    pub target_station: Option<String>,
    /// 奖励（物品 ID + 数量）
    pub reward: Option<(String, i32)>,
    /// 是否已完成
    pub completed: bool,
}

/// 任务板
#[derive(Debug, Clone, Default)]
pub struct CarQuestBoard {
    pub available: Vec<CarQuest>,
    pub accepted: Vec<CarQuest>,
}

impl CarQuestBoard {
    /// 接受任务
    pub fn accept(&mut self, quest_id: &str) -> Option<CarQuest> {
        let idx = self.available.iter().position(|q| q.id == quest_id)?;
        let mut quest = self.available.remove(idx);
        quest.completed = false;
        self.accepted.push(quest.clone());
        Some(quest)
    }

    /// 检查并完成任务（如果在目标车站）
    pub fn try_complete(&mut self, station_id: &str, game_state: &mut GameState) {
        let completed: Vec<usize> = self
            .accepted
            .iter()
            .enumerate()
            .filter(|(_, q)| {
                !q.completed
                    && q.target_station
                        .as_ref()
                        .map(|t| t == station_id)
                        .unwrap_or(false)
            })
            .map(|(i, _)| i)
            .collect();

        for i in completed.into_iter().rev() {
            let mut quest = self.accepted.remove(i);
            quest.completed = true;
            // 发放奖励
            if let Some((item_id, qty)) = &quest.reward {
                game_state.add_item(item_id.clone(), *qty);
            }
            // 设置完成 flag
            game_state.set_flag(format!("quest_{}_completed", quest.id), "1");
        }
    }
}

/// 车厢
#[derive(Debug, Clone)]
pub struct TrainCar {
    pub car_number: u8,
    pub events: Vec<CarEvent>,
    pub shop: CarShop,
    pub quest_board: CarQuestBoard,
}

impl TrainCar {
    /// 创建默认车厢（含 06_train_scene 事件）
    pub fn default_for_prologue() -> Self {
        Self {
            car_number: 3, // 第3节车厢
            events: vec![
                // 上车后 1 分钟触发摇晃电车场景
                CarEvent::narrative("SCENE_06_TRAIN", 1),
            ],
            shop: CarShop {
                items: vec![
                    CarShopItem {
                        id: "train_bento".into(),
                        name: "駅弁".into(),
                        price: 800,
                        train_exclusive: true,
                    },
                    CarShopItem {
                        id: "battery".into(),
                        name: "乾電池".into(),
                        price: 300,
                        train_exclusive: false,
                    },
                ],
            },
            quest_board: CarQuestBoard::default(),
        }
    }

    /// 检查并触发事件
    ///
    /// 返回本次触发的事件列表。触发后标记为已触发。
    pub fn poll_events(
        &mut self,
        elapsed_min: u32,
        game_state: &GameState,
    ) -> Vec<CarEvent> {
        let mut triggered = Vec::new();
        for event in &mut self.events {
            if event.should_trigger(elapsed_min, game_state) {
                // 概率判定
                if event.probability >= 100 || fastrand::u8(0..100) < event.probability {
                    triggered.push(event.clone());
                }
                event.triggered = true;
            }
        }
        triggered
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum TradeError {
    ItemNotFound,
    InsufficientFunds,
}

impl std::fmt::Display for TradeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            TradeError::ItemNotFound => write!(f, "商品不存在"),
            TradeError::InsufficientFunds => write!(f, "余额不足"),
        }
    }
}

impl std::error::Error for TradeError {}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_event_trigger() {
        let event = CarEvent::narrative("SCENE_06_TRAIN", 1);
        let gs = GameState::default();
        assert!(!event.should_trigger(0, &gs));
        assert!(event.should_trigger(1, &gs));
        assert!(!event.should_trigger(2, &gs)); // 精确时间点
    }

    #[test]
    fn test_event_once_only() {
        let mut event = CarEvent::narrative("TEST", 1);
        let gs = GameState::default();
        assert!(event.should_trigger(1, &gs));
        event.triggered = true;
        assert!(!event.should_trigger(1, &gs));
    }

    #[test]
    fn test_car_poll_events() {
        let mut car = TrainCar::default_for_prologue();
        let gs = GameState::default();
        let events = car.poll_events(1, &gs);
        assert_eq!(events.len(), 1);
        assert_eq!(events[0].event_type, CarEventType::Dialogue { scene_id: "SCENE_06_TRAIN".into(), condition_note: "narrative".into() });
        // 再次轮询不应重复触发
        let events = car.poll_events(1, &gs);
        assert!(events.is_empty());
    }

    #[test]
    fn test_shop_buy() {
        let car = TrainCar::default_for_prologue();
        let mut gs = GameState::default();
        gs.player.credit_level = 1000;

        car.shop.buy("train_bento", &mut gs).unwrap();
        assert_eq!(gs.player.credit_level, 200);
        assert_eq!(gs.player.inventory.len(), 1);
        assert_eq!(gs.player.inventory[0].0, "train_bento");
    }

    #[test]
    fn test_quest_accept_and_complete() {
        let mut board = CarQuestBoard {
            available: vec![CarQuest {
                id: "test_q".into(),
                title: "测试任务".into(),
                description: "...".into(),
                issued_at: "shibuya".into(),
                target_station: Some("shinjuku".into()),
                reward: Some(("key".into(), 1)),
                completed: false,
            }],
            accepted: vec![],
        };

        let quest = board.accept("test_q");
        assert!(quest.is_some());
        assert!(board.available.is_empty());
        assert_eq!(board.accepted.len(), 1);

        let mut gs = GameState::default();
        board.try_complete("shinjuku", &mut gs);
        assert_eq!(gs.player.inventory.len(), 1);
        assert_eq!(gs.get_flag("quest_test_q_completed"), Some("1"));
    }
}
