//! 自由文本命令系统 —— 替代 C 版本的 execute_command()
//!
//! 设计说明：
//! - 解析器是纯函数：输入字符串 → ParsedCommand
//! - 执行器有副作用：读取/修改 GameState，读取 Assets
//! - 返回 CommandOutput（文本输出 + 待执行的 Command 列表）
//! - 主循环负责渲染输出并执行 Command

use crate::assets::Assets;
use crate::engine::state::GameState;
use crate::engine::time;
use crate::narrative::command::Command;
use crate::narrative::executor::resolve_action;
use crate::narrative::scene::Scene;

/// 解析后的命令
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ParsedCommand {
    /// 显示背包
    Inventory,
    /// 区域扫描（可选指定 POI）
    AreaListScan { poi_id: Option<String> },
    /// 检查 POI
    Examine { poi_id: String },
    /// 移动到连接地点
    Move { destination: String },
    /// 显示帮助
    Help,
    /// 显示当前时间
    Time,
    /// 进入 NAVI
    Navi,
    /// 调试时间
    DebugTime,
    /// 邮件客户端
    Mail,
    /// 调试场景跳转
    DebugScene { scene_id: String },
    /// 保存进度
    Save { name: Option<String> },
    /// 加载存档
    Load { name: String },
    /// 未知命令
    Unknown(String),
    /// 空输入
    Empty,
}

/// 命令执行结果
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommandOutput {
    /// 要显示的文本行
    pub lines: Vec<String>,
    /// 由主循环执行的命令
    pub commands: Vec<Command>,
}

impl CommandOutput {
    pub fn message(msg: impl Into<String>) -> Self {
        Self {
            lines: vec![msg.into()],
            commands: vec![],
        }
    }

    pub fn empty() -> Self {
        Self {
            lines: vec![],
            commands: vec![],
        }
    }

    pub fn with_transition(scene_id: impl Into<String>) -> Self {
        Self {
            lines: vec![],
            commands: vec![Command::TransitionTo(scene_id.into())],
        }
    }
}

/// 解析输入字符串
pub fn parse(input: &str) -> ParsedCommand {
    let trimmed = input.trim();
    if trimmed.is_empty() {
        return ParsedCommand::Empty;
    }

    let parts: Vec<&str> = trimmed.split_whitespace().collect();
    let cmd = parts[0].to_lowercase();

    match cmd.as_str() {
        "inventory" | "inv" => ParsedCommand::Inventory,
        "arls" => {
            if parts.len() >= 2 {
                ParsedCommand::AreaListScan {
                    poi_id: Some(parts[1].to_string()),
                }
            } else {
                ParsedCommand::AreaListScan { poi_id: None }
            }
        }
        "exper" => {
            if parts.len() >= 2 {
                ParsedCommand::Examine {
                    poi_id: parts[1].to_string(),
                }
            } else {
                ParsedCommand::Unknown("exper 需要指定对象 ID。用法: exper <poi_id>".into())
            }
        }
        "move" => {
            if parts.len() >= 2 {
                ParsedCommand::Move {
                    destination: parts[1].to_string(),
                }
            } else {
                ParsedCommand::Unknown("move 需要指定目的地。用法: move <destination>".into())
            }
        }
        "help" => ParsedCommand::Help,
        "time" => ParsedCommand::Time,
        "navi" => ParsedCommand::Navi,
        "mail" => ParsedCommand::Mail,
        "debug_time" => ParsedCommand::DebugTime,
        "debug_scene" => {
            if parts.len() >= 2 {
                ParsedCommand::DebugScene {
                    scene_id: parts[1].to_string(),
                }
            } else {
                ParsedCommand::Unknown("debug_scene 需要指定场景 ID。用法: debug_scene <scene_id>".into())
            }
        }
        "save" => {
            if parts.len() >= 2 {
                ParsedCommand::Save { name: Some(parts[1].to_string()) }
            } else {
                ParsedCommand::Save { name: None }
            }
        }
        "load" => {
            if parts.len() >= 2 {
                ParsedCommand::Load { name: parts[1].to_string() }
            } else {
                ParsedCommand::Unknown("load 需要指定存档名。用法: load <session_name>".into())
            }
        }
        _ => ParsedCommand::Unknown(format!("未知命令: {}", cmd)),
    }
}

