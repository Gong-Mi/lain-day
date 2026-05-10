//! 行程状态机 —— 售票 → 进站 → 上车 → 乘车 → 下车 → 出站
//!
//! 设计说明（由作者提供）：
//! - 电车系统不是"瞬间移动"，而是完整的状态流程
//! - 每个阶段都可能触发场景切换、对话或子系统接管
//! - 车厢内（OnTrain）是核心交互阶段，事件由 CarEvent 驱动
//! - 被遗弃车站的联系可能在任意阶段打破流程
//!
//! 和 C 版的差异：
//! - C 版：execute_action 硬编码 25/30 分钟跳转，无中间状态
//! - Rust 版：每个阶段显式建模，车厢内有完整的事件系统

use crate::engine::state::GameState;
use crate::systems::train::car::{CarEvent, CarEventType, TrainCar};
use crate::systems::train::station::{StationId, StationNetwork};
use crate::systems::train::ticket::{Ticket, TicketValidator};

/// 行程阶段
///
/// 状态转换图：
/// ```text
/// AtStation -> BuyingTicket -> TicketGate -> WaitingOnPlatform
///   ^                                                        |
///   |                                                        v
/// Completed <- ExitGate <- ExitingTrain <- Approaching <- OnTrain <- Boarding
/// ```
///
/// 特殊转换：
/// - OnTrain 阶段可能因被遗弃车站事件跳转到异常状态
/// - 任意阶段可因玩家选择取消而回到 AtStation
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum JourneyPhase {
    /// 在车站内（可交互：购票机、出口、NPC）
    AtStation,
    /// 正在购票机界面（子系统接管输入）
    BuyingTicket,
    /// 检票进站
    EnteringGate,
    /// 在站台上等车
    WaitingOnPlatform,
    /// 上车过程（过渡）
    Boarding,
    /// 在车厢内（核心交互阶段）
    OnTrain,
    /// 即将到站（广播）
    Approaching,
    /// 下车
    ExitingTrain,
    /// 检票出站
    ExitGate,
    /// 到达目的地，流程结束
    Completed,
    /// 被取消/中断
    Cancelled,
}

impl JourneyPhase {
    /// 阶段的文字描述
    pub fn description(&self) -> &'static str {
        match self {
            JourneyPhase::AtStation => "在车站内",
            JourneyPhase::BuyingTicket => "购票中",
            JourneyPhase::EnteringGate => "检票进站",
            JourneyPhase::WaitingOnPlatform => "在站台等车",
            JourneyPhase::Boarding => "上车",
            JourneyPhase::OnTrain => "车厢内",
            JourneyPhase::Approaching => "即将到站",
            JourneyPhase::ExitingTrain => "下车",
            JourneyPhase::ExitGate => "检票出站",
            JourneyPhase::Completed => "已到达",
            JourneyPhase::Cancelled => "已取消",
        }
    }

    /// 是否为进行中（未完成也未取消）
    pub fn is_active(&self) -> bool {
        !matches!(self, JourneyPhase::Completed | JourneyPhase::Cancelled)
    }
}

/// 行程推进结果
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum JourneyResult {
    /// 正常推进，阶段可能已改变
    Advanced,
    /// 触发了车厢事件
    EventTriggered(Vec<CarEvent>),
    /// 到达目的地
    Arrived,
    /// 行程被取消
    Cancelled,
    /// 发生异常（如被遗弃车站）
    Anomaly { description: String },
}

/// 行程
///
/// 管理一次完整的电车出行。创建后通过 `advance()` 推进。
#[derive(Debug, Clone)]
pub struct Journey {
    pub phase: JourneyPhase,
    pub from: StationId,
    pub to: StationId,
    /// 当前持有的票（进站后设置）
    pub ticket: Option<Ticket>,
    /// 乘坐的车厢
    pub car: TrainCar,
    /// 车上已过的分钟数
    pub elapsed_min: u32,
    /// 预计总乘车时间（分钟）
    pub total_min: u32,
    /// 距离（站数）
    pub distance: usize,
    /// 方向
    pub direction: crate::systems::train::station::Direction,
    /// 是否已经触发过到达事件
    pub arrival_handled: bool,
    /// 异常状态描述（被遗弃车站等）
    pub anomaly: Option<String>,
}

