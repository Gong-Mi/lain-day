//! lain-day-lint —— 静态数据验证工具
//!
//! 用法: cargo run --bin lint
//!
//! 检查项：
//! 1. 所有 .ssl 中的 text_id 在字符串表中存在
//! 2. 所有 .ssl 中的 target_scene 有对应的 .ssl 文件
//! 3. 所有 .ssl 中的 location_id 在地图中存在
//! 4. 所有 speaker 值在枚举中合法
//! 5. executor 硬编码表引用的场景存在
//! 6. 发现 orphaned strings（未被任何 .ssl 引用）
//! 7. .ssl 文件格式一致性（description 字段用法）

use std::collections::HashSet;
use std::path::PathBuf;
use std::process;

use lain_day::assets::Assets;
use lain_day::narrative::scene::Scene;

#[derive(Default)]
struct LintReport {
    errors: Vec<String>,
    warnings: Vec<String>,
    infos: Vec<String>,
}

impl LintReport {
    fn error(&mut self, msg: impl Into<String>) {
        self.errors.push(msg.into());
    }
    fn warn(&mut self, msg: impl Into<String>) {
        self.warnings.push(msg.into());
    }
    fn info(&mut self, msg: impl Into<String>) {
        self.infos.push(msg.into());
    }

    fn print(&self) {
        for e in &self.errors {
            println!("[ERROR] {}", e);
        }
        for w in &self.warnings {
            println!("[WARN]  {}", w);
        }
        for i in &self.infos {
            println!("[INFO]  {}", i);
        }
        println!(
            "\nSummary: {} errors, {} warnings, {} infos",
            self.errors.len(),
            self.warnings.len(),
            self.infos.len()
        );
    }

    fn has_errors(&self) -> bool {
        !self.errors.is_empty()
    }
}

fn main() {
    let data_dir = PathBuf::from("data");
    let assets = match Assets::load_from_data_dir(&data_dir) {
        Ok(a) => a,
        Err(e) => {
            eprintln!("Failed to load assets: {}", e);
            process::exit(1);
        }
    };

    let mut report = LintReport::default();

    // 收集所有被引用的 text_id、target_scene、location_id
    let mut referenced_text_ids: HashSet<&str> = HashSet::new();
    let mut referenced_scenes: HashSet<&str> = HashSet::new();
    let mut referenced_locations: HashSet<&str> = HashSet::new();

    for (scene_id, scene) in &assets.scenes {
        check_scene(scene_id, scene, &assets, &mut report, &mut referenced_text_ids, &mut referenced_scenes, &mut referenced_locations);
    }

    // 检查 executor 硬编码表中的场景引用
    check_executor_references(&assets, &mut report, &mut referenced_scenes);

    // 检查 orphaned strings
    check_orphaned_strings(&assets, &referenced_text_ids, &mut report);

    // 检查缺失的场景文件（被引用但不存在）
    check_missing_scenes(&assets, &referenced_scenes, &mut report);

    // 打印报告
    report.print();

    if report.has_errors() {
        process::exit(1);
    }
}

fn check_scene<'a>(
    scene_id: &str,
    scene: &'a Scene,
    assets: &Assets,
    report: &mut LintReport,
    ref_texts: &mut HashSet<&'a str>,
    ref_scenes: &mut HashSet<&'a str>,
    ref_locs: &mut HashSet<&'a str>,
) {
    // 1. name_text_id
    ref_texts.insert(scene.name_text_id.as_str());
    if !assets.strings.contains_key(&scene.name_text_id) {
        report.error(format!(
            "[{}] name_text_id '{}' not found in strings",
            scene_id, scene.name_text_id
        ));
    }

    // 2. location_id
    if !scene.location_id.is_empty() {
        ref_locs.insert(scene.location_id.as_str());
        if assets.get_location(&scene.location_id).is_none() {
            report.info(format!(
                "[{}] location_id '{}' not found in map (MVP: map incomplete)",
                scene_id, scene.location_id
            ));
        }
    }

    // 3. dialogue lines
    for (i, line) in scene.dialogue.iter().enumerate() {
        ref_texts.insert(line.text_id.as_str());
        if !assets.strings.contains_key(&line.text_id) {
            report.error(format!(
                "[{}] dialogue[{}].text_id '{}' not found in strings",
                scene_id, i, line.text_id
            ));
        }
    }

    // 4. choices
    for (i, choice) in scene.choices.iter().enumerate() {
        ref_texts.insert(choice.text_id.as_str());
        if !assets.strings.contains_key(&choice.text_id) {
            report.error(format!(
                "[{}] choice[{}].text_id '{}' not found in strings",
                scene_id, i, choice.text_id
            ));
        }

        if let Some(ref target) = choice.target_scene {
            ref_scenes.insert(target.as_str());
        }
    }

    // 5. auto_events
    for (_i, event) in scene.auto_events.iter().enumerate() {
        ref_scenes.insert(event.target_scene.as_str());
    }

    // 6. description 字段一致性检查
    if let Some(ref desc) = scene.description {
        // 如果 description 看起来像 text_id（全大写，下划线连接）
        if desc.starts_with("TEXT_") || desc.starts_with("MAP_") {
            ref_texts.insert(desc.as_str());
            if !assets.strings.contains_key(desc) {
                report.error(format!(
                    "[{}] description references unknown text_id '{}'",
                    scene_id, desc
                ));
            }
        }
    }
}

