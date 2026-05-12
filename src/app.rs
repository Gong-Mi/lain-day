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
    /// 邮件客户端
    mailbox: crate::systems::mail::Mailbox,
    /// 邮件目录
    maildir: PathBuf,
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

        let mut mailbox = crate::systems::mail::Mailbox::new();
        let maildir = data_dir.join("mail");
        let _ = mailbox.load_from_dir(&maildir);

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
            mailbox,
            maildir,
        })
    }

    pub fn run(&mut self) -> io::Result<()> {
        self.renderer.enter()?;
        crate::world::maps::init_map(&mut self.assets, &self.state);

        while self.running {
            // 处理 resize
            if let Some((w, h)) = self.renderer.poll_resize()? {
                self.renderer.term_size = (w, h);
            }

            let scene_opt = self.assets.get_scene(&self.state.current_scene).cloned();
            let elapsed = self.scene_start.elapsed().as_millis() as u64;

            // 如果场景存在且地点匹配，正常渲染；否则渲染地点默认视图
            let render_location_view = match &scene_opt {
                Some(scene) => scene.location_id != self.state.player.location,
                None => self.assets.get_location(&self.state.player.location).is_some(),
            };

            if render_location_view {
                self.renderer.render_location_view(&self.assets, &self.state)?;
            } else if let Some(ref scene) = scene_opt {
                // 检查自动事件
                if let Some(target) = check_auto_events(&mut self.state, scene) {
                    self.state.current_scene = target;
                    self.scene_start = Instant::now();
                    self.state.scene_entry_time = self.state.time_of_day;
                    let _ = self.bgm_manager.on_scene_changed(&self.state.current_scene);
                    continue;
                }

                // 渲染
                self.renderer
                    .render_scene(scene, &self.assets, &self.state, elapsed)?;
            } else {
                self.handle_missing_scene()?;
                continue;
            }

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
                        if let Some(ref scene) = scene_opt {
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
                            self.handle_input(key, scene)?;
                        }
                    }
                    crossterm::event::Event::Mouse(mouse) => {
                        if !self.command_mode {
                            if let Some(ref scene) = scene_opt {
                                self.handle_mouse(mouse, scene, elapsed)?;
                            }
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
                self.dispatch_subsystem(cmd)?;
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

            // 子系统命令处理
            for cmd in &output.commands {
                self.dispatch_subsystem(cmd)?;
            }
        }

        // 显示结果
        if !output.lines.is_empty() {
            self.command_result = Some(output.lines.clone());
        }

        Ok(())
    }

    /// 调度子系统命令
    ///
    /// 统一处理 EnterNaviMini / EnterTrain / EnterMystery / EnterMail 等子系统命令。
    /// 这是一个阻塞调用，子系统会接管终端直到退出。
    fn dispatch_subsystem(&mut self, cmd: &Command) -> io::Result<()> {
        match cmd {
            Command::EnterNavi | Command::EnterNaviMini => {
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
                let _ = self.mailbox.load_from_dir(&self.maildir);
                crate::systems::mail::run_mail_app(&mut self.mailbox, &self.maildir)?;
                self.renderer.clear()?;
            }
            _ => {}
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
        crate::world::maps::init_map(&mut app.assets, &app.state);

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
        crate::world::maps::init_map(&mut app.assets, &app.state);

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
        crate::world::maps::init_map(&mut app.assets, &app.state);

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
        crate::world::maps::init_map(&mut app.assets, &app.state);

        // ENTRY → 下楼
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02_DOWNSTAIRS");
        assert_eq!(app.state.player.location, "iwakura_living_dining_kitchen");

        // DOWNSTAIRS 是地点视图（无选项），通过命令 talk_to_dad 进入父亲场景
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let cmds = crate::narrative::executor::resolve_action("talk_to_dad", &app.state, &scene);
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
        crate::world::maps::init_map(&mut app.assets, &app.state);

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
        crate::world::maps::init_map(&mut app.assets, &app.state);

        // ENTRY → DOWNSTAIRS
        let scene = app.assets.get_scene("SCENE_00_ENTRY").unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(2, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02_DOWNSTAIRS");
        assert_eq!(app.state.player.location, "iwakura_living_dining_kitchen");

        // DOWNSTAIRS 是地点视图，通过 talk_to_mom 命令进入母亲场景
        let cmds = crate::narrative::executor::resolve_action("talk_to_mom", &app.state, &scene);
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02D_TALK_TO_MOM_NORMAL");

        // MOM_NORMAL 的 downstairs 选项返回 DOWNSTAIRS
        let scene = app.assets.get_scene(&app.state.current_scene).unwrap().clone();
        let (cmds, _) = resolve_numeric_choice(1, &app.state, &scene, 999999).unwrap();
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02_DOWNSTAIRS");

        // 验证 mom_reply_silent 动作映射
        let cmds = crate::narrative::executor::resolve_action("mom_reply_silent", &app.state, &scene);
        apply_commands(&cmds, &mut app.state);
        assert_eq!(app.state.current_scene, "SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE");
        assert_eq!(app.state.get_flag("sister_mood"), Some("cold"));
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
