//! 列车购票机 —— 替代 C 版本的 train_system.c
//!
//! 在 raw mode 下运行，使用 crossterm 事件读取。

use std::io::{self, Write};

use crossterm::event::KeyCode;
use crossterm::{
    cursor::MoveTo,
    terminal::{Clear, ClearType},
    ExecutableCommand,
};

/// 车站
#[derive(Debug, Clone)]
pub struct Station {
    pub id: String,
    pub name: String,
}

/// 运行购票机界面
pub fn run_ticket_machine() -> io::Result<()> {
    let stations = vec![
        Station { id: "shibuya".into(), name: "涩谷".into() },
        Station { id: "shinjuku".into(), name: "新宿".into() },
        Station { id: "ikebukuro".into(), name: "池袋".into() },
        Station { id: "tokyo".into(), name: "东京".into() },
        Station { id: "ueno".into(), name: "上野".into() },
        Station { id: "akihabara".into(), name: "秋叶原".into() },
        Station { id: "ginza".into(), name: "银座".into() },
    ];

    let mut stdout = io::stdout();

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    println!("         东京电车购票机");
    println!("========================================");
    println!("请选择您的目的地：");
    for (i, station) in stations.iter().enumerate() {
        println!("  {}. {} ({})", i + 1, station.name, station.id);
    }
    println!("----------------------------------------");
    println!("  e. 退出");
    println!("========================================");
    stdout.flush()?;

    let mut running = true;
    while running {
        print!("> ");
        stdout.flush()?;

        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char('e') | KeyCode::Char('E') => {
                    running = false;
                }
                KeyCode::Char(c) if c.is_ascii_digit() => {
                    let idx = c.to_digit(10).unwrap() as usize;
                    if idx > 0 && idx <= stations.len() {
                        println!(
                            "您选择了: {}。当前功能开发中，请选择 'e' 退出。",
                            stations[idx - 1].name
                        );
                    } else {
                        println!("无效选择。");
                    }
                }
                _ => {}
            }
        }
    }

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;
    stdout.flush()?;
    Ok(())
}
