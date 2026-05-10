//! 车站网络 —— 基于 station_coordinates.json 的环形线路
//!
//! 设计说明：
//! - 30 站山手线环形排列（品川 → 高輪ゲートウェイ → ... → 品川）
//! - 每站都是独立 Location，有 ticket_machine POI
//! - 站间移动通过 `go_next_station` / `go_prev_station` 连接
//! - 宫之坂站 (miyanosaka_station) 是特殊站，连接外部街道

use serde::{Deserialize, Serialize};
use std::collections::HashMap;

pub type StationId = String;

/// 车站
#[derive(Debug, Clone, Serialize, Deserialize, PartialEq, Eq)]
pub struct Station {
    pub id: StationId,
    pub name: String,
    /// 坐标（当前未用于路线计算，预留未来地图可视化）
    pub x: i32,
    pub y: i32,
}

/// 线路方向
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Direction {
    /// 外回り（顺时针）
    Clockwise,
    /// 内回り（逆时针）
    CounterClockwise,
}

impl Direction {
    /// 方向的文字描述
    pub fn label(self) -> &'static str {
        match self {
            Direction::Clockwise => "外回り",
            Direction::CounterClockwise => "内回り",
        }
    }
}

/// 车站网络
///
/// 负责加载、查询、路由计算。数据来自嵌入的 station_coordinates.json。
#[derive(Debug, Clone, Default)]
pub struct StationNetwork {
    stations: Vec<Station>,
    id_to_index: HashMap<StationId, usize>,
}

impl StationNetwork {
    /// 从 JSON 字符串加载
    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        let stations: Vec<Station> = serde_json::from_str(json)?;
        let mut id_to_index = HashMap::new();
        for (i, s) in stations.iter().enumerate() {
            id_to_index.insert(s.id.clone(), i);
        }
        Ok(Self {
            stations,
            id_to_index,
        })
    }

    /// 使用编译时嵌入的车站数据
    pub fn embedded() -> Self {
        const JSON: &str = include_str!("../../../sequences/station_coordinates.json");
        Self::from_json(JSON).expect("嵌入的车站数据格式错误")
    }

    /// 车站总数
    pub fn len(&self) -> usize {
        self.stations.len()
    }

    pub fn is_empty(&self) -> bool {
        self.stations.is_empty()
    }

    /// 按 ID 查找车站
    pub fn station(&self, id: &StationId) -> Option<&Station> {
        self.id_to_index.get(id).map(|&i| &self.stations[i])
    }

    /// 所有车站列表
    pub fn all_stations(&self) -> &[Station] {
        &self.stations
    }

    /// 获取下一站（环形）
    pub fn next(&self, id: &StationId) -> Option<&Station> {
        let idx = *self.id_to_index.get(id)?;
        let next_idx = (idx + 1) % self.stations.len();
        Some(&self.stations[next_idx])
    }

    /// 获取前一站（环形）
    pub fn prev(&self, id: &StationId) -> Option<&Station> {
        let idx = *self.id_to_index.get(id)?;
        let prev_idx = (idx + self.stations.len() - 1) % self.stations.len();
        Some(&self.stations[prev_idx])
    }

    /// 计算两站之间的最短距离（按站数）
    pub fn distance(&self, from: &StationId, to: &StationId) -> Option<usize> {
        let from_idx = *self.id_to_index.get(from)?;
        let to_idx = *self.id_to_index.get(to)?;
        let n = self.stations.len();
        let forward = (to_idx + n - from_idx) % n;
        let backward = (from_idx + n - to_idx) % n;
        Some(forward.min(backward))
    }

    /// 计算最优方向
    pub fn direction(&self, from: &StationId, to: &StationId) -> Option<Direction> {
        let from_idx = *self.id_to_index.get(from)?;
        let to_idx = *self.id_to_index.get(to)?;
        let n = self.stations.len();
        let forward = (to_idx + n - from_idx) % n;
        let backward = (from_idx + n - to_idx) % n;
        if forward <= backward {
            Some(Direction::Clockwise)
        } else {
            Some(Direction::CounterClockwise)
        }
    }

    /// 计算两站之间经过的所有站（含起点，不含终点）
    ///
    /// 按最优方向排列，用于显示路线或逐站推进。
    pub fn route_stations(
        &self,
        from: &StationId,
        to: &StationId,
    ) -> Option<Vec<&Station>> {
        let from_idx = *self.id_to_index.get(from)?;
        let to_idx = *self.id_to_index.get(to)?;
        let n = self.stations.len();
        let forward = (to_idx + n - from_idx) % n;
        let backward = (from_idx + n - to_idx) % n;

        let (step, count) = if forward <= backward {
            (1usize, forward)
        } else {
            (n - 1, backward)
        };

        let mut result = Vec::with_capacity(count);
        let mut current = from_idx;
        for _ in 0..count {
            result.push(&self.stations[current]);
            current = (current + step) % n;
        }
        Some(result)
    }

    /// 估算乘车时间（分钟）—— 每站约 2 分钟
    pub fn estimated_travel_time(&self, from: &StationId, to: &StationId) -> Option<u32> {
        self.distance(from, to).map(|d| d as u32 * 2)
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    fn test_network() -> StationNetwork {
        StationNetwork::embedded()
    }

    #[test]
    fn test_load_embedded() {
        let net = test_network();
        assert_eq!(net.len(), 30);
        assert!(net.station(&"shibuya".to_string()).is_some());
        assert!(net.station(&"shinjuku".to_string()).is_some());
    }

    #[test]
    fn test_next_prev() {
        let net = test_network();
        // 品川 -> 大崎 -> 五反田
        let shinagawa = net.station(&"shinagawa".to_string()).unwrap();
        let osaki = net.next(&shinagawa.id).unwrap();
        assert_eq!(osaki.id, "osaki");
        let gotanda = net.next(&osaki.id).unwrap();
        assert_eq!(gotanda.id, "gotanda");
        // 反向
        let back = net.prev(&osaki.id).unwrap();
        assert_eq!(back.id, "shinagawa");
    }

    #[test]
    fn test_distance_and_direction() {
        let net = test_network();
        // 涩谷到池袋：顺时针 7 站（涩谷→原宿→代代木→新宿→新大久保→高田马场→目白→池袋）
        // 逆时针：30-7=23 站
        let dist = net.distance(&"shibuya".to_string(), &"ikebukuro".to_string());
        assert_eq!(dist, Some(7));
        let dir = net.direction(&"shibuya".to_string(), &"ikebukuro".to_string());
        assert_eq!(dir, Some(Direction::Clockwise));

        // 池袋到涩谷：方向相反
        let dir_back = net.direction(&"ikebukuro".to_string(), &"shibuya".to_string());
        assert_eq!(dir_back, Some(Direction::CounterClockwise));
    }

    #[test]
    fn test_route_stations() {
        let net = test_network();
        let route = net.route_stations(&"shibuya".to_string(), &"harajuku".to_string());
        assert_eq!(route.map(|r| r.len()), Some(1)); // 只经过涩谷
        let route = net.route_stations(&"shibuya".to_string(), &"shinjuku".to_string());
        assert_eq!(route.map(|r| r.len()), Some(3)); // 涩谷→原宿→代代木
    }
}
