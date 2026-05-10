//! 动作执行器 —— 将 action_id 解析为 Command 列表
//!
//! 关键设计：
//! 1. 纯函数：输入 (GameState, action_id) → 输出 Vec<Command>
//! 2. 不修改状态，无副作用
//! 3. 默认 fallback：如果 action_id 没有特殊处理，且当前选项声明了 target_scene，则生成 TransitionTo 命令

use crate::engine::state::GameState;
use super::command::Command;
use super::scene::Scene;

/// 执行一个动作，返回命令列表
///
/// # 设计说明
/// 与 C 版本不同，这个函数不递归调用自己。
/// 如果角色模块需要触发额外动作，它应该返回额外的 Command，
/// 由调用方统一 apply。
pub fn resolve_action(action_id: &str, state: &GameState, current_scene: &Scene) -> Vec<Command> {
    use Command::*;

    // --- 特殊系统动作 ---
    match action_id {
        "use_phone_navi" => return vec![EnterNavi],
        "use_desktop_navi" => return vec![EnterNaviMini],
        "exit_story" => return vec![NoOp],
        "use_ticket_machine" => return vec![EnterTrain],
        _ => {}
    }

    // --- 复杂的地图移动 + 场景切换 ---
    match action_id {
        "go_back_to_shibuya" => {
            return vec![
                MoveTo("shibuya_street".into()),
                TransitionTo("SCENE_09_CYBERIA".into()),
            ];
        }
        "go_to_shinjuku_site" => {
            return vec![
                MoveTo("shinjuku_abandoned_site".into()),
                TransitionTo("SCENE_SHINJUKU_ABANDONED_SITE".into()),
            ];
        }
        "explore_shinjuku_site" => {
            if state.get_flag("door_opened_by_ghost") != Some("1") {
                return vec![
                    SetFlag { key: "sister_mood".into(), value: "cold".into() },
                    SetFlag { key: "door_opened_by_ghost".into(), value: "1".into() },
                    TransitionTo("SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE".into()),
                ];
            } else {
                return vec![TransientMessage("这里已经什么都没有了。".into())];
            }
        }
        "read_email_from_chisa" => {
            return vec![
                UnlockCommand("mail".into()),
                TransitionTo("SCENE_SIDE_STORIES_EMAIL_CLIENT".into()),
            ];
        }
        "gunshot_exit" => {
            return vec![
                TransitionTo("SCENE_00_ENTRY".into()),
                // 时间重置由 Apply 层处理
                AdvanceTime(0), // 占位，实际在 apply 层特殊处理
                SetFlag { key: "TIME_GLITCH_ACTIVE".into(), value: "0".into() },
            ];
        }
        _ => {}
    }

    // --- 物品操作 ---
    match action_id {
        "order_milk" => return vec![AcquireItem { item_id: "milk".into(), quantity: 1 }],
        "order_coffee" => return vec![AcquireItem { item_id: "coffee".into(), quantity: 1 }],
        "order_juice" => return vec![AcquireItem { item_id: "juice".into(), quantity: 1 }],
        "take_milk_from_fridge" => {
            return vec![
                AcquireItem { item_id: "milk".into(), quantity: 1 },
                TransitionTo("SCENE_EXAMINE_FRIDGE".into()),
            ];
        }
        _ => {}
    }

    // --- 网络协议切换 ---
    match action_id {
        "toggle_ipv4" => {
            let new_val = if state.get_flag("network_status.protocols.ipv4") == Some("on") { "off" } else { "on" };
            return vec![SetFlag { key: "network_status.protocols.ipv4".into(), value: new_val.into() }];
        }
        "toggle_ipv6" => {
            let new_val = if state.get_flag("network_status.protocols.ipv6") == Some("on") { "off" } else { "on" };
            return vec![SetFlag { key: "network_status.protocols.ipv6".into(), value: new_val.into() }];
        }
        _ => {}
    }

    // --- 标准故事跳转表 ---
    // 这些是从 .ssl 的 target_scene 和常见副作用中提取的
    let standard = match action_id {
        "prologue_go_downstairs" => Some(("SCENE_02_DOWNSTAIRS", None)),
        "open_door_broken" => Some(("SCENE_01_LAIN_ROOM_BROKEN", None)),
        "upstairs" => Some(("SCENE_IWAKURA_UPPER_HALLWAY", None)),
        "lains_room" => Some(("SCENE_IWAKURA_LAINS_ROOM", None)),
        "talk_to_figure" => Some(("SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE", Some(("sister_mood", "cold")))),
        "navi_shutdown" => Some(("SCENE_01B_NAVI_SHUTDOWN", Some(("sister_mood", "curious")))),
        "navi_reboot" => Some(("SCENE_01D_NAVI_REBOOT_ENDPROLOGUE", Some(("sister_mood", "curious")))),
        "navi_connect" => Some(("SCENE_01E_NAVI_CONNECT_ENDPROLOGUE", Some(("sister_mood", "curious")))),
        "dad_reply_no" => Some(("SCENE_02B_DAD_REPLY_NO", None)),
        "dad_ask_help" => Some(("SCENE_02C_DAD_ASK_HELP", None)),
        "get_milk" => Some(("SCENE_02J_GET_MILK_ENDPROLOGUE", Some(("sister_mood", "normal")))),
        "mom_reply_fine" => Some(("SCENE_02F_MOM_REPLY_FINE_ENDPROLOGUE", Some(("sister_mood", "normal")))),
        "mom_reply_silent" => Some(("SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE", Some(("sister_mood", "cold")))),
        "start_chapter_one" => Some(("SCENE_03_CHAPTER_ONE_INTRO", None)),
        "talk_to_dad" => Some(("SCENE_DAD_HUB", None)),
        "talk_to_sister_cold" => Some(("SCENE_04A_TALK_TO_SISTER_COLD", None)),
        "talk_to_sister_curious" => Some(("SCENE_04B_TALK_TO_SISTER_CURIOUS", None)),
        "talk_to_sister_default" => Some(("SCENE_04C_TALK_TO_SISTER_DEFAULT", None)),
        "go_to_school" => Some(("SCENE_06_TRAIN_SCENE", None)),
        "go_to_classroom" => Some(("SCENE_07_CLASSROOM", None)),
        "ask_teacher_knows" => Some(("SCENE_08B_ASK_TEACHER", Some(("asked_teacher", "1")))),
        "ask_about_proxy" => Some(("SCENE_08C_ASK_PROXY", Some(("asked_proxy", "1")))),
        "ask_about_chisa" => Some(("SCENE_08D_ASK_CHISA", Some(("asked_chisa", "1")))),
        "go_to_bar" => Some(("SCENE_09_CYBERIA", None)),
        "persuade_to_bar" => Some(("SCENE_09A_PERSUASION", None)),
        "ask_alice_scared" => Some(("SCENE_08E_ASK_ALICE_SCARED", None)),
        "active_overload" => Some(("SCENE_06Z_TRAIN_EVENT_RESULT", Some(("overload_result", "active")))),
        "passive_overload" => Some(("SCENE_06Z_TRAIN_EVENT_RESULT", Some(("overload_result", "passive")))),
        "examine_bookshelf" => Some(("SCENE_EXAMINE_BOOKSHELF", None)),
        "examine_mika_wardrobe" => Some(("SCENE_EXAMINE_MIKA_WARDROBE", None)),
        "set_font_speed_fast" => Some(("SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", Some(("typewriter_delay", "0.02")))),
        "set_font_speed_normal" => Some(("SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", Some(("typewriter_delay", "0.04")))),
        "set_font_speed_slow" => Some(("SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL", Some(("typewriter_delay", "0.07")))),
        "connect_to_regional" => Some(("SCENE_SIDE_STORIES_NETWORK_STATUS", Some(("network_status.scope", "地区局域网")))),
        "connect_to_national" => Some(("SCENE_SIDE_STORIES_NETWORK_STATUS", Some(("network_status.scope", "全国互联网")))),
        _ => None,
    };

    if let Some((scene, flag)) = standard {
        let mut cmds = vec![];
        if let Some((k, v)) = flag {
            cmds.push(SetFlag { key: k.into(), value: v.into() });
        }
        cmds.push(TransitionTo(scene.into()));
        return cmds;
    }

    // --- Fallback：检查当前场景的选项声明的 target_scene ---
    if let Some(choice) = current_scene.find_choice(action_id) {
        if let Some(ref target) = choice.target_scene {
            return vec![TransitionTo(target.clone())];
        }
    }

    // 未知动作
    vec![NoOp]
}

