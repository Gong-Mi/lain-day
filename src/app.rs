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
use crate::engine::commands::{self};
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
    /// 命令输入模式
    command_mode: bool,
    /// 命令输入缓冲区
    command_buffer: String,
    /// 命令执行结果（等待显示）
    command_result: Option<Vec<String>>,
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
            command_mode: false,
            command_buffer: String::new(),
            command_result: None,
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

            // 检查自动事件
            if let Some(target) = check_auto_events(&mut self.state, &scene) {
                self.state.current_scene = target;
                self.scene_start = Instant::now();
                self.state.scene_entry_time = self.state.time_of_day;
                let _ = self.bgm_manager.on_scene_changed(&self.state.current_scene);
                continue;
            }

            let elapsed = self.scene_start.elapsed().as_millis() as u64;

            // 渲染
            self.renderer
                .render_scene(&scene, &self.assets, &self.state, elapsed)?;

            // 显示命令执行结果
            if let Some(lines) = self.command_result.take() {
                self.show_command_result(&lines)?;
                continue;
            }

            // 命令模式下在底部显示命令行
            if self.command_mode {
                self.render_command_line()?;
            }

            // 处理输入
            if crossterm::event::poll(Duration::from_millis(16))? {
                match crossterm::event::read()? {
                    crossterm::event::Event::Key(key) => {
                        // Takeover 模式下，如果对话还没放完，抑制输入
                        if scene.is_takeover && !self.command_mode {
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
                        if !self.command_mode {
                            self.handle_mouse(mouse, &scene, elapsed)?;
                        }
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
        if self.command_mode {
            match key.code {
                KeyCode::Esc => {
                    self.command_mode = false;
                    self.command_buffer.clear();
                }
                KeyCode::Enter => {
                    let input = self.command_buffer.clone();
                    self.command_buffer.clear();
                    self.command_mode = false;
                    self.execute_command(&input, scene)?;
                }
                KeyCode::Backspace => {
                    self.command_buffer.pop();
                }
                KeyCode::Char(c) => {
                    self.command_buffer.push(c);
                }
                _ => {}
            }
            return Ok(());
        }

        match key.code {
            KeyCode::Char('q') | KeyCode::Char('Q') => {
                self.running = false;
            }
            KeyCode::Char('/') | KeyCode::Char(':') => {
                self.command_mode = true;
                self.command_buffer.clear();
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
                self.state.scene_entry_time = self.state.time_of_day;
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
                                self.state.scene_entry_time = self.state.time_of_day;
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

    /// 在屏幕底部渲染命令输入行
    fn render_command_line(&self) -> io::Result<()> {
        use crossterm::cursor::MoveTo;
        use crossterm::ExecutableCommand;
        let (_, h) = self.renderer.term_size;
        let row = h.saturating_sub(1);
        let mut stdout = io::stdout();
        stdout.execute(MoveTo(0, row))?;
        print!("\x1b[7m> {}\x1b[0m", self.command_buffer);
        stdout.flush()?;
        Ok(())
    }

    /// 显示命令执行结果，等待按键继续
    fn show_command_result(&mut self, lines: &[String]) -> io::Result<()> {
        self.renderer.clear()?;
        let mut stdout = io::stdout();
        for line in lines {
            println!("{}", line);
        }
        println!("\n[按任意键继续]");
        stdout.flush()?;

        // 等待任意键
        loop {
            if let crossterm::event::Event::Key(_) = crossterm::event::read()? {
                break;
            }
        }
        Ok(())
    }

    /// 执行自由文本命令
    fn execute_command(&mut self, input: &str, scene: &crate::narrative::scene::Scene) -> io::Result<()> {
        let parsed = commands::parse(input);
        let output = commands::execute(&parsed, &mut self.state, &self.assets, scene);

        // 应用命令输出中的 Command
        if !output.commands.is_empty() {
            apply_commands(&output.commands, &mut self.state);

            // 同步 renderer
            self.renderer.typewriter_delay =
                Duration::from_secs_f32(self.state.typewriter_delay);

            // 场景切换处理
            if output.commands.iter().any(|c| matches!(c, Command::TransitionTo(_))) {
                self.scene_start = Instant::now();
                self.state.scene_entry_time = self.state.time_of_day;
                let _ = self.bgm_manager.on_scene_changed(&self.state.current_scene);
            }

            // 子系统命令处理（复用 handle_choice 中的逻辑）
            for cmd in &output.commands {
                match cmd {
                    Command::EnterNaviMini => {
                        let result = navi_mini::run_navi_mini(&self.assets, &mut self.state)?;
                        navi_mini::exit_navi()?;
                        match result {
                            navi_mini::NaviResult::ReturnTo(scene) => {
                                self.state.current_scene = scene;
                                self.scene_start = Instant::now();
                                self.state.scene_entry_time = self.state.time_of_day;
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

        // 显示结果
        if !output.lines.is_empty() {
            self.command_result = Some(output.lines.clone());
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
        use crate::world::location::Poi;

        // 辅助函数：从字符串表获取文本，fallback 到硬编码
        let s = |id: &str, _fallback: &str| -> String {
            self.assets.get_string(id).to_string()
        };

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

        // 插入所有地点
        for loc in [front_yard, lower_hallway, living_dining_kitchen, bathroom, upper_hallway, lain_room, mikas_room, study] {
            self.assets.locations.insert(loc.id.clone(), loc);
        }
    }
}

// =============================================================================
// 自动事件检查
// =============================================================================
/// 检查自动事件
///
/// 基于进入场景后的经过秒数，自动触发场景跳转。
/// 防止循环：如果 auto_event 设置了 flag，且该 flag 已为 "1"，则跳过。
fn check_auto_events(
    gs: &mut GameState,
    scene: &crate::narrative::scene::Scene,
) -> Option<String> {
    use crate::engine::time;
    use crate::narrative::conditions::check_all;

    let decoded_current = time::decode(gs.time_of_day);
    let decoded_entry = time::decode(gs.scene_entry_time);
    let elapsed = if decoded_current.data >= decoded_entry.data {
        decoded_current.data - decoded_entry.data
    } else {
        0
    };
    let elapsed_seconds = elapsed / 16;

    for event in &scene.auto_events {
        // 防止循环
        if let Some(ref flag) = event.flag_set {
            if gs.get_flag(flag) == Some("1") {
                continue;
            }
        }

        if (elapsed_seconds as u64) < event.wait_time {
            continue;
        }

        if check_all(&event.conditions, gs) {
            if let Some(ref flag) = event.flag_set {
                gs.set_flag(flag, "1");
            }
            return Some(event.target_scene.clone());
        }
    }

    None
}

mod tests {
    use super::*;
    use crate::narrative::command::Command;
    use std::path::PathBuf;

    
    

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
