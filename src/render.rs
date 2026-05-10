//! 终端渲染 —— 替代 C 版本的 render_utils.c
//!
//! 使用 crossterm 实现跨平台终端控制。

use std::io::{self, stdout, Write};
use std::thread;
use std::time::Duration;

use crossterm::{
    cursor::{self, MoveTo},
    event::{self, Event, KeyEvent},
    style::{Color, Print, ResetColor, SetForegroundColor},
    terminal::{self, Clear, ClearType},
    ExecutableCommand, QueueableCommand,
};

use crate::assets::Assets;
use crate::engine::state::GameState;
use crate::engine::time;
use crate::narrative::scene::{DialogueLine, Scene, Speaker};

/// 选项的屏幕位置（起始行，1-indexed）
#[derive(Debug, Clone)]
pub struct ChoicePosition {
    pub index: usize,      // 在 scene.choices 中的索引
    pub visible_index: usize, // 玩家看到的编号（1,2,3...）
    pub row: u16,          // 屏幕行号
}

#[derive(Clone)]
pub struct Renderer {
    pub typewriter_delay: Duration,
    pub use_typewriter: bool,
    pub mouse_enabled: bool,
    /// 当前显示的是哪个场景（用于 takeover 增量注入）
    pub last_scene_id: String,
    /// 已经打印了多少行对话（takeover 模式用）
    pub printed_lines: usize,
    /// 终端尺寸
    pub term_size: (u16, u16),
    /// 当前帧选项的屏幕位置（用于鼠标点击）
    pub choice_positions: Vec<ChoicePosition>,
}

impl Default for Renderer {
    fn default() -> Self {
        Self {
            typewriter_delay: Duration::from_millis(40),
            use_typewriter: false,
            mouse_enabled: true,
            last_scene_id: String::new(),
            printed_lines: 0,
            term_size: terminal::size().unwrap_or((80, 24)),
            choice_positions: vec![],
        }
    }
}

impl Renderer {
    /// 初始化终端（进入备用屏幕缓冲区）
    pub fn enter(&self) -> io::Result<()> {
        stdout().execute(terminal::EnterAlternateScreen)?;
        stdout().execute(cursor::Hide)?;
        stdout().execute(terminal::DisableLineWrap)?;
        terminal::enable_raw_mode()?;
        if self.mouse_enabled {
            print!("\x1b[?1000h\x1b[?1006h");
            stdout().flush()?;
        }
        Ok(())
    }

    /// 恢复终端
    pub fn exit(&self) -> io::Result<()> {
        if self.mouse_enabled {
            print!("\x1b[?1006l\x1b[?1000l");
        }
        terminal::disable_raw_mode()?;
        stdout().execute(terminal::EnableLineWrap)?;
        stdout().execute(cursor::Show)?;
        stdout().execute(terminal::LeaveAlternateScreen)?;
        Ok(())
    }

    /// 清屏
    pub fn clear(&self) -> io::Result<()> {
        stdout().execute(Clear(ClearType::All))?;
        stdout().execute(MoveTo(0, 0))?;
        Ok(())
    }

    /// 检查并更新终端尺寸
    pub fn update_size(&mut self) -> io::Result<()> {
        self.term_size = terminal::size()?;
        Ok(())
    }

    /// 渲染完整场景
    pub fn render_scene(
        &mut self,
        scene: &Scene,
        assets: &Assets,
        gs: &GameState,
        elapsed_ms: u64,
    ) -> io::Result<()> {
        // 场景切换检测
        let scene_changed = self.last_scene_id != scene.scene_id;
        if scene_changed {
            self.last_scene_id = scene.scene_id.clone();
            self.printed_lines = 0;
        }

        if scene.is_takeover {
            self.render_takeover(scene, assets, gs, elapsed_ms, scene_changed)
        } else {
            self.render_normal(scene, assets, gs, elapsed_ms)
        }
    }

    /// 普通模式：标准滚动界面
    fn render_normal(
        &mut self,
        scene: &Scene,
        assets: &Assets,
        gs: &GameState,
        elapsed_ms: u64,
    ) -> io::Result<()> {
        self.clear()?;

        // 时间显示
        self.print_time(gs)?;
        println!("========================================");

        // 地点
        if let Some(loc) = assets.get_location(&scene.location_id) {
            println!("Location: {}", loc.name);
        } else if !scene.location_id.is_empty() {
            println!("Location: {}", scene.location_id);
        }
        println!("========================================");

        // 对话（普通模式显示全部）
        for line in &scene.dialogue {
            self.print_dialogue_line(line, assets)?;
        }

        // 选项
        self.render_choices(scene, assets, gs, elapsed_ms)?;

        stdout().flush()?;
        Ok(())
    }

    /// Takeover 模式：沉浸式逐行注入
    fn render_takeover(
        &mut self,
        scene: &Scene,
        assets: &Assets,
        gs: &GameState,
        elapsed_ms: u64,
        scene_changed: bool,
    ) -> io::Result<()> {
        if scene_changed {
            self.clear()?;
            self.print_time(gs)?;
            stdout().flush()?;
        }

        // 计算当前应该显示多少行
        let visible_lines = scene
            .dialogue
            .iter()
            .take_while(|line| elapsed_ms >= line.delay_ms())
            .count();

        // 增量注入：只打印新出现的行
        for i in self.printed_lines..visible_lines {
            let line = &scene.dialogue[i];
            self.print_dialogue_line(line, assets)?;
            stdout().flush()?;
            self.printed_lines = i + 1;
        }

        // 所有对话显示完毕后，显示选项
        if self.printed_lines >= scene.dialogue.len() && !scene.choices.is_empty() {
            // 清除之前可能残留的选项（简单方案：重绘选项区）
            self.render_choices(scene, assets, gs, elapsed_ms)?;
        }

        Ok(())
    }

