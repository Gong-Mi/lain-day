//! 主应用循环 —— 替代 C 版本的 main.c
//!
//! 职责：
//! 1. 初始化终端和资源
//! 2. 运行主循环：渲染 → 输入 → 执行 → 切换
//! 3. 清理终端

use std::io::{self, Write};
use std::path::PathBuf;
use std::time::{Duration, Instant};

use crossterm::event::{KeyCode, KeyEvent};

use crate::assets::Assets;
use crate::bgm::BgmManager;
use crate::engine::state::GameState;
use crate::engine::time;
use crate::narrative::command::{apply_commands, Command};
use crate::narrative::executor::resolve_numeric_choice;
use crate::render::Renderer;
use crate::systems::navi_mini;
use crate::systems::boot::BootConfig;
use crate::world::location::{Connection, Location};

#[derive(Clone)]
pub struct App {
    assets: Assets,
    state: GameState,
    renderer: Renderer,
    scene_start: Instant,
    running: bool,
    #[allow(dead_code)]
    bgm_manager: BgmManager,
}

impl App {
    pub fn new(data_dir: PathBuf, boot_config: Option<&BootConfig>) -> Result<Self, Box<dyn std::error::Error>> {
        let assets = Assets::load_from_data_dir(&data_dir)?;
        let mut state = GameState::default();
        if let Some(cfg) = boot_config {
            state.session_name = cfg.session_name.clone();
            state.set_flag("lang", match cfg.lang {
                crate::systems::boot::Lang::English => "en",
                crate::systems::boot::Lang::Chinese => "zh",
            });
        }
        let mut renderer = Renderer::default();
        renderer.typewriter_delay =
            Duration::from_secs_f32(state.typewriter_delay);

        let bgm_manager = BgmManager::new(&data_dir)?;

        Ok(Self {
            assets,
            state,
            renderer,
            scene_start: Instant::now(),
            running: true,
            bgm_manager,
        })
    }

    pub fn run(&mut self) -> io::Result<()> {
        self.renderer.enter()?;
        self.init_map();

        while self.running {
            // 处理 resize
            if let Some((w, h)) = self.renderer.poll_resize()? {
                self.renderer.term_size = (w, h);
            }

            let scene = match self.assets.get_scene(&self.state.current_scene) {
                Some(s) => s.clone(),
                None => {
                    self.handle_missing_scene()?;
                    continue;
                }
            };

            let elapsed = self.scene_start.elapsed().as_millis() as u64;

            // 渲染
            self.renderer
                .render_scene(&scene, &self.assets, &self.state, elapsed)?;

            // 处理输入
            if crossterm::event::poll(Duration::from_millis(16))? {
                match crossterm::event::read()? {
                    crossterm::event::Event::Key(key) => {
                        // Takeover 模式下，如果对话还没放完，抑制输入
                        if scene.is_takeover {
                            let all_lines_done = scene
                                .dialogue
                                .iter()
                                .all(|line| elapsed >= line.delay_ms());
                            if !all_lines_done {
                                if key.code == KeyCode::Char('q') {
                                    self.running = false;
                                }
                                continue;
                            }
                        }
                        self.handle_input(key, &scene)?;
                    }
                    crossterm::event::Event::Mouse(mouse) => {
                        self.handle_mouse(mouse, &scene, elapsed)?;
                    }
                    _ => {}
                }
            }

            std::thread::sleep(Duration::from_millis(16));
        }

        self.renderer.exit()?;
        Ok(())
    }

    fn handle_input(&mut self, key: KeyEvent, scene: &crate::narrative::scene::Scene) -> io::Result<()> {
        match key.code {
            KeyCode::Char('q') | KeyCode::Char('Q') => {
                self.running = false;
            }
            KeyCode::Char(c) if c.is_ascii_digit() => {
                let num = c.to_digit(10).unwrap() as usize;
                if num > 0 {
                    self.handle_choice(num, scene)?;
                }
            }
            KeyCode::Enter => {}
            _ => {}
        }
        Ok(())
    }

