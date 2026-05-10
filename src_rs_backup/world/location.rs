//! 世界地图 —— 地点、连接、兴趣点

use serde::Deserialize;

/// 兴趣点（Point of Interest）
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct Poi {
    pub id: String,
    pub name: String,
    pub description: String,
    #[serde(default)]
    pub examine_action_id: Option<String>,
    #[serde(default)]
    pub view_scene_id: Option<String>,
}

/// 地点之间的连接
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct Connection {
    pub action_id: String,
    pub target_location_id: String,
    #[serde(default)]
    pub target_scene_id: Option<String>,
    #[serde(default)]
    pub requires_flag: Option<String>,
    #[serde(default)]
    pub denied_scene_id: Option<String>,
}

/// 地点
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct Location {
    pub id: String,
    pub name: String,
    pub description: String,
    #[serde(default)]
    pub pois: Vec<Poi>,
    #[serde(default)]
    pub connections: Vec<Connection>,
}

/// 世界地图
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct WorldMap {
    locations: Vec<Location>,
}

impl WorldMap {
    pub fn new() -> Self {
        Self { locations: Vec::new() }
    }

    pub fn add_location(&mut self, loc: Location) {
        self.locations.push(loc);
    }

    pub fn find(&self, id: &str) -> Option<&Location> {
        self.locations.iter().find(|l| l.id == id)
    }

    pub fn find_connection(&self, from: &str, action_id: &str) -> Option<&Connection> {
        self.find(from)?.connections.iter().find(|c| c.action_id == action_id)
    }

    pub fn find_poi(&self, loc_id: &str, poi_id: &str) -> Option<&Poi> {
        self.find(loc_id)?.pois.iter().find(|p| p.id == poi_id)
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    fn make_test_map() -> WorldMap {
        let mut map = WorldMap::new();
        map.add_location(Location {
            id: "iwakura_upper_hallway".into(),
            name: "上走廊".into(),
            description: "二楼的走廊。".into(),
            pois: vec![
                Poi {
                    id: "painting".into(),
                    name: "油画".into(),
                    description: "一幅看不懂的抽象画。".into(),
                    examine_action_id: None,
                    view_scene_id: None,
                }
            ],
            connections: vec![
                Connection {
                    action_id: "downstairs".into(),
                    target_location_id: "iwakura_lower_hallway".into(),
                    target_scene_id: Some("SCENE_IWAKURA_LOWER_HALLWAY".into()),
                    requires_flag: None,
                    denied_scene_id: None,
                },
                Connection {
                    action_id: "lains_room".into(),
                    target_location_id: "iwakura_lains_room".into(),
                    target_scene_id: Some("SCENE_IWAKURA_LAINS_ROOM".into()),
                    requires_flag: None,
                    denied_scene_id: None,
                },
            ],
        });
        map.add_location(Location {
            id: "iwakura_lains_room".into(),
            name: "lain的房间".into(),
            description: "房间里很暗。".into(),
            pois: vec![],
            connections: vec![
                Connection {
                    action_id: "upper_hallway".into(),
                    target_location_id: "iwakura_upper_hallway".into(),
                    target_scene_id: Some("SCENE_IWAKURA_UPPER_HALLWAY".into()),
                    requires_flag: None,
                    denied_scene_id: None,
                }
            ],
        });
        map
    }

    #[test]
    fn test_find_location() {
        let map = make_test_map();
        assert!(map.find("iwakura_upper_hallway").is_some());
        assert!(map.find("nowhere").is_none());
    }

    #[test]
    fn test_find_connection() {
        let map = make_test_map();
        let conn = map.find_connection("iwakura_upper_hallway", "downstairs");
        assert!(conn.is_some());
        assert_eq!(conn.unwrap().target_location_id, "iwakura_lower_hallway");

        assert!(map.find_connection("iwakura_upper_hallway", "fly").is_none());
    }

    #[test]
    fn test_find_poi() {
        let map = make_test_map();
        let poi = map.find_poi("iwakura_upper_hallway", "painting");
        assert!(poi.is_some());
        assert_eq!(poi.unwrap().name, "油画");
    }
}