/// 执行命令
pub fn execute(
    cmd: &ParsedCommand,
    gs: &mut GameState,
    assets: &Assets,
    current_scene: &Scene,
) -> CommandOutput {
    match cmd {
        ParsedCommand::Inventory => cmd_inventory(gs),
        ParsedCommand::AreaListScan { poi_id } => cmd_arls(gs, assets, poi_id.as_deref()),
        ParsedCommand::Examine { poi_id } => cmd_exper(gs, assets, current_scene, poi_id),
        ParsedCommand::Move { destination } => cmd_move(gs, assets, current_scene, destination),
        ParsedCommand::Help => cmd_help(gs),
        ParsedCommand::Time => cmd_time(gs),
        ParsedCommand::Navi => {
            let has_navi = gs.has_item("mobile_phone") || gs.has_item("handi_navi");
            if !has_navi {
                CommandOutput::message("你摸了摸口袋——手机不在身上。")
            } else {
                CommandOutput {
                    lines: vec!["进入 NAVI 系统...".into()],
                    commands: vec![Command::EnterNaviMini],
                }
            }
        }
        ParsedCommand::Mail => {
            let has_navi = gs.has_item("mobile_phone") || gs.has_item("handi_navi");
            let at_desktop = gs.player.location == "iwakura_lains_room";
            if !has_navi && !at_desktop {
                CommandOutput::message("你摸了摸口袋——手机不在身上。附近也没有可以使用的设备。")
            } else {
                CommandOutput {
                    lines: vec!["打开邮件客户端...".into()],
                    commands: vec![Command::EnterMail],
                }
            }
        }
        ParsedCommand::DebugTime => cmd_debug_time(gs),
        ParsedCommand::DebugScene { scene_id } => CommandOutput::with_transition(scene_id.clone()),
        ParsedCommand::Save { name } => cmd_save(gs, name.as_deref()),
        ParsedCommand::Load { name } => cmd_load(gs, name),
        ParsedCommand::Unknown(msg) => CommandOutput::message(format!("错误: {}", msg)),
        ParsedCommand::Empty => CommandOutput::empty(),
    }
}

// ============================================================================
// 具体命令实现
// ============================================================================

fn cmd_inventory(gs: &GameState) -> CommandOutput {
    let mut lines = vec!["--- 背包 ---".into()];
    if gs.player.inventory.is_empty() {
        lines.push("  (空)".into());
    } else {
        for (name, qty) in &gs.player.inventory {
            lines.push(format!("  - {}: {}", name, qty));
        }
    }
    lines.push("------------".into());
    CommandOutput { lines, commands: vec![] }
}

fn cmd_arls(gs: &GameState, assets: &Assets, poi_id: Option<&str>) -> CommandOutput {
    let loc_id = &gs.player.location;
    let Some(loc) = assets.get_location(loc_id) else {
        return CommandOutput::message(format!("位置数据损坏: 无法在数据库中定位 '{}'", loc_id));
    };

    // arls <poi_id> — 查看特定 POI 详情
    if let Some(poi_id) = poi_id {
        let Some(poi) = loc.pois.iter().find(|p| p.id == poi_id) else {
            return CommandOutput::message(format!("'{}' 不是这里有效的兴趣点。", poi_id));
        };

        // 如果 POI 有 view_scene_id，跳转到该场景
        if let Some(ref scene_id) = poi.view_scene_id {
            return CommandOutput {
                lines: vec![format!("你察看了 {}: {}", poi.name, poi.description)],
                commands: vec![Command::TransitionTo(scene_id.clone())],
            };
        }

        return CommandOutput::message(format!(
            "你察看了 {}: {}",
            poi.name, poi.description
        ));
    }

    // arls — 区域扫描
    let mut lines = vec!["--- 区域扫描 ---".into()];
    lines.push(assets.get_string(&loc.description).to_string());

    if !loc.pois.is_empty() {
        lines.push("\n兴趣点:\n".into());
        for poi in &loc.pois {
            lines.push(format!("  - {}", poi.name));
        }
    }

    if !loc.connections.is_empty() {
        lines.push("\n连接:\n".into());
        for conn in &loc.connections {
            lines.push(format!("  - {} -> {}", conn.action_id, conn.target_location_id));
        }
    }

    lines.push("----------------".into());
    CommandOutput { lines, commands: vec![] }
}

fn cmd_exper(
    gs: &mut GameState,
    assets: &Assets,
    current_scene: &Scene,
    poi_id: &str,
) -> CommandOutput {
    let loc_id = &gs.player.location;
    let Some(loc) = assets.get_location(loc_id) else {
        return CommandOutput::message(format!("位置数据损坏: 无法在数据库中定位 '{}'", loc_id));
    };

    let Some(poi) = loc.pois.iter().find(|p| p.id == poi_id) else {
        return CommandOutput::message(format!("'{}' 不是这里有效的兴趣点。", poi_id));
    };

    if let Some(ref action_id) = poi.examine_action_id {
        let cmds = resolve_action(action_id, gs, current_scene);
        CommandOutput {
            lines: vec![format!("你检查了 {}。", poi.name)],
            commands: cmds,
        }
    } else {
        CommandOutput::message(format!(
            "你无法以那种方式与 {} 交互。",
            poi.name
        ))
    }
}