    fn handle_mouse(
        &mut self,
        mouse: crossterm::event::MouseEvent,
        scene: &crate::narrative::scene::Scene,
        elapsed_ms: u64,
    ) -> io::Result<()> {
        use crossterm::event::{MouseButton, MouseEventKind};

        if matches!(mouse.kind, MouseEventKind::Down(MouseButton::Left)) {
            // Takeover 模式下，如果对话还没放完，忽略鼠标
            if scene.is_takeover {
                let all_lines_done = scene
                    .dialogue
                    .iter()
                    .all(|line| elapsed_ms >= line.delay_ms());
                if !all_lines_done {
                    return Ok(());
                }
            }

            // 查找点击了哪个选项
            let y = mouse.row;
            for pos in &self.renderer.choice_positions {
                if y == pos.row {
                    self.handle_choice(pos.visible_index, scene)?;
                    break;
                }
            }
        }
        Ok(())
    }

    fn handle_choice(
        &mut self,
        num: usize,
        scene: &crate::narrative::scene::Scene,
    ) -> io::Result<()> {
        let elapsed = self.scene_start.elapsed().as_millis() as u64;

        if let Some((cmds, _idx)) = resolve_numeric_choice(num, &self.state, scene, elapsed) {
            // 应用时间成本
            if let Some(choice) = scene.choices.get(num - 1) {
                self.apply_time_cost(&choice.action_id);
            }

            // 应用命令
            apply_commands(&cmds, &mut self.state);

            // 同步 renderer 的 typewriter delay
            self.renderer.typewriter_delay =
                Duration::from_secs_f32(self.state.typewriter_delay);

            // 场景切换时重置计时并切换 BGM
            if cmds.iter().any(|c| matches!(c, Command::TransitionTo(_))) {
                self.scene_start = Instant::now();
                let _ = self.bgm_manager.on_scene_changed(&self.state.current_scene);
            }

            // 处理子系统命令
            for cmd in &cmds {
                match cmd {
                    Command::EnterNaviMini => {
                        let result = navi_mini::run_navi_mini(&self.assets, &mut self.state)?;
                        navi_mini::exit_navi()?;
                        match result {
                            navi_mini::NaviResult::ReturnTo(scene) => {
                                self.state.current_scene = scene;
                                self.scene_start = Instant::now();
                            }
                            navi_mini::NaviResult::OpenBrowser => {
                                crate::systems::browser::run_browser(&self.assets)?;
                                self.renderer.clear()?;
                            }
                            navi_mini::NaviResult::ViewFiles => {
                                crate::systems::file_manager::run_file_manager(&self.assets)?;
                                self.renderer.clear()?;
                            }
                            navi_mini::NaviResult::NoOp => {}
                        }
                    }
                    Command::EnterTrain => {
                        crate::systems::train::run_ticket_machine()?;
                        // 恢复主界面
                        self.renderer.clear()?;
                    }
                    Command::EnterMystery => {
                        let _solved = crate::systems::mystery::run_mystery_app()?;
                        self.renderer.clear()?;
                    }
                    Command::EnterMail => {
                        let maildir = std::path::PathBuf::from("data/mail");
                        let mut mailbox = crate::systems::mail::Mailbox::new();
                        let _ = mailbox.load_from_dir(&maildir);
                        crate::systems::mail::run_mail_app(&mut mailbox, &maildir)?;
                        self.renderer.clear()?;
                    }
                    _ => {}
                }
            }
        }

        Ok(())
    }

    fn apply_time_cost(&mut self, action_id: &str) {
        let minutes = match action_id {
            "wait_one_minute" => 1,
            "talk_to_dad" | "talk_to_mom" | "talk_to_sister" => 5,
            "get_milk" | "take_milk_from_fridge" => 3,
            "downstairs" | "upstairs" | "lains_room" | "upper_hallway"
            | "living_area" | "hallway" | "outside" | "house"
            | "enter_mika_room" | "bathroom" | "study"
            | "go_to_park" | "go_to_center_park" | "return_to_street" => 1,
            "shibuya" | "home" => 25,
            "shinjuku_site" => 30,
            "cyberia" => 15,
            _ => 0,
        };

        if minutes > 0 {
            let units = minutes * 60 * 16;
            let decoded = time::decode(self.state.time_of_day);
            self.state.time_of_day = time::encode(decoded.data + units as u32);
        }
    }

