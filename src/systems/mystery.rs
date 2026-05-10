//! 谜题系统 —— 替代 C 版本的 mystery_system.c
//!
//! 案件 001: "The Barman's Gun"
//! 玩家通过组合关键词推理谜题，最终通过测验。

use std::io::{self, Write};
use std::thread;
use std::time::Duration;

use crossterm::event::KeyCode;
use crossterm::{
    cursor::MoveTo,
    style::{Color, Print, ResetColor, SetForegroundColor},
    terminal::{Clear, ClearType},
    ExecutableCommand, QueueableCommand,
};

// =============================================================================
// 数据
// =============================================================================

struct Keyword {
    id: &'static str,
    display_name: &'static str,
    is_visible: bool,
}

struct Combination {
    keyword1: &'static str,
    keyword2: &'static str,
    question: &'static str,
    answer: &'static str,
}

struct QuizQuestion {
    question: &'static str,
    options: [&'static str; 3],
    correct: usize,
}

const KEYWORDS: &[Keyword] = &[
    Keyword { id: "man", display_name: "Man", is_visible: true },
    Keyword { id: "water", display_name: "Water", is_visible: true },
    Keyword { id: "bartender", display_name: "Bartender", is_visible: true },
    Keyword { id: "gun", display_name: "Gun", is_visible: true },
    Keyword { id: "thank_you", display_name: "Thank You", is_visible: true },
    Keyword { id: "hiccups", display_name: "Hiccups", is_visible: false },
];

const COMBINATIONS: &[Combination] = &[
    Combination { keyword1: "water", keyword2: "", question: "Did he want water to drink?", answer: "NO" },
    Combination { keyword1: "man", keyword2: "water", question: "Was the man thirsty?", answer: "NO" },
    Combination { keyword1: "man", keyword2: "gun", question: "Was the man afraid of the gun?", answer: "YES (Initially)" },
    Combination { keyword1: "bartender", keyword2: "gun", question: "Did the bartender want to kill the man?", answer: "NO" },
    Combination { keyword1: "bartender", keyword2: "water", question: "Did the bartender refuse to give water?", answer: "NO (He helped in another way)" },
    Combination { keyword1: "gun", keyword2: "water", question: "Was it a water gun?", answer: "NO" },
    Combination { keyword1: "man", keyword2: "thank_you", question: "Was the man grateful for the gun?", answer: "YES" },
    Combination { keyword1: "gun", keyword2: "thank_you", question: "Did the gun help the man?", answer: "YES" },
];

const QUIZ: &[QuizQuestion] = &[
    QuizQuestion { question: "Why did the man ask for water?", options: ["He was thirsty", "He had hiccups", "He wanted to clean a stain"], correct: 1 },
    QuizQuestion { question: "Why did the bartender pull a gun?", options: ["To rob the man", "To scare the man", "To clean the gun"], correct: 1 },
    QuizQuestion { question: "Why did the man say thank you?", options: ["He likes guns", "He was suicidal", "His hiccups were cured"], correct: 2 },
];

// =============================================================================
// 主入口
// =============================================================================

/// 运行谜题应用
/// 返回 true 表示解谜成功
pub fn run_mystery_app() -> io::Result<bool> {
    let mut slot1 = String::new();
    let mut slot2 = String::new();
    let mut last_question = String::new();
    let mut last_response = String::new();

    let mut stdout = io::stdout();

    loop {
        render_screen(&mut stdout, &slot1, &slot2, &last_question, &last_response)?;
        stdout.flush()?;

        print!("MYSTERY> ");
        stdout.flush()?;

        let input = read_line_raw()?;
        let trimmed = input.trim();

        match trimmed {
            "exit" | "quit" => return Ok(false),
            "clear" => {
                slot1.clear();
                slot2.clear();
                last_question.clear();
                last_response.clear();
            }
            "solve" => {
                if run_quiz(&mut stdout)? {
                    stdout.execute(Clear(ClearType::All))?;
                    stdout.execute(MoveTo(0, 0))?;
                    println!("\x1b[32m\n=== CASE SOLVED ===\x1b[0m");
                    println!("Congratulations, Lain. You have uncovered the truth.");
                    println!("(Press any key to return)");
                    stdout.flush()?;
                    let _ = crossterm::event::read()?;
                    return Ok(true);
                }
            }
            _ if trimmed.starts_with("touch ") => {
                let word = trimmed[6..].trim();
                let kw = KEYWORDS.iter().find(|k| {
                    k.id.eq_ignore_ascii_case(word) || k.display_name.eq_ignore_ascii_case(word)
                });

                if let Some(k) = kw {
                    if slot1.is_empty() {
                        slot1 = k.id.to_string();
                        last_question.clear();
                        last_response.clear();
                    } else if slot2.is_empty() {
                        if slot1 == k.id {
                            last_question = "Duplicate keyword".to_string();
                            last_response = "(You already selected that)".to_string();
                        } else {
                            slot2 = k.id.to_string();
                            check_combination(&slot1, &slot2, &mut last_question, &mut last_response);
                        }
                    } else {
                        // FIFO shift
                        slot1 = slot2.clone();
                        slot2 = k.id.to_string();
                        check_combination(&slot1, &slot2, &mut last_question, &mut last_response);
                    }
                } else {
                    last_question = format!("Keyword '{}' not found", word);
                    last_response = "Try one of the visible keywords.".to_string();
                }
            }
            _ => {
                last_question = "Command not recognized".to_string();
                last_response = "Try 'touch <word>', 'clear', 'solve', or 'exit'".to_string();
            }
        }
    }
}

// =============================================================================
// 渲染
// =============================================================================

fn render_screen(
    stdout: &mut io::Stdout,
    slot1: &str,
    slot2: &str,
    last_question: &str,
    last_response: &str,
) -> io::Result<()> {
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    stdout.queue(SetForegroundColor(Color::Cyan))?;
    stdout.queue(Print("=== MYSTERY APP: CASE 001 ===\n"))?;
    stdout.queue(ResetColor)?;

    stdout.queue(SetForegroundColor(Color::Yellow))?;
    stdout.queue(Print("The Barman's Gun\n\n"))?;
    stdout.queue(ResetColor)?;

    println!("A man walks into a bar and asks for a glass of water.");
    println!("The bartender pulls out a gun and points it at the man.");
    println!("The man says 'thank you' and leaves.");
    println!("Why?");
    println!("----------------------------------------");

    print!("KEYWORDS: ");
    for kw in KEYWORDS {
        if kw.is_visible {
            print!("[{}] ", kw.display_name);
        }
    }
    println!("\n----------------------------------------");

    print!("THINKING: ");
    if !slot1.is_empty() {
        print!("[{}] ", slot1);
    } else {
        print!("[   ] ");
    }
    print!("+ ");
    if !slot2.is_empty() {
        print!("[{}] ", slot2);
    } else {
        print!("[   ] ");
    }
    println!("\n----------------------------------------");

    if !last_question.is_empty() {
        println!("Q: {}", last_question);
        stdout.queue(SetForegroundColor(Color::Green))?;
        stdout.queue(Print(format!("A: {}\n", last_response)))?;
        stdout.queue(ResetColor)?;
    }

    println!("\nCommands: 'touch <word>', 'clear', 'solve', 'exit'");
    Ok(())
}

// =============================================================================
// 逻辑
// =============================================================================

fn check_combination(slot1: &str, slot2: &str, out_q: &mut String, out_a: &mut String) {
    // 规范化顺序
    let (k1, k2) = if slot1 > slot2 { (slot2, slot1) } else { (slot1, slot2) };

    for comb in COMBINATIONS {
        let matched = if comb.keyword2.is_empty() {
            comb.keyword1 == k1 && k2.is_empty()
        } else {
            (comb.keyword1 == k1 && comb.keyword2 == k2)
                || (comb.keyword1 == k2 && comb.keyword2 == k1)
        };

        if matched {
            *out_q = comb.question.to_string();
            *out_a = comb.answer.to_string();
            return;
        }
    }

    if !k2.is_empty() {
        *out_q = format!("Is there a connection between {} and {}?", k1, k2);
    } else {
        *out_q = format!("Is {} relevant?", k1);
    }
    *out_a = "The connection is unclear... (Try a different pair)".to_string();
}

fn run_quiz(stdout: &mut io::Stdout) -> io::Result<bool> {
    stdout.execute(Clear(ClearType::All))?;
    stdout.execute(MoveTo(0, 0))?;

    println!("\x1b[36m=== FINAL DEDUCTION ===\n\x1b[0m");

    for q in QUIZ {
        println!("Q: {}", q.question);
        for (i, opt) in q.options.iter().enumerate() {
            println!("  {}) {}", i + 1, opt);
        }

        let mut choice = 0usize;
        while choice < 1 || choice > 3 {
            print!("Choice (1-3): ");
            stdout.flush()?;

            if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
                match key.code {
                    KeyCode::Char('1') => choice = 1,
                    KeyCode::Char('2') => choice = 2,
                    KeyCode::Char('3') => choice = 3,
                    _ => {}
                }
            }
        }

        if choice - 1 != q.correct {
            println!("\x1b[31m\nINCORRECT! The truth is still lost in the Wired...\x1b[0m");
            thread::sleep(Duration::from_secs(2));
            return Ok(false);
        }
        println!("\x1b[32mCORRECT!\n\x1b[0m");
    }

    Ok(true)
}

/// 在 raw mode 下读取一行
fn read_line_raw() -> io::Result<String> {
    let mut input = String::new();
    loop {
        if let crossterm::event::Event::Key(key) = crossterm::event::read()? {
            match key.code {
                KeyCode::Char(c) => {
                    input.push(c);
                    print!("{}", c);
                    io::stdout().flush()?;
                }
                KeyCode::Backspace => {
                    if !input.is_empty() {
                        input.pop();
                        print!("\x08 \x08");
                        io::stdout().flush()?;
                    }
                }
                KeyCode::Enter => {
                    println!();
                    return Ok(input);
                }
                _ => {}
            }
        }
    }
}