    fn print_time(&self, gs: &GameState) -> io::Result<()> {
        let decoded = time::decode(gs.time_of_day);
        let (h, m) = time::to_hm(decoded.data);

        match decoded.status {
            time::TimeDecodeStatus::DoubleBitErrorDetected => {
                print!("\x1b[31m[##:##]\x1b[0m");
            }
            _ => {
                print!("\x1b[33m[{:02}:{:02}]\x1b[0m", h, m);
            }
        }
        println!(" \x1b[90m[U:{}]\x1b[0m", decoded.data);
        Ok(())
    }

    fn print_dialogue_line(&self, line: &DialogueLine, assets: &Assets) -> io::Result<()> {
        let text = assets.get_string(&line.text_id);
        let (speaker_name, color) = speaker_style(line.speaker);

        let mut stdout = stdout();

        if !speaker_name.is_empty() {
            stdout.queue(SetForegroundColor(color))?;
            stdout.queue(Print(format!("{}: ", speaker_name)))?;
            stdout.queue(ResetColor)?;
        }

        if self.use_typewriter && !text.is_empty() {
            for ch in text.chars() {
                stdout.queue(Print(ch.to_string()))?;
                stdout.flush()?;
                thread::sleep(self.typewriter_delay);
            }
            stdout.queue(Print("\n"))?;
        } else {
            if line.speaker == Speaker::None || line.speaker == Speaker::Navi {
                self.print_system_line(text)?;
            } else {
                println!("{}", text);
            }
        }

        Ok(())
    }

    fn print_system_line(&self, text: &str) -> io::Result<()> {
        if text.starts_with("[OK]") {
            println!(
                "\x1b[90m   [\x1b[32m OK \x1b[90m]     {}\x1b[0m",
                &text[4..]
            );
        } else if text.starts_with("[WARN]") {
            println!(
                "\x1b[90m   [\x1b[33m WARN \x1b[90m]   {}\x1b[0m",
                &text[6..]
            );
        } else if text.starts_with("[ERROR]") {
            println!(
                "\x1b[90m   [\x1b[31m ERROR \x1b[90m]  {}\x1b[0m",
                &text[7..]
            );
        } else if text.starts_with("[SYSTEM]") {
            println!(
                "\x1b[90m   [\x1b[36m SYSTEM \x1b[90m] {}\x1b[0m",
                &text[8..]
            );
        } else if text.starts_with("[NET]") {
            println!(
                "\x1b[90m   [\x1b[36m NET \x1b[90m]    {}\x1b[0m",
                &text[5..]
            );
        } else {
            println!("{}", text);
        }
        Ok(())
    }

    fn render_choices(
        &mut self,
        scene: &Scene,
        assets: &Assets,
        _gs: &GameState,
        elapsed_ms: u64,
    ) -> io::Result<()> {
        self.choice_positions.clear();

        if scene.choices.is_empty() {
            return Ok(());
        }

        println!("\n--- Choices ---");

        let mut visible_idx = 1;
        for (i, choice) in scene.choices.iter().enumerate() {
            if elapsed_ms < choice.delay_ms() {
                continue;
            }
            let text = assets.get_string(&choice.text_id);
            let row = cursor::position()?.1;
            println!("{}. {}", visible_idx, text);
            self.choice_positions.push(ChoicePosition {
                index: i,
                visible_index: visible_idx,
                row,
            });
            visible_idx += 1;
        }

        println!("---------------");
        Ok(())
    }

    /// 非阻塞检查按键（用于主循环）
    pub fn poll_key(&self) -> io::Result<Option<KeyEvent>> {
        if event::poll(Duration::from_millis(16))? {
            if let Event::Key(key) = event::read()? {
                return Ok(Some(key));
            }
        }
        Ok(None)
    }

    /// 阻塞读取按键
    pub fn read_key(&self) -> io::Result<KeyEvent> {
        loop {
            if let Event::Key(key) = event::read()? {
                return Ok(key);
            }
        }
    }

    /// 检查是否有 resize 事件
    pub fn poll_resize(&self) -> io::Result<Option<(u16, u16)>> {
        while event::poll(Duration::from_millis(0))? {
            if let Event::Resize(w, h) = event::read()? {
                return Ok(Some((w, h)));
            }
        }
        Ok(None)
    }
}

fn speaker_style(speaker: Speaker) -> (&'static str, Color) {
    match speaker {
        Speaker::None => ("", Color::Reset),
        Speaker::Lain => ("你", Color::Cyan),
        Speaker::Mom => ("妈妈", Color::Magenta),
        Speaker::Dad => ("爸爸", Color::Rgb { r: 180, g: 130, b: 70 }),
        Speaker::Alice => ("Alice", Color::Rgb { r: 255, g: 183, b: 197 }),
        Speaker::Chisa => ("Chisa", Color::Rgb { r: 152, g: 251, b: 152 }),
        Speaker::Mika => ("Mika", Color::Rgb { r: 221, g: 160, b: 221 }),
        Speaker::Ghost => ("幽灵", Color::Red),
        Speaker::Doctor => ("米良柊子", Color::Rgb { r: 100, g: 149, b: 237 }),
        Speaker::Navi => ("Navi", Color::Rgb { r: 0, g: 255, b: 127 }),
        Speaker::Shu => ("束", Color::Rgb { r: 255, g: 215, b: 0 }),
        Speaker::Parent => ("父母", Color::Yellow),
    }
}

/// 安全退出时的清理（用于 panic hook）
pub fn emergency_cleanup() {
    let _ = terminal::disable_raw_mode();
    let mut stdout = stdout();
    let _ = stdout.execute(terminal::LeaveAlternateScreen);
    let _ = stdout.execute(cursor::Show);
}