fn cmd_move(
    gs: &mut GameState,
    assets: &Assets,
    current_scene: &Scene,
    destination: &str,
) -> CommandOutput {
    let loc_id = &gs.player.location;
    let Some(loc) = assets.get_location(loc_id) else {
        return CommandOutput::message(format!("位置数据损坏: 无法在数据库中定位 '{}'", loc_id));
    };

    let Some(conn) = loc.connections.iter().find(|c| c.action_id == destination) else {
        return CommandOutput::message(format!(
            "你无法从这里移动到 '{}'。",
            destination
        ));
    };

    let cmds = resolve_action(destination, gs, current_scene);
    CommandOutput {
        lines: vec![format!("你前往 {}...", conn.target_location_id)],
        commands: cmds,
    }
}

fn cmd_help(gs: &GameState) -> CommandOutput {
    let mut lines = vec!["--- 帮助 ---".into(), "可用命令:".into()];
    for cmd in &gs.player.unlocked_commands {
        lines.push(format!("  - {}", cmd));
    }
    lines.push("  - quit".into());
    lines.push("------------".into());
    CommandOutput { lines, commands: vec![] }
}

fn cmd_time(gs: &GameState) -> CommandOutput {
    let decoded = time::decode(gs.time_of_day);
    let (h, m) = time::to_hm(decoded.data);
    CommandOutput::message(format!("当前时间: {:02}:{:02}", h, m))
}

fn cmd_save(gs: &GameState, name: Option<&str>) -> CommandOutput {
    let session = name.unwrap_or(&gs.session_name);
    let path = format!("saves/{}.json", session);
    if let Err(e) = std::fs::create_dir_all("saves") {
        return CommandOutput::message(format!("无法创建存档目录: {}", e));
    }
    match serde_json::to_string_pretty(gs) {
        Ok(json) => match std::fs::write(&path, json) {
            Ok(_) => CommandOutput::message(format!("已保存到: {}", path)),
            Err(e) => CommandOutput::message(format!("保存失败: {}", e)),
        },
        Err(e) => CommandOutput::message(format!("序列化失败: {}", e)),
    }
}

fn cmd_load(gs: &mut GameState, name: &str) -> CommandOutput {
    let path = format!("saves/{}.json", name);
    match std::fs::read_to_string(&path) {
        Ok(json) => match serde_json::from_str::<GameState>(&json) {
            Ok(loaded) => {
                *gs = loaded;
                CommandOutput::message(format!("已加载存档: {}", name))
            }
            Err(e) => CommandOutput::message(format!("存档解析错误: {}", e)),
        },
        Err(e) => CommandOutput::message(format!("无法读取存档: {}", e)),
    }
}

fn cmd_debug_time(gs: &GameState) -> CommandOutput {
    let decoded = time::decode(gs.time_of_day);
    let (h, m) = time::to_hm(decoded.data);
    CommandOutput::message(format!(
        "调试时间: raw={}, decoded={}, status={:?}, {:02}:{:02}",
        gs.time_of_day, decoded.data, decoded.status, h, m
    ))
}

// ============================================================================
// 测试
// ============================================================================
#[cfg(test)]
mod tests {
    use super::*;
    use crate::engine::state::GameState;

    #[test]
    fn test_parse_commands() {
        assert!(matches!(parse("inventory"), ParsedCommand::Inventory));
        assert!(matches!(parse("inv"), ParsedCommand::Inventory));
        assert!(matches!(parse("arls"), ParsedCommand::AreaListScan { poi_id: None }));
        assert!(matches!(parse("arls navi"), ParsedCommand::AreaListScan { poi_id: Some(s) } if s == "navi"));
        assert!(matches!(parse("exper navi"), ParsedCommand::Examine { poi_id } if poi_id == "navi"));
        assert!(matches!(parse("move downstairs"), ParsedCommand::Move { destination } if destination == "downstairs"));
        assert!(matches!(parse("help"), ParsedCommand::Help));
        assert!(matches!(parse("time"), ParsedCommand::Time));
        assert!(matches!(parse("navi"), ParsedCommand::Navi));
        assert!(matches!(parse("debug_scene TEST"), ParsedCommand::DebugScene { scene_id } if scene_id == "TEST"));
        assert!(matches!(parse("foo"), ParsedCommand::Unknown(_)));
    }

    #[test]
    fn test_inventory_empty() {
        let gs = GameState::default();
        let out = cmd_inventory(&gs);
        assert!(out.lines.iter().any(|l| l.contains("(空)")));
    }

    #[test]
    fn test_inventory_with_items() {
        let mut gs = GameState::default();
        gs.add_item("milk", 2);
        let out = cmd_inventory(&gs);
        assert!(out.lines.iter().any(|l| l.contains("milk: 2")));
    }
}
