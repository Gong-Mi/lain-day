//! 电车系统 —— 车站、票务、车厢、行程
//!
//! ============================================================================
//! 设计意图（由作者提供，Rust 版需完整实现）
//! ============================================================================
//!
//! 电车系统不是"瞬间移动"，而是完整的状态机流程：
//!   售票 → 进站 → 上车 → 车厢内交互 → 下车 → 出站
//!
//! 车厢内是移动的社交/交易/任务空间，包含：
//!   · 对话/场景（如 06_train_scene.md 的摇晃电车事件）
//!   · 商品交易（车内限定商品）
//!   · 任务发布与接受
//!   · 被遗弃车站的联系（灵异/超自然剧情线，可能改变路线或时间）
//!
//! C 版局限：
//!   - train_system.c 只有购票机外壳（213 行，功能未完成）
//!   - 长距离移动被硬编码为 execute_action 中 25/30 分钟的瞬间跳转
//!   - 没有车厢内事件系统
//!
//! Rust 版目标：
//!   - 为每个阶段提供显式状态机（JourneyPhase）
//!   - 车厢内事件由 CarEvent 驱动，和 narrative 系统无缝衔接
//!   - 被遗弃车站预留了异常接口（AbandonedEffect）
//!
//! ============================================================================
//! 模块结构
//! ============================================================================
//!
//! · station  — 车站网络（30 站山手线环形，加载自 station_coordinates.json）
//! · ticket   — 票务（票价计算、售票机、检票器）
//! · car      — 车厢（车内事件、商店、任务板）
//! · journey  — 行程状态机（售票→进站→上车→乘车→下车→出站）
//!
//! ============================================================================
//! 和 narrative 系统的集成点
//! ============================================================================
//!
//! 1. 当玩家在世界地图中选择 "go_next_station" / "go_prev_station" 时，
//!    进入 `JourneyPhase::AtStation`，渲染对应车站场景。
//!
//! 2. 车站 POI "ticket_machine" 触发 `JourneyAction::BuyTicket`，
//!    进入 `JourneyPhase::BuyingTicket`，可调用 `TicketMachine::purchase()`。
//!
//! 3. 购票完成后，`Journey::advance()` 自动推进阶段。
//!    `JourneyPhase::OnTrain` 阶段返回 `SCENE_06_TRAIN`。
//!
//! 4. `TrainCar::poll_events()` 根据 elapsed_min 触发 CarEvent。
//!    若事件类型为 `Dialogue { scene_id }`，narrative 系统加载对应场景。
//!
//! 5. 到达后 `JourneyPhase::Completed`，`game_state.player.location` 更新，
//!    narrative 系统渲染目标车站场景。
//!
//! 6. 被遗弃车站事件（`AbandonedStationContact`）通过 `Journey::apply_anomaly()`
//!    修改行程状态，narrative 系统根据 `journey.anomaly` 渲染异常场景。
//!
//! ============================================================================
//! 和 executor 的集成点
//! ============================================================================
//!
//! C 版 executor.c 中硬编码的长距离动作（shibuya:25min, home:25min 等）
//! 在 Rust 版中应被替换为：
//!   · 检查玩家是否在车站
//!   · 创建 Journey
//!   · 推进到 OnTrain 阶段
//!   · 让 Journey 管理时间推进和场景切换
//!
//! 临时兼容：在 executor 尚未重构前，可使用 `Journey::new()` + 手动推进
//! 作为过渡方案。

pub mod car;
pub mod journey;
pub mod station;
pub mod ticket;

pub use car::{CarEvent, CarEventType, CarQuest, CarQuestBoard, CarShop, CarShopItem, TrainCar};
pub use journey::{
    Journey, JourneyAction, JourneyError, JourneyPhase, JourneyResult,
};
pub use station::{Direction, Station, StationId, StationNetwork};
pub use ticket::{FareTable, PurchaseError, Ticket, TicketMachine, TicketValidator};

use crate::engine::state::GameState;
use std::io::{self, Write};

use crossterm::event::KeyCode;
use crossterm::{
    cursor::MoveTo,
    terminal::{Clear, ClearType},
    ExecutableCommand,
};

// ============================================================================
// 遗留接口：购票机 UI
// ============================================================================
//
// 这是 C 版 train_system.c 的直接移植，保留供 backward compatibility。
// 未来应由 narrative 系统接管 BuyingTicket 阶段的渲染。

/// 运行购票机界面（遗留实现）
///
/// 该函数阻塞当前线程，使用 crossterm 事件循环。在完整实现中，
/// 应由 `JourneyPhase::BuyingTicket` 阶段的 narrative 渲染器替代。
pub fn run_ticket_machine() -> io::Result<Option<StationId>> {
    let network = StationNetwork::embedded();
    let stations = network.all_stations();

    let mut stdout = io::stdout();

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    println!("         东京电车购票机");
    println!("========================================");
    println!("请选择您的目的地：");
    for (i, station) in stations.iter().enumerate() {
        println!("  {}. {} ({})", i + 1, station.name, station.id);
    }
    println!("----------------------------------------");
    println!("  e. 退出");
    println!("========================================");
    stdout.flush()?;

    let mut selected: Option<StationId> = None;
    let mut running = true;

    while running {
        print!("> ");
        stdout.flush()?;

        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char('e') | KeyCode::Char('E') => {
                    running = false;
                }
                KeyCode::Char(c) if c.is_ascii_digit() => {
                    let idx = c.to_digit(10).unwrap() as usize;
                    if idx > 0 && idx <= stations.len() {
                        let station = &stations[idx - 1];
                        println!(
                            "您选择了: {}。",
                            station.name
                        );
                        selected = Some(station.id.clone());
                        // 在完整实现中，这里应调用 TicketMachine::purchase()
                        // 并返回 Ticket，而不是只打印信息。
                    } else {
                        println!("无效选择。");
                    }
                }
                _ => {}
            }
        }
    }

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;
    stdout.flush()?;

    Ok(selected)
}

// ============================================================================
// 便捷构造函数
// ============================================================================

/// 创建一次从 `from` 到 `to` 的完整行程（含购票、进站、上车）
///
/// 用于测试和快速启动。生产代码中应由玩家逐步操作。
pub fn create_journey(
    from: StationId,
    to: StationId,
    game_state: &mut GameState,
) -> Result<Journey, JourneyError> {
    let network = StationNetwork::embedded();
    let fare_table = FareTable::default();
    let machine = TicketMachine::new(&network, &fare_table);

    let mut journey = Journey::new(from.clone(), &network)
        .ok_or(JourneyError::InvalidRoute)?;

    if !journey.set_destination(to.clone(), &network) {
        return Err(JourneyError::InvalidRoute);
    }

    let ticket = machine
        .purchase(from, to, game_state.time_of_day, game_state)
        .map_err(|_| JourneyError::NoTicket)?;

    journey.set_ticket(ticket);

    // 自动推进到 WaitingOnPlatform（模拟已进站）
    journey.transition(JourneyAction::EnterGate).ok();

    Ok(journey)
}
