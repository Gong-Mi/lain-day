//! 启动序列 —— 替代 C 版本的 boot_system.c
//!
//! 在进入主游戏循环前执行：
//! 1. 解析命令行参数（--test, -d）
//! 2. 显示 "CLOSE THE WORLD" 动画
//! 3. 系统状态消息
//! 4. 语言选择
//! 5. 会话 ID 输入与验证

use std::io::{self, Write};
use std::thread;
use std::time::Duration;

/// 启动配置
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct BootConfig {
    pub session_name: String,
    pub fast_boot: bool,
    pub test_mode: bool,
    pub lang: Lang,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Lang {
    English,
    Chinese,
}

impl Default for BootConfig {
    fn default() -> Self {
        Self {
            session_name: "default".to_string(),
            fast_boot: false,
            test_mode: false,
            lang: Lang::Chinese,
        }
    }
}

/// 解析命令行参数
pub fn parse_boot_args(args: &[String]) -> BootConfig {
    let mut config = BootConfig::default();
    for arg in args.iter().skip(1) {
        match arg.as_str() {
            "--test" => config.test_mode = true,
            "-d" => config.fast_boot = true,
            _ => {}
        }
    }
    config
}

/// 执行启动序列
///
/// 在标准终端模式下运行（尚未进入 crossterm alternate screen）。
pub fn perform_boot_sequence(config: &mut BootConfig) -> io::Result<()> {
    if config.fast_boot {
        config.session_name = "debug_user".to_string();
        return Ok(());
    }

    if config.test_mode {
        config.session_name = "test_user".to_string();
        return Ok(());
    }

    let mut stdout = io::stdout();

    // 清屏
    print!("\x1b[2J\x1b[H");
    stdout.flush()?;

    // 1. "CLOSE THE WORLD, OPEN THE NExT"
    println!("\x1b[36m   CLOSE THE WORLD,\x1b[0m");
    stdout.flush()?;
    thread::sleep(Duration::from_millis(1000));
    println!("\x1b[35m           OPEN THE NExT.\x1b[0m");
    stdout.flush()?;
    thread::sleep(Duration::from_millis(1200));
    println!();

    // 2. 系统初始化消息
    sys_line("[WARN]    NO TRUSTED ENVIRONMENT DETECTED.", "33");
    thread::sleep(Duration::from_millis(500));
    sys_line("[OK]      SYSTEM INTEGRITY CHECK COMPLETE.", "32");
    thread::sleep(Duration::from_millis(400));
    sys_line("[SYSTEM]  INITIALIZING PROTOCOLS...", "36");
    thread::sleep(Duration::from_millis(300));
    sys_line("[NET]     CONNECTING TO DEFAULT GATEWAY...", "36");
    thread::sleep(Duration::from_millis(800));
    sys_line("[ERROR]   CONNECTION REFUSED (TIMEOUT).", "31");
    thread::sleep(Duration::from_millis(600));
    sys_line("[WARN]    REROUTING VIA BACKUP GATEWAY (IPv4)...", "33");
    thread::sleep(Duration::from_millis(700));
    sys_line("[NET]     HANDSHAKE INITIATED...", "36");
    thread::sleep(Duration::from_millis(300));
    sys_line("[OK]      CONNECTION ESTABLISHED.", "32");
    thread::sleep(Duration::from_millis(500));
    sys_line("[WARN]    TARGET_IDENTITY IS NULL (null).", "31");
    thread::sleep(Duration::from_millis(400));
    sys_line("[WARN]    ATTEMPTING SESSION RECOVERY...", "33");
    thread::sleep(Duration::from_millis(900));
    sys_line("[ERROR]   NO ARCHIVE HISTORY FOUND.", "31");
    thread::sleep(Duration::from_millis(500));
    sys_line("[WARN]    INITIALIZING WORKSPACE...", "90");
    thread::sleep(Duration::from_millis(300));
    println!();
    sys_line("[SYSTEM]  DETECTING LOCALE...", "36");
    thread::sleep(Duration::from_millis(400));
    sys_line("[WARN]    UNSUPPORTED ENCODING. FALLBACK TO UTF-8.", "33");
    thread::sleep(Duration::from_millis(300));

    // 3. 语言选择
    println!();
    println!("\x1b[90m   [\x1b[35m QUERY \x1b[90m]  SELECT INTERFACE LANGUAGE:\x1b[0m");
    println!("\x1b[90m              1. English (Standard)\x1b[0m");
    println!("\x1b[90m              2. 简体中文 (Local)\x1b[0m");
    println!();

    let mut chosen = false;
    while !chosen {
        print!("\x1b[90m   [\x1b[36m INPUT \x1b[90m]  language_id: \x1b[0m");
        stdout.flush()?;

        let mut input = String::new();
        io::stdin().read_line(&mut input)?;
        match input.trim() {
            "1" => {
                config.lang = Lang::English;
                chosen = true;
            }
            "2" => {
                config.lang = Lang::Chinese;
                chosen = true;
            }
            _ => {}
        }
    }

    let lang_str = match config.lang {
        Lang::English => "ENGLISH",
        Lang::Chinese => "CHINESE",
    };
    sys_line(&format!("[OK]      LOCALIZATION SET TO {}.", lang_str), "32");
    thread::sleep(Duration::from_millis(500));
    println!();

    // 4. 会话 ID 输入
    config.session_name = read_session_id(config.lang)?;

    // 5. 创建会话目录
    let session_dir = format!("sessions/{}", config.session_name);
    std::fs::create_dir_all(&session_dir)?;

    let session_file = format!("{}/character.json", session_dir);
    if std::path::Path::new(&session_file).exists() {
        let msg = match config.lang {
            Lang::English => format!("   Resuming session '{}'...", config.session_name),
            Lang::Chinese => format!("   正在恢复会话 '{}'...", config.session_name),
        };
        println!("\x1b[90m{}\x1b[0m", msg);
        thread::sleep(Duration::from_millis(300));
    } else {
        let default_char = r#"{"name":"Lain","version":"1.0"}"#;
        std::fs::write(&session_file, default_char)?;
    }

    println!();
    thread::sleep(Duration::from_millis(300));
    Ok(())
}

/// 打印带颜色标签的系统行
fn sys_line(text: &str, _color: &str) {
    let (label, rest) = if let Some(pos) = text.find(']') {
        let tag = &text[..=pos];
        let body = &text[pos + 1..];
        let colored_tag = match tag {
            "[WARN]" => "\x1b[90m   [\x1b[33m WARN \x1b[90m]   ",
            "[OK]" => "\x1b[90m   [\x1b[32m OK \x1b[90m]     ",
            "[ERROR]" => "\x1b[90m   [\x1b[31m ERROR \x1b[90m]  ",
            "[SYSTEM]" => "\x1b[90m   [\x1b[36m SYSTEM \x1b[90m] ",
            "[NET]" => "\x1b[90m   [\x1b[36m NET \x1b[90m]    ",
            "[QUERY]" => "\x1b[90m   [\x1b[35m QUERY \x1b[90m]  ",
            "[INPUT]" => "\x1b[90m   [\x1b[36m INPUT \x1b[90m]  ",
            _ => "\x1b[90m              ",
        };
        (colored_tag, body)
    } else {
        ("\x1b[90m              ", text)
    };
    println!("{}{}\x1b[0m", label, rest);
}

fn read_session_id(lang: Lang) -> io::Result<String> {
    let prompt = match lang {
        Lang::English => "\x1b[90m   [\x1b[36m INPUT \x1b[90m]  Session ID: \x1b[0m",
        Lang::Chinese => "\x1b[90m   [\x1b[36m INPUT \x1b[90m]  请输入会话 ID: \x1b[0m",
    };

    loop {
        print!("{}", prompt);
        io::stdout().flush()?;

        let mut input = String::new();
        io::stdin().read_line(&mut input)?;
        let trimmed = input.trim();

        if is_valid_session_name(trimmed) {
            return Ok(trimmed.to_string());
        }

        let err = match lang {
            Lang::English => "\x1b[31m   Error: Invalid Session ID.\x1b[0m",
            Lang::Chinese => "\x1b[31m   错误：无效的会话名称。\x1b[0m",
        };
        println!("{}", err);
    }
}

fn is_valid_session_name(name: &str) -> bool {
    !name.is_empty() && name.chars().all(|c| c.is_ascii_alphanumeric() || c == '_' || c == '-')
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_args() {
        let args = vec!["lain-day".into(), "--test".into()];
        let cfg = parse_boot_args(&args);
        assert!(cfg.test_mode);
        assert!(!cfg.fast_boot);
    }

    #[test]
    fn test_parse_fast_boot() {
        let args = vec!["lain-day".into(), "-d".into()];
        let cfg = parse_boot_args(&args);
        assert!(cfg.fast_boot);
    }

    #[test]
    fn test_valid_session_name() {
        assert!(is_valid_session_name("lain_001"));
        assert!(is_valid_session_name("test-user"));
        assert!(!is_valid_session_name(""));
        assert!(!is_valid_session_name("hello world"));
        assert!(!is_valid_session_name("a@b"));
    }
}