impl Journey {
    /// 创建新行程（尚未购票）
    ///
    /// `current_station` 是当前所在站（用于计算距离和时间）。
    pub fn new(
        current_station: StationId,
        network: &StationNetwork,
    ) -> Option<Self> {
        // 默认目的地为下一站（玩家可在购票时更改）
        let default_to = network.next(&current_station)?.id.clone();
        let distance = network.distance(&current_station, &default_to)?;
        let direction = network.direction(&current_station, &default_to)?;
        let total_min = network.estimated_travel_time(&current_station, &default_to)?;

        Some(Self {
            phase: JourneyPhase::AtStation,
            from: current_station,
            to: default_to,
            ticket: None,
            car: TrainCar::default_for_prologue(),
            elapsed_min: 0,
            total_min,
            distance,
            direction,
            arrival_handled: false,
            anomaly: None,
        })
    }

    /// 设置目的地（购票后调用）
    pub fn set_destination(
        &mut self,
        to: StationId,
        network: &StationNetwork,
    ) -> bool {
        if network.station(&to).is_none() {
            return false;
        }
        self.to = to.clone();
        if let Some(dist) = network.distance(&self.from, &to) {
            self.distance = dist;
            self.total_min = network.estimated_travel_time(&self.from, &to).unwrap_or(0);
            self.direction = network.direction(&self.from, &to).unwrap_or(
                crate::systems::train::station::Direction::Clockwise,
            );
            true
        } else {
            false
        }
    }

    /// 设置车票（购票成功后调用）
    pub fn set_ticket(&mut self, ticket: Ticket) {
        self.ticket = Some(ticket);
    }

    /// 推进行程
    ///
    /// `delta_min`：本次推进的分钟数（通常为 1，表示一分钟）。
    /// 返回推进结果，可能包含触发的事件或阶段变化。
    pub fn advance(
        &mut self,
        delta_min: u32,
        game_state: &mut GameState,
    ) -> JourneyResult {
        if !self.phase.is_active() {
            return JourneyResult::Arrived;
        }

        // 阶段自动转换逻辑
        match &self.phase {
            JourneyPhase::EnteringGate => {
                if let Some(ticket) = &self.ticket {
                    if TicketValidator::validate_entry(ticket, &self.from) {
                        self.phase = JourneyPhase::WaitingOnPlatform;
                    } else {
                        self.phase = JourneyPhase::Cancelled;
                        return JourneyResult::Cancelled;
                    }
                } else {
                    self.phase = JourneyPhase::Cancelled;
                    return JourneyResult::Cancelled;
                }
            }
            JourneyPhase::Boarding => {
                self.phase = JourneyPhase::OnTrain;
                self.elapsed_min = 0;
            }
            JourneyPhase::OnTrain => {
                self.elapsed_min += delta_min;

                // 检查车厢事件
                let events = self.car.poll_events(self.elapsed_min, game_state);
                let has_events = !events.is_empty();

                // 检查是否到站
                if self.elapsed_min >= self.total_min && !self.arrival_handled {
                    self.arrival_handled = true;
                    self.phase = JourneyPhase::Approaching;
                    if has_events {
                        let mut result = events;
                        // 追加到达事件
                        result.push(CarEvent {
                            event_type: CarEventType::Dialogue {
                                scene_id: "SCENE_ARRIVAL_BROADCAST".into(),
                                condition_note: "arrival".into(),
                            },
                            trigger_after_min: self.elapsed_min,
                            trigger_before_min: self.elapsed_min,
                            triggered: true,
                            probability: 100,
                            required_flags: Vec::new(),
                        });
                        return JourneyResult::EventTriggered(result);
                    }
                    return JourneyResult::EventTriggered(vec![CarEvent {
                        event_type: CarEventType::Dialogue {
                            scene_id: "SCENE_ARRIVAL_BROADCAST".into(),
                            condition_note: "arrival".into(),
                        },
                        trigger_after_min: self.elapsed_min,
                        trigger_before_min: self.elapsed_min,
                        triggered: true,
                        probability: 100,
                        required_flags: Vec::new(),
                    }]);
                }

                if has_events {
                    return JourneyResult::EventTriggered(events);
                }

                return JourneyResult::Advanced;
            }
            JourneyPhase::Approaching => {
                self.phase = JourneyPhase::ExitingTrain;
            }
            JourneyPhase::ExitingTrain => {
                self.phase = JourneyPhase::ExitGate;
            }
            JourneyPhase::ExitGate => {
                if let Some(ticket) = &self.ticket {
                    if TicketValidator::validate_exit(ticket, &self.to) {
                        self.phase = JourneyPhase::Completed;
                        // 更新玩家位置
                        game_state.player.location.clone_from(&self.to);
                        // 尝试完成任务
                        self.car.quest_board.try_complete(&self.to, game_state);
                        return JourneyResult::Arrived;
                    } else {
                        // 票不匹配：可能坐过站或被遗弃车站改变了路线
                        self.anomaly = Some("票证与出站口不匹配".into());
                        return JourneyResult::Anomaly {
                            description: "票证与出站口不匹配".into(),
                        };
                    }
                } else {
                    self.phase = JourneyPhase::Cancelled;
                    return JourneyResult::Cancelled;
                }
            }
            _ => {}
        }

        JourneyResult::Advanced
    }

