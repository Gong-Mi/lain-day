use crate::world::location::{Connection, Location, Poi};

pub fn build_neighborhood(_s: &dyn Fn(&str, &str) -> String) -> Vec<Location> {
    let mut locations = Vec::new();

    // --- 9. 宫之坂街道 ---
    let miyanosaka_street = Location {
        id: "miyanosaka_street".into(),
        name: "宫之坂街道".into(),
        description: "安静的住宅区街道，两旁是低矮的日式房屋。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "house".into(),
                target_location_id: "iwakura_front_yard".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "station".into(),
                target_location_id: "miyanosaka_station".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "park".into(),
                target_location_id: "miyanosaka_park".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "center_park".into(),
                target_location_id: "miyasaka_center_park".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 10. 宫之坂站 ---
    let miyanosaka_station = Location {
        id: "miyanosaka_station".into(),
        name: "宫之坂站".into(),
        description: "一个小型电车站，月台上没有多少乘客。".into(),
        pois: vec![Poi {
            id: "ticket_machine".into(),
            name: "购票机".into(),
            description: "自动售票机。".into(),
            examine_action_id: Some("use_ticket_machine".into()),
            view_scene_id: None,
        }],
        connections: vec![
            Connection {
                action_id: "street".into(),
                target_location_id: "miyanosaka_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 11. 宫之坂公园 ---
    let miyanosaka_park = Location {
        id: "miyanosaka_park".into(),
        name: "宫之坂公园".into(),
        description: "附近居民常来的小公园，有几张长椅和儿童游乐设施。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "street".into(),
                target_location_id: "miyanosaka_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 12. 宫之坂中心公园 ---
    let miyasaka_center_park = Location {
        id: "miyasaka_center_park".into(),
        name: "宫之坂中心公园".into(),
        description: "比小公园大一些，有喷泉和更多的绿地。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "street".into(),
                target_location_id: "miyanosaka_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    locations.push(miyanosaka_street);
    locations.push(miyanosaka_station);
    locations.push(miyanosaka_park);
    locations.push(miyasaka_center_park);

    locations
}
