//! 文件管理器子系统 —— Psyche OS File Manager
//!
//! 模拟 1998 年的终端文件管理器，浏览 Lain 的 Navi 文件系统。
//! 支持目录切换、文件查看。

use std::io::{self, Write};

use crossterm::event::KeyCode;
use crossterm::{
    cursor::MoveTo,
    style::{Color, Print, ResetColor, SetForegroundColor},
    terminal::{Clear, ClearType},
    ExecutableCommand, QueueableCommand,
};

use crate::assets::Assets;

/// 文件管理器运行结果
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FileManagerResult {
    /// 无操作（正常退出）
    NoOp,
}

/// 虚拟文件或目录
struct VfsNode {
    name: &'static str,
    is_dir: bool,
    content: Option<&'static [&'static str]>,
}

/// 虚拟文件系统结构
const ROOT_NODES: &[VfsNode] = &[
    VfsNode {
        name: "documents",
        is_dir: true,
        content: None,
    },
    VfsNode {
        name: "downloads",
        is_dir: true,
        content: None,
    },
    VfsNode {
        name: "readme.txt",
        is_dir: false,
        content: Some(&[
            "这是我的 Navi。",
            "",
            "这里存放着我所有的东西，",
            "也存放着所有不是我的东西。",
            "",
            "如果你看到了这个文件，",
            "说明你已经在这里了。",
        ]),
    },
    VfsNode {
        name: "system.log",
        is_dir: false,
        content: Some(&[
            "[1998-07-06 02:14] System boot complete.",
            "[1998-07-06 02:15] Network adapter initialized.",
            "[1998-07-06 02:16] WARNING: Unrecognized protocol detected.",
            "[1998-07-06 02:17] Connection to unknown host established.",
            "[1998-07-06 02:18] ERROR: Memory segment 0x7A3F corrupted.",
            "[1998-07-06 02:19] Recovered from panic. Continuing...",
        ]),
    },
];

const DOCUMENTS_NODES: &[VfsNode] = &[
    VfsNode {
        name: "school_notes.txt",
        is_dir: false,
        content: Some(&[
            "学校笔记 - 1998年7月",
            "",
            "计算机网络课：",
            "  TCP/IP 协议栈",
            "  OSI 七层模型",
            "",
            "在最后一页有人写了一句：",
            "  'Protocol is people.'",
            "",
            "不知道是谁写的。",
        ]),
    },
    VfsNode {
        name: "wired_theory.txt",
        is_dir: false,
        content: Some(&[
            "Wired 理论草稿",
            "",
            "1. Wired 不是网络。Wired 是意识。",
            "2. 肉体是限制。协议是桥梁。",
            "3. 如果所有记忆都能上传，",
            "   那么'死亡'只是断开连接。",
            "",
            "WARNING: 该文件已被标记为 [SUSPICIOUS]",
        ]),
    },
];

const DOWNLOADS_NODES: &[VfsNode] = &[
    VfsNode {
        name: "unknown_protocol.dat",
        is_dir: false,
        content: Some(&[
            "<二进制数据 - 无法解析>",
            "",
            "文件签名: 0x4E 0x45 0x58 0x54",
            "大小: 65535 bytes",
            "来源: unknown.tld",
            "",
            "尝试用文本编辑器打开时，",
            "屏幕上出现了自己的脸。",
        ]),
    },
    VfsNode {
        name: "knights_manifesto.txt",
        is_dir: false,
        content: Some(&[
            "The Knights of the Eastern Calculus",
            "",
            "We do not seek God.",
            "We seek the protocol that God uses.",
            "",
            "Distributed. Anonymous. Everywhere.",
            "",
            "---",
            "",
            "如果你想加入我们，",
            "证明你能看见不可见之物。",
        ]),
    },
];

/// 当前目录上下文
struct DirCtx {
    name: &'static str,
    nodes: &'static [VfsNode],
}