/// 处理数字选项选择（1, 2, 3...）
/// 返回 (命令列表, 选中的选项索引)
pub fn resolve_numeric_choice(
    num: usize,
    state: &GameState,
    scene: &Scene,
    elapsed_ms: u64,
) -> Option<(Vec<Command>, usize)> {
    use super::conditions::check_all;

    let mut visible_idx = 0;
    for (i, choice) in scene.choices.iter().enumerate() {
        // 时间锁：选项延迟出现
        if elapsed_ms < choice.delay_ms() {
            continue;
        }
        // 条件锁
        if !check_all(&choice.conditions, state) {
            continue;
        }
        visible_idx += 1;
        if visible_idx == num {
            let cmds = resolve_action(&choice.action_id, state, scene);
            return Some((cmds, i));
        }
    }
    None
}

// =============================================================================
// 测试 —— 这是 Rust 版本最爽的部分：纯函数，直接 assert_eq! 命令列表
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::narrative::scene::Scene;

    #[test]
    fn test_standard_transition() {
        let state = GameState::default();
        let scene = Scene::from_yaml("scene_id: TEST\nname_text_id: T\nlocation_id: test").unwrap();
        let cmds = resolve_action("open_door_broken", &state, &scene);
        assert_eq!(cmds, vec![Command::TransitionTo("SCENE_01_LAIN_ROOM_BROKEN".into())]);
    }

    #[test]
    fn test_transition_with_flag() {
        let state = GameState::default();
        let scene = Scene::from_yaml("scene_id: TEST\nname_text_id: T\nlocation_id: test").unwrap();
        let cmds = resolve_action("talk_to_figure", &state, &scene);
        assert_eq!(cmds, vec![
            Command::SetFlag { key: "sister_mood".into(), value: "cold".into() },
            Command::TransitionTo("SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE".into()),
        ]);
    }

    #[test]
    fn test_conditional_explore_first_time() {
        let state = GameState::default(); // flag "door_opened_by_ghost" 未设置
        let scene = Scene::from_yaml("scene_id: TEST\nname_text_id: T\nlocation_id: test").unwrap();
        let cmds = resolve_action("explore_shinjuku_site", &state, &scene);
        assert_eq!(cmds.len(), 3);
        assert!(matches!(cmds[2], Command::TransitionTo(ref s) if s == "SCENE_00A_WAIT_ONE_MINUTE_ENDPROLOGUE"));
    }

    #[test]
    fn test_conditional_explore_second_time() {
        let mut state = GameState::default();
        state.set_flag("door_opened_by_ghost", "1");
        let scene = Scene::from_yaml("scene_id: TEST\nname_text_id: T\nlocation_id: test").unwrap();
        let cmds = resolve_action("explore_shinjuku_site", &state, &scene);
        assert_eq!(cmds, vec![Command::TransientMessage("这里已经什么都没有了。".into())]);
    }

    #[test]
    fn test_fallback_to_scene_choice() {
        let state = GameState::default();
        let yaml = r#"
scene_id: TEST
name_text_id: T
location_id: test
choices:
  - action_id: custom_action
    text_id: TEXT_CUSTOM
    target_scene: SCENE_CUSTOM_TARGET
"#;
        let scene = Scene::from_yaml(yaml).unwrap();
        // "custom_action" 不在 executor 的硬编码表中
        let cmds = resolve_action("custom_action", &state, &scene);
        assert_eq!(cmds, vec![Command::TransitionTo("SCENE_CUSTOM_TARGET".into())]);
    }

    #[test]
    fn test_numeric_choice_selection() {
        let mut state = GameState::default();
        state.set_flag("has_key", "1");

        let yaml = r#"
scene_id: TEST
name_text_id: T
location_id: test
choices:
  - action_id: open_door
    text_id: TEXT_OPEN
    conditions:
      - requires_flag: has_key
        flag_value: "1"
  - action_id: kick_door
    text_id: TEXT_KICK
"#;
        let scene = Scene::from_yaml(yaml).unwrap();

        // 第一个可见选项是 "open_door"（条件满足）
        let result = resolve_numeric_choice(1, &state, &scene, 0);
        assert!(result.is_some());
        let (cmds, idx) = result.unwrap();
        assert_eq!(idx, 0);
        assert_eq!(cmds, vec![Command::NoOp]); // 没有 target_scene，fallback 到 NoOp
    }

    #[test]
    fn test_numeric_choice_with_delay() {
        let state = GameState::default();
        let yaml = r#"
scene_id: TEST
name_text_id: T
location_id: test
choices:
  - action_id: instant
    text_id: TEXT_INSTANT
    delay: 0.0
  - action_id: delayed
    text_id: TEXT_DELAYED
    delay: 5.0
"#;
        let scene = Scene::from_yaml(yaml).unwrap();

        // 0 秒时，只有 instant 可见
        let result = resolve_numeric_choice(1, &state, &scene, 0);
        assert!(result.is_some());
        let (_, idx) = result.unwrap();
        assert_eq!(idx, 0); // instant

        // 0 秒时，delayed 不可见，所以输入 "2" 应该找不到
        let result2 = resolve_numeric_choice(2, &state, &scene, 0);
        assert!(result2.is_none());
    }
}
