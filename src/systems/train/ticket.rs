//! 票务系统 —— 售票、检票、票价计算
//!
//! 设计说明：
//! - 山手线统一票价模型：起步价 + 每站附加费
//! - 购票时扣除 credit_level
//! - 检票时验证 from/to 匹配
//! - 预留多线路支持（`line` 字段）

use crate::engine::state::GameState;
use crate::systems::train::station::{StationId, StationNetwork};

/// 车票
///
/// 一次行程对应一张票。上车时验证 from，下车时验证 to。
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Ticket {
    pub from: StationId,
    pub to: StationId,
    /// 线路 ID（预留山手线以外线路）
    pub line: String,
    /// 票价（日元）
    pub fare: u32,
    /// 发行时间（ECC 编码）
    pub issued_at: u32,
}

impl Ticket {
    /// 快速创建测试票（不扣款）
    #[cfg(test)]
    pub fn test_ticket(from: &str, to: &str) -> Self {
        Self {
            from: from.into(),
            to: to.into(),
            line: "yamanote".into(),
            fare: 150,
            issued_at: 0,
        }
    }
}

/// 票价表
///
/// 简化模型：基础票价 + 每站附加费。
/// 未来可扩展为按区域计费（东京都区内/区间外）。
#[derive(Debug, Clone)]
pub struct FareTable {
    /// 起步价
    pub base_fare: u32,
    /// 每站附加费
    pub per_station: u32,
}

impl FareTable {
    pub fn new(base_fare: u32, per_station: u32) -> Self {
        Self {
            base_fare,
            per_station,
        }
    }

    /// 计算两站间票价
    pub fn calculate(
        &self,
        network: &StationNetwork,
        from: &StationId,
        to: &StationId,
    ) -> Option<u32> {
        let dist = network.distance(from, to)?;
        Some(self.base_fare + self.per_station * dist as u32)
    }
}

impl Default for FareTable {
    /// 山手线默认票价：起步 150 日元，每站 10 日元
    fn default() -> Self {
        Self::new(150, 10)
    }
}

/// 售票机 —— 负责购票流程
pub struct TicketMachine<'a> {
    pub network: &'a StationNetwork,
    pub fare_table: &'a FareTable,
}

impl<'a> TicketMachine<'a> {
    pub fn new(network: &'a StationNetwork, fare_table: &'a FareTable) -> Self {
        Self {
            network,
            fare_table,
        }
    }

    /// 列出从当前站可到达的所有目的地及票价
    pub fn list_destinations(
        &self,
        current: &StationId,
    ) -> Vec<(StationId, String, u32)> {
        self.network
            .all_stations()
            .iter()
            .filter(|s| &s.id != current)
            .filter_map(|s| {
                let fare = self.fare_table.calculate(self.network, current, &s.id)?;
                Some((s.id.clone(), s.name.clone(), fare))
            })
            .collect()
    }

    /// 购票
    ///
    /// 扣除 credit_level，返回 Ticket。失败时返回错误。
    pub fn purchase(
        &self,
        from: StationId,
        to: StationId,
        issued_at: u32,
        game_state: &mut GameState,
    ) -> Result<Ticket, PurchaseError> {
        let fare = self
            .fare_table
            .calculate(self.network, &from, &to)
            .ok_or(PurchaseError::InvalidRoute)?;

        if game_state.player.credit_level < fare as i32 {
            return Err(PurchaseError::InsufficientFunds);
        }

        game_state.player.credit_level -= fare as i32;

        Ok(Ticket {
            from,
            to,
            line: "yamanote".to_string(),
            fare,
            issued_at,
        })
    }
}

/// 检票器
pub struct TicketValidator;

impl TicketValidator {
    /// 验证进站票（当前站是否为出发站）
    pub fn validate_entry(ticket: &Ticket, station: &StationId) -> bool {
        &ticket.from == station
    }

    /// 验证出站票（当前站是否为目的站）
    pub fn validate_exit(ticket: &Ticket, station: &StationId) -> bool {
        &ticket.to == station
    }

    /// 完整验证（进站+出站）
    pub fn validate_full(ticket: &Ticket, entry: &StationId, exit: &StationId) -> bool {
        Self::validate_entry(ticket, entry) && Self::validate_exit(ticket, exit)
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PurchaseError {
    /// 路线无效（车站不存在或相同）
    InvalidRoute,
    /// 余额不足
    InsufficientFunds,
}

impl std::fmt::Display for PurchaseError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            PurchaseError::InvalidRoute => write!(f, "无效路线"),
            PurchaseError::InsufficientFunds => write!(f, "余额不足"),
        }
    }
}

impl std::error::Error for PurchaseError {}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::systems::train::station::StationNetwork;

    fn setup() -> (StationNetwork, FareTable) {
        (StationNetwork::embedded(), FareTable::default())
    }

    #[test]
    fn test_fare_calculation() {
        let (net, fare) = setup();
        // 涩谷到原宿 = 1 站 = 150 + 10 = 160
        let f = fare.calculate(&net, &"shibuya".to_string(), &"harajuku".to_string());
        assert_eq!(f, Some(160));
        // 涩谷到池袋 = 7 站 = 150 + 70 = 220
        let f = fare.calculate(&net, &"shibuya".to_string(), &"ikebukuro".to_string());
        assert_eq!(f, Some(220));
    }

    #[test]
    fn test_purchase_success() {
        let (net, fare) = setup();
        let machine = TicketMachine::new(&net, &fare);
        let mut gs = GameState::default();
        gs.player.credit_level = 500;

        let ticket = machine.purchase(
            "shibuya".to_string(),
            "ikebukuro".to_string(),
            0,
            &mut gs,
        );
        assert!(ticket.is_ok());
        assert_eq!(ticket.unwrap().fare, 220);
        assert_eq!(gs.player.credit_level, 280);
    }

    #[test]
    fn test_purchase_insufficient_funds() {
        let (net, fare) = setup();
        let machine = TicketMachine::new(&net, &fare);
        let mut gs = GameState::default();
        gs.player.credit_level = 100;

        let result = machine.purchase(
            "shibuya".to_string(),
            "ikebukuro".to_string(),
            0,
            &mut gs,
        );
        assert_eq!(result, Err(PurchaseError::InsufficientFunds));
        // 余额不应被扣除
        assert_eq!(gs.player.credit_level, 100);
    }

    #[test]
    fn test_validator() {
        let ticket = Ticket::test_ticket("shibuya", "shinjuku");
        assert!(TicketValidator::validate_entry(&ticket, &"shibuya".to_string()));
        assert!(!TicketValidator::validate_entry(&ticket, &"shinjuku".to_string()));
        assert!(TicketValidator::validate_exit(&ticket, &"shinjuku".to_string()));
        assert!(TicketValidator::validate_full(
            &ticket,
            &"shibuya".to_string(),
            &"shinjuku".to_string()
        ));
    }
}
