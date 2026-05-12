use crate::world::location::{Connection, Location, Poi};

pub fn build_iwakura(s: &dyn Fn(&str, &str) -> String) -> Vec<Location> {
    let mut locations = Vec::new();

    // --- 1. 前院 ---
    let front_yard = Location {
        id: "iwakura_front_yard".into(),
        name: s("MAP_LOCATION_FRONT_YARD_NAME", "前院"),
        description: s("MAP_LOCATION_FRONT_YARD_DESC", "岩仓家的前院。"),
        pois: vec![
            Poi {
                id: "mailbox".into(),
                name: s("MAP_POI_FRONT_YARD_MAILBOX_NAME", "邮箱"),
                description: s("MAP_POI_FRONT_YARD_MAILBOX_DESC", "一个老旧的邮箱。"),
                examine_action_id: None,
                view_scene_id: Some("SCENE_EXAMINE_MAILBOX".into()),
            },
            Poi {
                id: "doorbell".into(),
                name: s("MAP_POI_FRONT_YARD_DOORBELL_NAME", "门铃"),
                description: s("MAP_POI_FRONT_YARD_DOORBELL_DESC", "门铃按钮。"),
                examine_action_id: None,
                view_scene_id: Some("SCENE_EXAMINE_DOORBELL".into()),
            },
        ],
        connections: vec![
            Connection {
                action_id: "house".into(),
                target_location_id: "iwakura_lower_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_LOWER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "street".into(),
                target_location_id: "miyanosaka_street".into(),
                target_scene_id: None,
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 2. 下走廊 ---
    let lower_hallway = Location {
        id: "iwakura_lower_hallway".into(),
        name: s("MAP_LOCATION_LOWER_HALLWAY_NAME", "下走廊"),
        description: s("MAP_LOCATION_LOWER_HALLWAY_DESC", "一楼的走廊。"),
        pois: vec![
            Poi {
                id: "shoe_rack".into(),
                name: s("MAP_POI_LOWER_HALLWAY_SHOE_RACK_NAME", "鞋架"),
                description: s("MAP_POI_LOWER_HALLWAY_SHOE_RACK_DESC", "放满了家人的鞋子。"),
                examine_action_id: None,
                view_scene_id: Some("SCENE_EXAMINE_SHOE_RACK".into()),
            },
            Poi {
                id: "telephone".into(),
                name: s("MAP_POI_LOWER_HALLWAY_TELEPHONE_NAME", "电话"),
                description: s("MAP_POI_LOWER_HALLWAY_TELEPHONE_DESC", "一台老式电话。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "umbrella_stand".into(),
                name: s("MAP_POI_LOWER_HALLWAY_UMBRELLA_STAND_NAME", "伞架"),
                description: s("MAP_POI_LOWER_HALLWAY_UMBRELLA_STAND_DESC", "几把折叠伞。"),
                examine_action_id: None,
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "outside".into(),
                target_location_id: "iwakura_front_yard".into(),
                target_scene_id: Some("SCENE_IWAKURA_FRONT_YARD".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "living_area".into(),
                target_location_id: "iwakura_living_dining_kitchen".into(),
                target_scene_id: Some("SCENE_02_DOWNSTAIRS".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "bathroom".into(),
                target_location_id: "iwakura_bathroom".into(),
                target_scene_id: Some("SCENE_IWAKURA_BATHROOM".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "upstairs".into(),
                target_location_id: "iwakura_upper_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_UPPER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
            Connection {
                action_id: "study".into(),
                target_location_id: "iwakura_study".into(),
                target_scene_id: Some("SCENE_IWAKURA_STUDY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 3. 客厅-餐厅-厨房 ---
    let living_dining_kitchen = Location {
        id: "iwakura_living_dining_kitchen".into(),
        name: s("MAP_LOCATION_LIVING_DINING_KITCHEN_NAME", "客厅/厨房"),
        description: s("MAP_LOCATION_LIVING_DINING_KITCHEN_DESC", "客厅连接着开放式厨房。"),
        pois: vec![
            Poi {
                id: "sofa".into(),
                name: s("MAP_POI_LIVING_DINING_KITCHEN_SOFA_NAME", "沙发"),
                description: s("MAP_POI_LIVING_DINING_KITCHEN_SOFA_DESC", "一张旧沙发。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "tv".into(),
                name: s("MAP_POI_LIVING_DINING_KITCHEN_TV_NAME", "电视"),
                description: s("MAP_POI_LIVING_DINING_KITCHEN_TV_DESC", "老式显像管电视。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "dining_table".into(),
                name: s("MAP_POI_LIVING_DINING_KITCHEN_DINING_TABLE_NAME", "餐桌"),
                description: s("MAP_POI_LIVING_DINING_KITCHEN_DINING_TABLE_DESC", "四人座餐桌。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "refrigerator".into(),
                name: s("MAP_POI_LIVING_DINING_KITCHEN_REFRIGERATOR_NAME", "冰箱"),
                description: s("MAP_POI_LIVING_DINING_KITCHEN_REFRIGERATOR_DESC", "双门冰箱。"),
                examine_action_id: None,
                view_scene_id: Some("SCENE_EXAMINE_FRIDGE".into()),
            },
            Poi {
                id: "dad".into(),
                name: s("MAP_POI_LIVING_DINING_KITCHEN_DAD_NAME", "爸爸"),
                description: s("MAP_POI_LIVING_DINING_KITCHEN_DAD_DESC", "爸爸坐在沙发上。"),
                examine_action_id: Some("talk_to_dad".into()),
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "iwakura_lower_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_LOWER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 4. 浴室 ---
    let bathroom = Location {
        id: "iwakura_bathroom".into(),
        name: s("MAP_LOCATION_BATHROOM_NAME", "浴室"),
        description: s("MAP_LOCATION_BATHROOM_DESC", "家里有浴缸的浴室。"),
        pois: vec![
            Poi {
                id: "sink".into(),
                name: s("MAP_POI_BATHROOM_SINK_NAME", "洗手池"),
                description: s("MAP_POI_BATHROOM_SINK_DESC", "白色陶瓷洗手池。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "bathtub".into(),
                name: s("MAP_POI_BATHROOM_BATHTUB_NAME", "浴缸"),
                description: s("MAP_POI_BATHROOM_BATHTUB_DESC", "一个旧浴缸。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "mirror".into(),
                name: s("MAP_POI_BATHROOM_MIRROR_NAME", "镜子"),
                description: s("MAP_POI_BATHROOM_MIRROR_DESC", "镜子上有水渍。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "shower".into(),
                name: s("MAP_POI_BATHROOM_SHOWER_NAME", "淋浴"),
                description: s("MAP_POI_BATHROOM_SHOWER_DESC", "淋浴喷头。"),
                examine_action_id: None,
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "iwakura_lower_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_LOWER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 5. 上走廊 ---
    let upper_hallway = Location {
        id: "iwakura_upper_hallway".into(),
        name: s("MAP_LOCATION_UPPER_HALLWAY_NAME", "上走廊"),
        description: s("MAP_LOCATION_UPPER_HALLWAY_DESC", "二楼的走廊，尽头是lain的房间。"),
        pois: vec![
            Poi {
                id: "painting".into(),
                name: s("MAP_POI_UPPER_HALLWAY_PAINTING_NAME", "油画"),
                description: s("MAP_POI_UPPER_HALLWAY_PAINTING_DESC", "一幅看不懂的抽象画。"),
                examine_action_id: None,
                view_scene_id: None,
            },
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
            Connection {
                action_id: "enter_mika_room".into(),
                target_location_id: "iwakura_mikas_room".into(),
                target_scene_id: Some("SCENE_IWAKURA_MIKAS_ROOM".into()),
                requires_flag: None,
                denied_scene_id: Some("SCENE_MIKA_ROOM_LOCKED".into()),
            },
        ],
    };

    // --- 6. Lain 的房间 ---
    let lain_room = Location {
        id: "iwakura_lains_room".into(),
        name: s("MAP_LOCATION_LAINS_ROOM_NAME_IWAKURA", "lain的房间"),
        description: s("MAP_LOCATION_LAINS_ROOM_DESC_IWAKURA", "房间里很暗，只有Navi屏幕的微光。"),
        pois: vec![
            Poi {
                id: "navi_computer".into(),
                name: s("MAP_POI_LAINS_ROOM_NAVI_COMPUTER_NAME", "Navi"),
                description: s("MAP_POI_LAINS_ROOM_NAVI_COMPUTER_DESC", "Lain 的个人电脑。"),
                examine_action_id: Some("use_phone_navi".into()),
                view_scene_id: None,
            },
            Poi {
                id: "navi_mini".into(),
                name: s("MAP_POI_LAIN_ROOM_PC_NAME", "Navi Mini"),
                description: s("MAP_POI_LAIN_ROOM_PC_DESC", "桌面电脑。"),
                examine_action_id: Some("use_desktop_navi".into()),
                view_scene_id: None,
            },
            Poi {
                id: "bed".into(),
                name: s("MAP_POI_LAINS_ROOM_BED_NAME_IWAKURA", "床"),
                description: s("MAP_POI_LAINS_ROOM_BED_DESC_IWAKURA", "单人床，被子凌乱。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "window".into(),
                name: s("MAP_POI_LAINS_ROOM_WINDOW_NAME", "窗户"),
                description: s("MAP_POI_LAINS_ROOM_WINDOW_DESC", "窗外是夜空。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "toy_dog".into(),
                name: s("MAP_POI_LAINS_ROOM_TOY_DOG_NAME", "玩具狗"),
                description: s("MAP_POI_LAINS_ROOM_TOY_DOG_DESC", "一只毛绒玩具狗。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "bookshelf".into(),
                name: s("MAP_POI_LAINS_ROOM_BOOKSHELF_NAME_IWAKURA", "书架"),
                description: s("MAP_POI_LAINS_ROOM_BOOKSHELF_DESC_IWAKURA", "塞满了电脑杂志。"),
                examine_action_id: Some("examine_bookshelf".into()),
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "upper_hallway".into(),
                target_location_id: "iwakura_upper_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_UPPER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 7. 美香的房间 ---
    let mikas_room = Location {
        id: "iwakura_mikas_room".into(),
        name: s("MAP_LOCATION_MIKAS_ROOM_NAME", "美香的房间"),
        description: s("MAP_LOCATION_MIKAS_ROOM_DESC", "姐姐美香的房间。"),
        pois: vec![
            Poi {
                id: "desk".into(),
                name: s("MAP_POI_MIKAS_ROOM_DESK_NAME", "书桌"),
                description: s("MAP_POI_MIKAS_ROOM_DESK_DESC", "书桌上放着化妆品。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "wardrobe".into(),
                name: s("MAP_POI_MIKAS_ROOM_WARDROBE_NAME", "衣柜"),
                description: s("MAP_POI_MIKAS_ROOM_WARDROBE_DESC", "姐姐的衣柜。"),
                examine_action_id: Some("examine_mika_wardrobe".into()),
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "upper_hallway".into(),
                target_location_id: "iwakura_upper_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_UPPER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    // --- 8. 书房 ---
    let study = Location {
        id: "iwakura_study".into(),
        name: s("MAP_LOCATION_STUDY_NAME", "书房"),
        description: s("MAP_LOCATION_STUDY_DESC", "爸爸的书房。"),
        pois: vec![
            Poi {
                id: "bookshelf".into(),
                name: s("MAP_POI_STUDY_BOOKSHELF_NAME", "书架"),
                description: s("MAP_POI_STUDY_BOOKSHELF_DESC", "专业书籍。"),
                examine_action_id: None,
                view_scene_id: None,
            },
            Poi {
                id: "desk".into(),
                name: s("MAP_POI_STUDY_DESK_NAME", "书桌"),
                description: s("MAP_POI_STUDY_DESK_DESC", "爸爸的工作台。"),
                examine_action_id: None,
                view_scene_id: None,
            },
        ],
        connections: vec![
            Connection {
                action_id: "hallway".into(),
                target_location_id: "iwakura_lower_hallway".into(),
                target_scene_id: Some("SCENE_IWAKURA_LOWER_HALLWAY".into()),
                requires_flag: None,
                denied_scene_id: None,
            },
        ],
    };

    locations.push(front_yard);
    locations.push(lower_hallway);
    locations.push(living_dining_kitchen);
    locations.push(bathroom);
    locations.push(upper_hallway);
    locations.push(lain_room);
    locations.push(mikas_room);
    locations.push(study);

    locations
}