    fn handle_missing_scene(&mut self) -> io::Result<()> {
        self.renderer.clear()?;
        let mut stdout = io::stdout();
        writeln!(
            stdout,
            "ERROR: Scene '{}' not found.\nPress 'q' to quit.",
            self.state.current_scene
        )?;
        stdout.flush()?;

        loop {
            let key = self.renderer.read_key()?;
            if key.code == KeyCode::Char('q') || key.code == KeyCode::Char('Q') {
                self.running = false;
                return Ok(());
            }
        }
    }

    fn init_map(&mut self) {
        let upper = Location {
            id: "iwakura_upper_hallway".into(),
            name: "上走廊".into(),
            description: "二楼的走廊，尽头是lain的房间。".into(),
            pois: vec![],
            connections: vec![
                Connection {
                    action_id: "downstairs".into(),
                    target_location_id: "iwakura_living_dining_kitchen".into(),
                    target_scene_id: Some("SCENE_02_DOWNSTAIRS".into()),
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
        };

        let lower = Location {
            id: "iwakura_living_dining_kitchen".into(),
            name: "客厅/厨房".into(),
            description: "客厅连接着开放式厨房。".into(),
            pois: vec![],
            connections: vec![
                Connection {
                    action_id: "upstairs".into(),
                    target_location_id: "iwakura_upper_hallway".into(),
                    target_scene_id: Some("SCENE_IWAKURA_UPPER_HALLWAY".into()),
                    requires_flag: None,
                    denied_scene_id: None,
                },
            ],
        };

        let lain_room = Location {
            id: "iwakura_lains_room".into(),
            name: "lain的房间".into(),
            description: "房间里很暗，只有Navi屏幕的微光。".into(),
            pois: vec![],
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

        self.assets.locations.insert(upper.id.clone(), upper);
        self.assets.locations.insert(lower.id.clone(), lower);
        self.assets.locations.insert(lain_room.id.clone(), lain_room);
    }
}

// =============================================================================
// 集成测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::narrative::command::Command;

    #[test]
    fn test_app_new_loads_data() {
        let app = App::new(PathBuf::from("data"), None).unwrap();
        assert!(!app.assets.strings.is_empty());
        assert!(!app.assets.scenes.is_empty());
        assert_eq!(app.state.current_scene, "SCENE_00_ENTRY");
    }

    #[test]
    fn test_time_cost_application() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        let before = app.state.time_of_day;
        app.apply_time_cost("downstairs");
        assert_ne!(app.state.time_of_day, before);
    }

    #[test]
    fn test_scene_transition_flow() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // 验证初始场景是 ENTRY
        assert_eq!(app.state.current_scene, "SCENE_00_ENTRY");

        // 模拟执行 "open_door_broken" 动作
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let cmds = crate::narrative::executor::resolve_action(
            "open_door_broken",
            &app.state,
            &scene,
        );
        apply_commands(&cmds, &mut app.state);

        assert_eq!(app.state.current_scene, "SCENE_01_LAIN_ROOM_BROKEN");
    }

    #[test]
    fn test_prologue_complete_flow() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // 1. 从 ENTRY 开始
        assert_eq!(app.state.current_scene, "SCENE_00_ENTRY");

        // 2. 选择 1: 打开房门
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(1, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_01_LAIN_ROOM_BROKEN");

        // 3. 选择 1: 使用 Navi（进入 PC NAVI DESKTOP）
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(1, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        // PC_NAVI_DESKTOP 没有 target_scene，action 是 EnterNaviMini
        // 当前实现中 EnterNaviMini 是 NoOp，所以场景不变
        // 这里验证命令类型即可
        assert!(cmds.iter().any(|c| matches!(c, Command::EnterNaviMini)));
    }

    #[test]
    fn test_takeover_scene_has_delays() {
        let app = App::new(PathBuf::from("data"), None).unwrap();
        let entry = app.assets.get_scene("SCENE_00_ENTRY").unwrap();
        assert!(entry.is_takeover);
        // 验证有多行带 delay 的对话
        assert!(entry.dialogue.len() > 1);
        assert!(entry.dialogue.iter().any(|l| l.delay > 0.0));
    }

    #[test]
    fn test_flag_based_branching() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // talk_to_figure 应该设置 sister_mood = cold
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let cmds = crate::narrative::executor::resolve_action(
            "talk_to_figure",
            &app.state,
            &scene,
        );
        apply_commands(&cmds, &mut app.state);

        assert_eq!(app.state.get_flag("sister_mood"), Some("cold"));
        assert_eq!(app.state.current_scene, "SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE");
    }

