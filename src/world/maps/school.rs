use crate::world::location::{Connection, Location};

pub fn build_school(_s: &dyn Fn(&str, &str) -> String) -> Vec<Location> {
    let mut locations = Vec::new();

    // --- 13. 六本木学校大门 ---
    let roppongi_school_gate = Location {
        id: "roppongi_school_gate".into(),
        name: "六本木中学·校门".into(),
        description: "学校的正门口，学生们在这里集合和分别。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "roppongi_school_hallway".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "street".into(),
                target_location_id: "roppongi_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 14. 六本木学校走廊 ---
    let roppongi_school_hallway = Location {
        id: "roppongi_school_hallway".into(),
        name: "六本木中学·走廊".into(),
        description: "铺着油毡地板的走廊，两侧是一间间教室。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "gate".into(),
                target_location_id: "roppongi_school_gate".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "classroom".into(),
                target_location_id: "roppongi_classroom".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "rooftop".into(),
                target_location_id: "roppongi_school_rooftop".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 15. 六本木学校天台 ---
    let roppongi_school_rooftop = Location {
        id: "roppongi_school_rooftop".into(),
        name: "六本木中学·天台".into(),
        description: "空旷的天台，风很大，可以看到远处的城市天际线。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "roppongi_school_hallway".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 16. 六本木教室 ---
    let roppongi_classroom = Location {
        id: "roppongi_classroom".into(),
        name: "六本木中学·教室".into(),
        description: "lain的教室。课桌整齐排列，黑板上还留着上一节课的板书。".into(),
        pois: vec![],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "roppongi_school_hallway".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    locations.push(roppongi_school_gate);
    locations.push(roppongi_school_hallway);
    locations.push(roppongi_school_rooftop);
    locations.push(roppongi_classroom);

    locations
}
