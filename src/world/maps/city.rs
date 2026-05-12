use crate::world::location::{Connection, Location, Poi};

pub fn build_city(_s: &dyn Fn(&str, &str) -> String) -> Vec<Location> {
    let mut locations = Vec::new();

    // --- 17. 涩谷街道 ---
    let shibuya_street = Location {
        id: "shibuya_street".into(),
        name: "涩谷街道".into(),
        description: "繁华的商业区，人群川流不息，巨大的电子广告牌闪烁着。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "cyberia".into(),
                target_location_id: "cyberia_club".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "home".into(),
                target_location_id: "miyanosaka_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 18. 新宿废弃工地 ---
    let shinjuku_abandoned_site = Location {
        id: "shinjuku_abandoned_site".into(),
        name: "新宿废弃工地".into(),
        description: "被围起来的废弃建筑工地，夜晚显得格外阴森。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "shinjuku".into(),
                target_location_id: "shinjuku_station".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 19. 新宿站 ---
    let shinjuku_station = Location {
        id: "shinjuku_station".into(),
        name: "新宿站".into(),
        description: "东京最繁忙的车站之一，巨大的换乘枢纽。".into(),
        pois: vec![Poi {
            id: "ticket_machine".into(),
            name: "购票机".into(),
            description: "自动售票机。".into(),
            examine_action_id: Some("use_ticket_machine".into()),
            view_scene_id: None,
        }],
        connections: vec![
            Connection {
                action_id: "site".into(),
                target_location_id: "shinjuku_abandoned_site".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 20. Cyberia 俱乐部 ---
    let cyberia_club = Location {
        id: "cyberia_club".into(),
        name: "Cyberia".into(),
        description: "地下俱乐部，昏暗的灯光和电子音乐充斥着空间。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "shibuya".into(),
                target_location_id: "shibuya_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 21. 六本木街道 ---
    let roppongi_street = Location {
        id: "roppongi_street".into(),
        name: "六本木街道".into(),
        description: " upscale 的商业街，高级餐厅和精品店林立。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "school".into(),
                target_location_id: "roppongi_school_gate".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 22. 千砂家 ---
    let chisa_home = Location {
        id: "chisa_home".into(),
        name: "千砂家".into(),
        description: "一栋普通的公寓，千砂曾经住在这里。".into(),
        pois: vec![],
        connections: vec![],
    };

    locations.push(shibuya_street);
    locations.push(shinjuku_abandoned_site);
    locations.push(shinjuku_station);
    locations.push(cyberia_club);
    locations.push(roppongi_street);
    locations.push(chisa_home);

    locations
}