/// 运行文件管理器子系统
///
/// 这是一个阻塞调用，接管终端直到玩家选择退出。
pub fn run_file_manager(_assets: &Assets) -> io::Result<FileManagerResult> {
    let mut stdout = io::stdout();

    let mut dir_stack: Vec<DirCtx> = vec![DirCtx {
        name: "/home/lain",
        nodes: ROOT_NODES,
    }];

    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    print_ui(&dir_stack)?;

    loop {
        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char('q') | KeyCode::Char('Q') => {
                    return Ok(FileManagerResult::NoOp);
                }
                KeyCode::Char('b') | KeyCode::Char('B') => {
                    if dir_stack.len() > 1 {
                        dir_stack.pop();
                        stdout.execute(Clear(ClearType::All))?;
                        stdout.execute(MoveTo(0, 0))?;
                        print_ui(&dir_stack)?;
                    }
                }
                KeyCode::Char(c) if c.is_ascii_digit() => {
                    let idx = c.to_digit(10).unwrap() as usize;
                    let current = dir_stack.last().unwrap();
                    if idx > 0 && idx <= current.nodes.len() {
                        let node = &current.nodes[idx - 1];
                        if node.is_dir {
                            // 进入子目录
                            let (name, nodes) = match node.name {
                                "documents" => ("documents", DOCUMENTS_NODES),
                                "downloads" => ("downloads", DOWNLOADS_NODES),
                                _ => continue,
                            };
                            dir_stack.push(DirCtx { name, nodes });
                            stdout.execute(Clear(ClearType::All))?;
                            stdout.execute(MoveTo(0, 0))?;
                            print_ui(&dir_stack)?;
                        } else if let Some(content) = node.content {
                            // 查看文件
                            view_file(&mut stdout, node.name, content)?;
                            // 等待按键返回
                            loop {
                                if let crossterm::event::Event::Key(key) =
                                    crossterm::event::read()?
                                {
                                    match key.code {
                                        KeyCode::Char('b')
                                        | KeyCode::Char('B')
                                        | KeyCode::Char('q')
                                        | KeyCode::Char('Q') => {
                                            break;
                                        }
                                        _ => {}
                                    }
                                }
                            }
                            stdout.execute(Clear(ClearType::All))?;
                            stdout.execute(MoveTo(0, 0))?;
                            print_ui(&dir_stack)?;
                        }
                    }
                }
                _ => {}
            }
        }
    }
}

fn print_ui(dir_stack: &[DirCtx]) -> io::Result<()> {
    let mut stdout = io::stdout();
    let current = dir_stack.last().unwrap();

    // 标题栏
    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche OS File Manager"))?;
    stdout.queue(ResetColor)?;
    stdout.queue(Print(" | "))?;
    stdout.queue(SetForegroundColor(Color::Yellow))?;
    let path = dir_stack.iter().map(|d| d.name).collect::<Vec<_>>().join("/");
    stdout.queue(Print(format!("{}", if path.starts_with('/') { path } else { format!("/home/lain/{}", path) })))?;
    stdout.queue(ResetColor)?;
    println!();
    println!("{}", "─".repeat(60));

    // 文件列表
    for (i, node) in current.nodes.iter().enumerate() {
        let icon = if node.is_dir { "[DIR] " } else { "[FILE]" };
        println!("  {}. {}  {}", i + 1, icon, node.name);
    }

    println!();
    if current.nodes.is_empty() {
        println!("  (空目录)");
    }

    println!();
    stdout.queue(SetForegroundColor(Color::DarkGrey))?;
    if dir_stack.len() > 1 {
        stdout.queue(Print("[b] 上级目录  "))?;
    }
    stdout.queue(Print("[数字] 打开  [q] 退出"))?;
    stdout.queue(ResetColor)?;
    println!();
    print!("> ");
    stdout.flush()?;
    Ok(())
}

fn view_file(
    stdout: &mut io::Stdout,
    filename: &str,
    content: &[&str],
) -> io::Result<()> {
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    stdout.queue(SetForegroundColor(Color::Green))?;
    stdout.queue(Print("Psyche OS File Manager"))?;
    stdout.queue(ResetColor)?;
    stdout.queue(Print(" | VIEW"))?;
    println!();
    println!("{}", "─".repeat(60));

    stdout.queue(SetForegroundColor(Color::Cyan))?;
    stdout.queue(Print(filename))?;
    stdout.queue(ResetColor)?;
    println!();
    println!();

    for line in content {
        println!("{}", line);
    }

    println!();
    stdout.queue(SetForegroundColor(Color::DarkGrey))?;
    stdout.queue(Print("[b/q] 返回"))?;
    stdout.queue(ResetColor)?;
    println!();
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
    fn test_file_manager_result_eq() {
        assert_eq!(FileManagerResult::NoOp, FileManagerResult::NoOp);
    }

    #[test]
    fn test_root_nodes_not_empty() {
        assert!(!ROOT_NODES.is_empty());
        assert!(ROOT_NODES.iter().any(|n| n.name == "readme.txt"));
        assert!(ROOT_NODES.iter().any(|n| n.name == "documents" && n.is_dir));
    }

    #[test]
    fn test_documents_nodes_have_content() {
        for node in DOCUMENTS_NODES {
            assert!(
                node.content.is_some(),
                "File {} should have content",
                node.name
            );
            assert!(!node.content.unwrap().is_empty());
        }
    }

    #[test]
    fn test_downloads_nodes_have_content() {
        for node in DOWNLOADS_NODES {
            assert!(
                node.content.is_some(),
                "File {} should have content",
                node.name
            );
            assert!(!node.content.unwrap().is_empty());
        }
    }

    #[test]
    fn test_system_log_has_error_entries() {
        let log = ROOT_NODES
            .iter()
            .find(|n| n.name == "system.log")
            .and_then(|n| n.content)
            .unwrap();
        let joined = log.join("\n");
        assert!(joined.contains("WARNING"));
        assert!(joined.contains("ERROR"));
        assert!(joined.contains("Memory segment"));
    }
}
