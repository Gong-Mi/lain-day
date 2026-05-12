use crate::world::location::{Connection, Location, Poi};

pub fn build_yamanote() -> Vec<Location> {
    let mut locations = Vec::new();

    // === 山手线车站生成器 ===
    let stations: [(&str, &str); 30] = [
        ("shinagawa", "品川"), ("osaki", "大崎"), ("gotanda", "五反田"),
        ("meguro", "目黒"), ("ebisu", "恵比寿"), ("shibuya", "渋谷"),
        ("harajuku", "原宿"), ("yoyogi", "代々木"), ("shinjuku", "新宿"),
        ("shin_okubo", "新大久保"), ("takadanobaba", "高田馬場"), ("mejiro", "目白"),
        ("ikebukuro", "池袋"), ("otsuka", "大塚"), ("sugamo", "巣鴨"),
        ("komagome", "駒込"), ("tabata", "田端"), ("nishi_nippori", "西日暮里"),
        ("nippori", "日暮里"), ("uguisudani", "鶯谷"), ("ueno", "上野"),
        ("okachimachi", "御徒町"), ("akihabara", "秋葉原"), ("kanda", "神田"),
        ("tokyo", "東京"), ("yurakucho", "有楽町"), ("shimbashi", "新橋"),
        ("hamamatsucho", "浜松町"), ("tamachi", "田町"), ("takanawa_gateway", "高輪ゲートウェイ"),
    ];

    for i in 0..stations.len() {
        let (id, name) = stations[i];
        let prev = stations[(i + stations.len() - 1) % stations.len()].0;
        let next = stations[(i + 1) % stations.len()].0;
        locations.push(Location {
            id: id.into(),
            name: name.into(),
            description: format!("山手线 {} 站。", name).into(),
            pois: vec![Poi {
                id: "ticket_machine".into(),
                name: "购票机".into(),
                description: "自动售票机。".into(),
                examine_action_id: Some("use_ticket_machine".into()),
                view_scene_id: None,
            }],
            connections: vec![
                Connection {
                    action_id: "next_station".into(),
                    target_location_id: next.into(),
                    target_scene_id: None,
                    requires_flag: None,
                    denied_scene_id: None,
                },
                Connection {
                    action_id: "prev_station".into(),
                    target_location_id: prev.into(),
                    target_scene_id: None,
                    requires_flag: None,
                    denied_scene_id: None,
                },
            ],
        });
    }

    locations
}