    /// 手动触发阶段转换（玩家动作驱动）
    ///
    /// 例如：玩家选择"购买车票"→ BuyingTicket，"进站"→ EnteringGate。
    pub fn transition(&mut self, action: JourneyAction) -> Result<(), JourneyError> {
        let allowed = self.allowed_actions();
        if !allowed.contains(&action) {
            return Err(JourneyError::InvalidAction {
                phase: self.phase.clone(),
                action,
            });
        }

        match action {
            JourneyAction::BuyTicket => {
                self.phase = JourneyPhase::BuyingTicket;
            }
            JourneyAction::EnterGate => {
                self.phase = JourneyPhase::EnteringGate;
            }
            JourneyAction::Board => {
                self.phase = JourneyPhase::Boarding;
            }
            JourneyAction::GetOff => {
                self.phase = JourneyPhase::ExitingTrain;
            }
            JourneyAction::Cancel => {
                self.phase = JourneyPhase::Cancelled;
            }
        }
        Ok(())
    }

    /// 当前阶段允许的玩家动作
    pub fn allowed_actions(&self) -> Vec<JourneyAction> {
        match self.phase {
            JourneyPhase::AtStation => {
                vec![JourneyAction::BuyTicket, JourneyAction::Cancel]
            }
            JourneyPhase::BuyingTicket => {
                vec![JourneyAction::EnterGate, JourneyAction::Cancel]
            }
            JourneyPhase::EnteringGate => vec![],
            JourneyPhase::WaitingOnPlatform => {
                vec![JourneyAction::Board, JourneyAction::Cancel]
            }
            JourneyPhase::Boarding => vec![],
            JourneyPhase::OnTrain => {
                vec![JourneyAction::GetOff]
            }
            JourneyPhase::Approaching => vec![],
            JourneyPhase::ExitingTrain => vec![],
            JourneyPhase::ExitGate => vec![],
            JourneyPhase::Completed | JourneyPhase::Cancelled => vec![],
        }
    }

    /// 当前应渲染的场景 ID
    ///
    /// 返回 `None` 表示使用默认车站/车厢描述。
    pub fn current_scene(&self) -> Option<String> {
        match &self.phase {
            JourneyPhase::AtStation => Some(format!("STATION_{}", self.from)),
            JourneyPhase::BuyingTicket => Some("SCENE_TICKET_MACHINE".into()),
            JourneyPhase::EnteringGate => Some("SCENE_ENTER_GATE".into()),
            JourneyPhase::WaitingOnPlatform => Some("SCENE_PLATFORM".into()),
            JourneyPhase::Boarding => Some("SCENE_BOARDING".into()),
            JourneyPhase::OnTrain => Some("SCENE_06_TRAIN".into()),
            JourneyPhase::Approaching => Some("SCENE_APPROACHING".into()),
            JourneyPhase::ExitingTrain => Some("SCENE_EXITING_TRAIN".into()),
            JourneyPhase::ExitGate => Some("SCENE_EXIT_GATE".into()),
            JourneyPhase::Completed => Some(format!("STATION_{}", self.to)),
            JourneyPhase::Cancelled => None,
        }
    }

    /// 当前场景的描述文本（用于无特定 scene ID 时的默认显示）
    pub fn default_description(&self) -> String {
        match &self.phase {
            JourneyPhase::AtStation => format!("你站在 {} 站内。", self.from),
            JourneyPhase::WaitingOnPlatform => {
                format!(
                    "{}方向的列车即将进站。目的地：{}（{}站）",
                    self.direction.label(),
                    self.to,
                    self.distance
                )
            }
            JourneyPhase::OnTrain => format!(
                "列车行驶中... 已行驶 {}/{} 分钟",
                self.elapsed_min, self.total_min
            ),
            JourneyPhase::Approaching => format!("列车即将到达 {} 站。", self.to),
            _ => self.phase.description().to_string(),
        }
    }