    #[test]
    fn test_entry_to_downstairs_flow() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // ENTRY → 下楼
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02_DOWNSTAIRS");

        // DOWNSTAIRS → 父亲
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(1, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_DAD_HUB");

        // DAD_HUB → DAD_REPLY_NO
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02B_DAD_REPLY_NO");
    }

    #[test]
    fn test_navi_branching_endings() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // ENTRY → LAIN_ROOM_BROKEN
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(1, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_01_LAIN_ROOM_BROKEN");

        // 测试 navi_shutdown 分支
        let mut app_shutdown = app.clone();
        let cmds = crate::narrative::executor::resolve_action(
            "navi_shutdown",
            &app_shutdown.state,
            &app_shutdown.assets.get_scene("SCENE_01_LAIN_ROOM_BROKEN").unwrap().clone(),
        );
        apply_commands(&cmds, &mut app_shutdown.state);
        assert_eq!(app_shutdown.state.current_scene, "SCENE_01B_NAVI_SHUTDOWN");
        assert_eq!(app_shutdown.state.get_flag("sister_mood"), Some("curious"));

        // 测试 navi_reboot 分支
        let mut app_reboot = app.clone();
        let cmds = crate::narrative::executor::resolve_action(
            "navi_reboot",
            &app_reboot.state,
            &app_reboot.assets.get_scene("SCENE_01_LAIN_ROOM_BROKEN").unwrap().clone(),
        );
        apply_commands(&cmds, &mut app_reboot.state);
        assert_eq!(app_reboot.state.current_scene, "SCENE_01D_NAVI_REBOOT_ENDPROLOGUE");
        assert_eq!(app_reboot.state.get_flag("sister_mood"), Some("curious"));

        // 测试 navi_connect 分支
        let mut app_connect = app.clone();
        let cmds = crate::narrative::executor::resolve_action(
            "navi_connect",
            &app_connect.state,
            &app_connect.assets.get_scene("SCENE_01_LAIN_ROOM_BROKEN").unwrap().clone(),
        );
        apply_commands(&cmds, &mut app_connect.state);
        assert_eq!(app_connect.state.current_scene, "SCENE_01E_NAVI_CONNECT_ENDPROLOGUE");
        assert_eq!(app_connect.state.get_flag("sister_mood"), Some("curious"));
    }

    #[test]
    fn test_mom_silent_branch() {
        let mut app = App::new(PathBuf::from("data"), None).unwrap();
        app.init_map();

        // ENTRY → DOWNSTAIRS
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);

        // DOWNSTAIRS → MOM_NORMAL
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02D_TALK_TO_MOM_NORMAL");
    }

    #[test]
    fn test_all_scenes_loadable() {
        let app = App::new(PathBuf::from("data"), None).unwrap();
        // 验证所有 executor 中引用的标准跳转目标都存在
        let required_scenes = vec![
            "SCENE_00_ENTRY",
            "SCENE_01_LAIN_ROOM_BROKEN",
            "SCENE_01B_NAVI_SHUTDOWN",
            "SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE",
            "SCENE_01D_NAVI_REBOOT_ENDPROLOGUE",
            "SCENE_01E_NAVI_CONNECT_ENDPROLOGUE",
            "SCENE_02_DOWNSTAIRS",
            "SCENE_02B_DAD_REPLY_NO",
            "SCENE_02C_DAD_ASK_HELP",
            "SCENE_02D_TALK_TO_MOM_NORMAL",
            "SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE",
            "SCENE_02J_GET_MILK_ENDPROLOGUE",
            "SCENE_03_CHAPTER_ONE_INTRO",
            "SCENE_04A_TALK_TO_SISTER_COLD",
            "SCENE_04B_TALK_TO_SISTER_CURIOUS",
            "SCENE_04C_TALK_TO_SISTER_DEFAULT",
            "SCENE_DAD_HUB",
            "SCENE_PC_NAVI_DESKTOP",
            "SCENE_IWAKURA_UPPER_HALLWAY",
            "SCENE_IWAKURA_LAINS_ROOM",
        ];

        for id in &required_scenes {
            assert!(
                app.assets.get_scene(id).is_some(),
                "Required scene {} is missing",
                id
            );
        }
    }
}
