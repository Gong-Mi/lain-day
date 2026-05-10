//! NAVI Mini 子系统 —— 最小化桌面环境
//!
//! 当玩家选择 `use_desktop_navi` 时进入此模式。
//! 显示 Psyche OS 风格的终端界面，提供基础命令。

use std::io::{self, Write};

use crossterm::{
    cursor::{MoveTo, Show},
    event::KeyCode,
    style::{Color, Print, ResetColor, SetForegroundColor},
    terminal::{Clear, ClearType},
    ExecutableCommand, QueueableCommand,
};

use crate::assets::Assets;
use crate::engine::state::GameState;
// use crate::render::emergency_cleanup;

/// NAVI Mini 运行结果
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum NaviResult {
    /// 继续游戏，返回指定场景
    ReturnTo(String),
    /// 打开浏览器（外部系统）
    OpenBrowser,
    /// 查看本地文件
    ViewFiles,
    /// 无操作
    NoOp,
}

/// 运行 NAVI Mini 子系统
///
/// 这是一个阻塞调用，接管终端直到玩家选择退出。
pub fn run_navi_mini(assets: &Assets, _state: &mut GameState) -> io::Result<NaviResult> {
    let mut stdout = io::stdout();

    // 清屏并显示 NAVI 桌面
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;
    stdout.execute(Show)?;

    // 标题
    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche OS [v0.7] 启动。\n"))?;
    stdout.queue(ResetColor)?;

    let welcome = assets.get_string("TEXT_PC_DESKTOP_WELCOME");
    if welcome != "TEXT_PC_DESKTOP_WELCOME" {
        println!("{}", welcome);
    } else {
        println!("欢迎，Lain。");
    }
    println!();

    // 选项
    let choice_browser = assets.get_string("TEXT_PC_DESKTOP_CHOICE_BROWSER");
    let choice_files = assets.get_string("TEXT_PC_DESKTOP_CHOICE_FILES");
    let choice_logoff = assets.get_string("TEXT_PC_DESKTOP_CHOICE_LOG_OFF");

    println!("1. {}", if choice_browser != "TEXT_PC_DESKTOP_CHOICE_BROWSER" { choice_browser } else { "打开浏览器" });
    println!("2. {}", if choice_files != "TEXT_PC_DESKTOP_CHOICE_FILES" { choice_files } else { "查看本地文件" });
    println!("3. {}", if choice_logoff != "TEXT_PC_DESKTOP_CHOICE_LOG_OFF" { choice_logoff } else { "断开连接" });
    println!();
    print!("> ");
    stdout.flush()?;

    // 简单输入循环
    loop {
        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char('1') => {
                    return Ok(NaviResult::OpenBrowser);
                }
                KeyCode::Char('2') => {
                    return Ok(NaviResult::ViewFiles);
                }
                KeyCode::Char('3') | KeyCode::Char('q') => {
                    return Ok(NaviResult::NoOp);
                }
                _ => {}
            }
        }
    }
}

/// 安全退出 NAVI（恢复主循环前的终端状态）
pub fn exit_navi() -> io::Result<()> {
    let mut stdout = io::stdout();
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_navi_result_eq() {
        assert_eq!(
            NaviResult::ReturnTo("SCENE_01".into()),
            NaviResult::ReturnTo("SCENE_01".into())
        );
        assert_ne!(
            NaviResult::ReturnTo("SCENE_01".into()),
            NaviResult::NoOp
        );
    }
}