    /// 应用被遗弃车站的异常效果
    ///
    /// 可能改变目的地、时间、flag 等。
    pub fn apply_anomaly(&mut self, description: impl Into<String>) {
        self.anomaly = Some(description.into());
    }
}

/// 玩家可对行程执行的动作
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum JourneyAction {
    /// 购买车票
    BuyTicket,
    /// 检票进站
    EnterGate,
    /// 上车
    Board,
    /// 下车
    GetOff,
    /// 取消行程
    Cancel,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum JourneyError {
    InvalidAction { phase: JourneyPhase, action: JourneyAction },
    InvalidRoute,
    NoTicket,
}

impl std::fmt::Display for JourneyError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            JourneyError::InvalidAction { phase, action } => {
                write!(f, "在 {:?} 阶段不能执行 {:?}", phase, action)
            }
            JourneyError::InvalidRoute => write!(f, "无效路线"),
            JourneyError::NoTicket => write!(f, "未持票"),
        }
    }
}

impl std::error::Error for JourneyError {}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::systems::train::station::StationNetwork;

    fn test_journey() -> Journey {
        let net = StationNetwork::embedded();
        Journey::new("shibuya".to_string(), &net).unwrap()
    }

    #[test]
    fn test_journey_creation() {
        let j = test_journey();
        assert_eq!(j.phase, JourneyPhase::AtStation);
        assert_eq!(j.from, "shibuya");
        assert_eq!(j.distance, 1); // 到下一站大崎
        assert_eq!(j.total_min, 2); // 1站*2分钟
    }

    #[test]
    fn test_set_destination() {
        let mut j = test_journey();
        let net = StationNetwork::embedded();
        assert!(j.set_destination("shinjuku".to_string(), &net));
        assert_eq!(j.to, "shinjuku");
        assert_eq!(j.distance, 3); // 涩谷→原宿→代代木→新宿
        assert_eq!(j.total_min, 6);
    }

    #[test]
    fn test_full_flow() {
        let net = StationNetwork::embedded();
        let mut j = Journey::new("shibuya".to_string(), &net).unwrap();
        let mut gs = GameState::default();

        // 购票
        j.set_ticket(Ticket::test_ticket("shibuya", "harajuku"));
        j.set_destination("harajuku".to_string(), &net);

        // 进入购票机阶段
        j.transition(JourneyAction::BuyTicket).unwrap();
        assert_eq!(j.phase, JourneyPhase::BuyingTicket);

        // 进站
        j.transition(JourneyAction::EnterGate).unwrap();
        let _r = j.advance(0, &mut gs);
        assert_eq!(j.phase, JourneyPhase::WaitingOnPlatform);

        // 上车
        j.transition(JourneyAction::Board).unwrap();
        assert_eq!(j.phase, JourneyPhase::Boarding);
        let _r = j.advance(0, &mut gs);
        assert_eq!(j.phase, JourneyPhase::OnTrain);

        // 乘车 1 分钟 → 触发摇晃电车场景
        let r = j.advance(1, &mut gs);
        assert!(matches!(r, JourneyResult::EventTriggered(_)));

        // 继续乘车
        let _r = j.advance(1, &mut gs);
        // 1站到站（total_min=2，已走1，再走1）
        // 注意：advance(1) 只会增加 elapsed_min 到 2，但触发到达的逻辑在 OnTrain 分支
        // 需要再走一步让 phase 变为 Approaching
        assert_eq!(j.phase, JourneyPhase::Approaching);

        // 下车流程
        let _r = j.advance(0, &mut gs);
        assert_eq!(j.phase, JourneyPhase::ExitingTrain);
        let _r = j.advance(0, &mut gs);
        assert_eq!(j.phase, JourneyPhase::ExitGate);
        let r = j.advance(0, &mut gs);
        assert_eq!(j.phase, JourneyPhase::Completed);
        assert_eq!(r, JourneyResult::Arrived);
        assert_eq!(gs.player.location, "harajuku");
    }

    #[test]
    fn test_cancel() {
        let mut j = test_journey();
        j.transition(JourneyAction::Cancel).unwrap();
        assert_eq!(j.phase, JourneyPhase::Cancelled);
    }
}