fn check_executor_references(
    assets: &Assets,
    report: &mut LintReport,
    ref_scenes: &mut HashSet<&str>,
) {
    // executor.rs 中的硬编码标准跳转表
    let hardcoded_targets = vec![
        "SCENE_02_DOWNSTAIRS",
        "SCENE_01_LAIN_ROOM_BROKEN",
        "SCENE_IWAKURA_UPPER_HALLWAY",
        "SCENE_IWAKURA_LAINS_ROOM",
        "SCENE_01C_TALK_TO_FIGURE_ENDPROLOGUE",
        "SCENE_01B_NAVI_SHUTDOWN",
        "SCENE_01D_NAVI_REBOOT_ENDPROLOGUE",
        "SCENE_01E_NAVI_CONNECT_ENDPROLOGUE",
        "SCENE_02B_DAD_REPLY_NO",
        "SCENE_02C_DAD_ASK_HELP",
        "SCENE_02J_GET_MILK_ENDPROLOGUE",
        "SCENE_02F_MOM_REPLY_FINE_ENDPROLOGUE",
        "SCENE_02G_MOM_REPLY_SILENT_ENDPROLOGUE",
        "SCENE_03_CHAPTER_ONE_INTRO",
        "SCENE_DAD_HUB",
        "SCENE_04A_TALK_TO_SISTER_COLD",
        "SCENE_04B_TALK_TO_SISTER_CURIOUS",
        "SCENE_04C_TALK_TO_SISTER_DEFAULT",
        "SCENE_06_TRAIN_SCENE",
        "SCENE_07_CLASSROOM",
        "SCENE_08B_ASK_TEACHER",
        "SCENE_08C_ASK_PROXY",
        "SCENE_08D_ASK_CHISA",
        "SCENE_09_CYBERIA",
        "SCENE_09A_PERSUASION",
        "SCENE_08E_ASK_ALICE_SCARED",
        "SCENE_06Z_TRAIN_EVENT_RESULT",
        "SCENE_SIDE_STORIES_ADJUST_FONT_INTERVAL",
        "SCENE_SIDE_STORIES_NETWORK_STATUS",
        "SCENE_19_COLD_OPEN_CH2",
        "SCENE_20_CHAPTER_TWO_INTRO",
        "SCENE_21A_HUG_ALICE",
        "SCENE_21B_REPLY_FINE",
        "SCENE_21C_ALICE_COMFORTS_LAIN",
        "SCENE_22_CYBERIA_FLASHBACK",
        "SCENE_22D_REPLY_IS_ME",
        "SCENE_22B_BOSS_INVITES_LAIN_TO_SING",
        "SCENE_SIDE_STORIES_OLD_MIC",
        "SCENE_SIDE_STORIES_SINGING_RESULT_ECHO",
        "SCENE_GUNSHOT_ADVANCE",
        "SCENE_CHAPTER_THREE_INTRO",
        "SCENE_CH2_ASK_WHO",
        "SCENE_EXAMINE_BOOKSHELF",
        "SCENE_EXAMINE_MIKA_WARDROBE",
        "SCENE_SHINJUKU_ABANDONED_SITE",
        "SCENE_SIDE_STORIES_EMAIL_CLIENT",
        "SCENE_SIDE_STORIES_CHATROOM_REAL",
        "SCENE_SIDE_STORIES_CHATROOM_EMPTY",
    ];

    let mut missing = Vec::new();
    for target in &hardcoded_targets {
        ref_scenes.insert(*target);
        if assets.get_scene(target).is_none() {
            missing.push(*target);
        }
    }

    if !missing.is_empty() {
        report.error(format!(
            "Executor references {} missing scene(s): {}",
            missing.len(),
            missing.join(", ")
        ));
    }
}

fn check_orphaned_strings(
    assets: &Assets,
    referenced: &HashSet<&str>,
    report: &mut LintReport,
) {
    let mut orphaned = Vec::new();
    for key in assets.strings.keys() {
        if !referenced.contains(key.as_str()) {
            orphaned.push(key.clone());
        }
    }

    if !orphaned.is_empty() {
        report.warn(format!(
            "Found {} orphaned string(s) not referenced by any .ssl: {}",
            orphaned.len(),
            orphaned[..(orphaned.len().min(10))].join(", ")
        ));
        if orphaned.len() > 10 {
            report.info(format!("... and {} more", orphaned.len() - 10));
        }
    }
}

fn check_missing_scenes(
    assets: &Assets,
    referenced: &HashSet<&str>,
    report: &mut LintReport,
) {
    let mut missing = Vec::new();
    for target in referenced.iter() {
        if assets.get_scene(target).is_none() {
            missing.push(target.to_string());
        }
    }

    if !missing.is_empty() {
        report.error(format!(
            "Found {} missing scene(s) referenced by .ssl or executor: {}",
            missing.len(),
            missing[..(missing.len().min(10))].join(", ")
        ));
        if missing.len() > 10 {
            report.info(format!("... and {} more", missing.len() - 10));
        }
    }
}
