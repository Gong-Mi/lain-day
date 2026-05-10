//! 浏览器子系统 —— Psyche Browser / Wired Navigator
//!
//! 模拟 1998 年的网络浏览体验，显示几个预置的"网站"。
//! 从 Assets 中读取文本（如果存在），否则使用默认内容。

use std::io::{self, Write};

use crossterm::event::KeyCode;
use crossterm::{
    cursor::MoveTo,
    style::{Color, Print, ResetColor, SetForegroundColor},
    terminal::{Clear, ClearType},
    ExecutableCommand, QueueableCommand,
};

use crate::assets::Assets;

/// 浏览器运行结果
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum BrowserResult {
    /// 无操作（正常退出）
    NoOp,
}

/// 预置网站
struct Site {
    id: &'static str,
    url: &'static str,
    title: &'static str,
    lines: &'static [&'static str],
}

const SITES: &[Site] = &[
    Site {
        id: "wired_gate",
        url: "http://wired.tld",
        title: "WIRED GATE",
        lines: &[
            "",
            "    CLOSE THE WORLD,",
            "           OPEN THE NExT.",
            "",
            "    欢迎，Network Navigator。",
            "    这里是 Wired 的入口。",
            "    没有人在线。所有人都在线。",
            "",
            "    [Connection: UNSTABLE]",
        ],
    },
    Site {
        id: "navi_home",
        url: "file:///home/lain/index.html",
        title: "Navi Home",
        lines: &[
            "",
            "    欢迎来到 Lain 的 Navi 主页。",
            "",
            "    这里什么都没有。",
            "    这里什么都有。",
            "",
            "    最后更新: 1998-07-06",
            "",
        ],
    },
    Site {
        id: "knights_frag",
        url: "http://knights.bbs.frag",
        title: "The Knights of the Eastern Calculus",
        lines: &[
            "",
            "    骑士团论坛 [碎片存档]",
            "",
            "    > 人类只是协议。",
            "    > 她在所有地方，她不在任何地方。",
            "    > 不要寻找神。神在寻找你。",
            "",
            "    [ERROR: 403 Forbidden]",
            "    该论坛已被管理员封锁。",
            "",
        ],
    },
    Site {
        id: "tachibana_lab",
        url: "http://tachibana-lab.co.jp",
        title: "Tachibana General Laboratories",
        lines: &[
            "",
            "    橘综合研究所",
            "",
            "    致力于下一代网络协议研发。",
            "    招聘：高级协议工程师、信息安全顾问。",
            "",
            "    联系电话: 03-XXXX-XXXX",
            "",
        ],
    },
    Site {
        id: "cyberia_club",
        url: "http://cyberia.club",
        title: "Cyberia Club",
        lines: &[
            "",
            "    Cyberia Club",
            "",
            "    今晚的 DJ: ---",
            "    入场费: 2000 JPY",
            "",
            "    [UNDER CONSTRUCTION]",
            "",
        ],
    },
];

/// 运行浏览器子系统
///
/// 这是一个阻塞调用，接管终端直到玩家选择退出。
pub fn run_browser(assets: &Assets) -> io::Result<BrowserResult> {
    let mut stdout = io::stdout();

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    // 浏览器标题栏
    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche Browser [v0.1]"))?;
    stdout.queue(ResetColor)?;
    stdout.queue(Print(" | "))?;
    stdout.queue(SetForegroundColor(Color::DarkGrey))?;
    stdout.queue(Print("Disconnected"))?;
    stdout.queue(ResetColor)?;
    println!();
    println!("{}", "─".repeat(60));

    // 书签列表
    println!("书签:");
    for (i, site) in SITES.iter().enumerate() {
        println!("  {}. {:25} {}", i + 1, site.title, site.url);
    }
    println!("  q. 关闭浏览器");
    println!();
    print!("> ");
    stdout.flush()?;

    loop {
        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char(c) if c.is_ascii_digit() => {
                    let idx = c.to_digit(10).unwrap() as usize;
                    if idx > 0 && idx <= SITES.len() {
                        let site = &SITES[idx - 1];
                        show_site(assets, site)?;
                        // 显示完返回书签页
                        return_to_bookmarks(assets)?;
                    }
                }
                KeyCode::Char('q') | KeyCode::Char('Q') => {
                    return Ok(BrowserResult::NoOp);
                }
                _ => {}
            }
        }
    }
}

fn show_site(assets: &Assets, site: &Site) -> io::Result<()> {
    let mut stdout = io::stdout();
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    // 地址栏
    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche Browser [v0.1]"))?;
    stdout.queue(ResetColor)?;
    stdout.queue(Print(" | "))?;
    stdout.queue(SetForegroundColor(Color::Yellow))?;
    stdout.queue(Print(site.url))?;
    stdout.queue(ResetColor)?;
    println!();
    println!("{}", "─".repeat(60));

    // 标题
    stdout.queue(SetForegroundColor(Color::Cyan))?;
    stdout.queue(Print(site.title))?;
    stdout.queue(ResetColor)?;
    println!();
    println!();

    // 内容（优先从 assets 读取，否则用默认）
    let text_key = format!("TEXT_BROWSER_SITE_{}", site.id.to_uppercase());
    let custom = assets.get_string(&text_key);
    if custom != text_key {
        // 使用自定义文本（支持多行，用 \n 分隔）
        for line in custom.lines() {
            println!("{}", line);
        }
    } else {
        for line in site.lines {
            println!("{}", line);
        }
    }

    println!();
    stdout.queue(SetForegroundColor(Color::DarkGrey))?;
    stdout.queue(Print("[b] 返回书签  [q] 关闭浏览器"))?;
    stdout.queue(ResetColor)?;
    println!();
    stdout.flush()?;

    loop {
        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char('b') | KeyCode::Char('B') => {
                    return Ok(());
                }
                KeyCode::Char('q') | KeyCode::Char('Q') => {
                    // 让调用方处理退出
                    return Ok(());
                }
                _ => {}
            }
        }
    }
}

fn return_to_bookmarks(_assets: &Assets) -> io::Result<()> {
    let mut stdout = io::stdout();
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche Browser [v0.1]"))?;
    stdout.queue(ResetColor)?;
    stdout.queue(Print(" | "))?;
    stdout.queue(SetForegroundColor(Color::DarkGrey))?;
    stdout.queue(Print("Disconnected"))?;
    stdout.queue(ResetColor)?;
    println!();
    println!("{}", "─".repeat(60));

    println!("书签:");
    for (i, site) in SITES.iter().enumerate() {
        println!("  {}. {:25} {}", i + 1, site.title, site.url);
    }
    println!("  q. 关闭浏览器");
    println!();
    print!("> ");
    stdout.flush()?;
    Ok(())
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_browser_result_eq() {
        assert_eq!(BrowserResult::NoOp, BrowserResult::NoOp);
    }

    #[test]
    fn test_sites_not_empty() {
        assert!(!SITES.is_empty());
        assert_eq!(SITES[0].id, "wired_gate");
        assert_eq!(SITES[4].id, "cyberia_club");
    }

    #[test]
    fn test_site_lines_not_empty() {
        for site in SITES {
            assert!(
                !site.lines.is_empty(),
                "Site {} should have content lines",
                site.id
            );
        }
    }
}
