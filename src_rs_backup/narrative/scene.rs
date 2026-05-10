//! 场景数据结构 —— 可直接从 .ssl (YAML) 反序列化
//!
//! 不再需要 parse_scenes.py 生成 C 代码。
//! serde_yaml 在运行时直接解析 YAML → Scene 结构体。

use serde::Deserialize;

/// 说话人枚举 —— 编译期穷举，杜绝非法 speaker_id
#[derive(Debug, Clone, Copy, PartialEq, Eq, Deserialize)]
#[serde(rename_all = "SCREAMING_SNAKE_CASE")]
pub enum Speaker {
    #[serde(alias = "SPEAKER_NONE")]
    None,
    #[serde(alias = "SPEAKER_LAIN")]
    Lain,
    #[serde(alias = "SPEAKER_MOM")]
    Mom,
    #[serde(alias = "SPEAKER_DAD")]
    Dad,
    #[serde(alias = "SPEAKER_ALICE")]
    Alice,
    #[serde(alias = "SPEAKER_CHISA")]
    Chisa,
    #[serde(alias = "SPEAKER_MIKA")]
    Mika,
    #[serde(alias = "SPEAKER_GHOST")]
    Ghost,
    #[serde(alias = "SPEAKER_DOCTOR")]
    Doctor,
    #[serde(alias = "SPEAKER_NAVI")]
    Navi,
    #[serde(alias = "SPEAKER_SHU")]
    Shu,
    #[serde(alias = "SPEAKER_PARENT")]
    Parent,
}

/// 对话行
#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct DialogueLine {
    pub speaker: Speaker,
    pub text_id: String,
    #[serde(default)]
    pub delay: f32, // 秒
    #[serde(default)]
    pub duration: f32, // 秒
}

impl DialogueLine {
    /// 延迟转毫秒（供渲染器使用）
    pub fn delay_ms(&self) -> u64 {
        (self.delay * 1000.0) as u64
    }
}

/// 场景定义 —— 对应一个 .ssl 文件
#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct Scene {
    pub scene_id: String,
    pub name_text_id: String,
    pub location_id: String,
    #[serde(default)]
    pub is_takeover: bool,
    #[serde(default)]
    pub description: Option<String>,
    #[serde(default)]
    pub dialogue: Vec<DialogueLine>,
    #[serde(default)]
    pub choices: Vec<super::choice::Choice>,
    #[serde(default)]
    pub auto_events: Vec<super::choice::AutoEvent>,
}

impl Scene {
    /// 从 YAML 字符串加载场景
    pub fn from_yaml(yaml: &str) -> Result<Self, serde_yaml::Error> {
        serde_yaml::from_str(yaml)
    }

    /// 查找指定 action_id 的选项
    pub fn find_choice(&self, action_id: &str) -> Option<&super::choice::Choice> {
        self.choices.iter().find(|c| c.action_id == action_id)
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_yaml() {
        let yaml = r#"
scene_id: SCENE_00_ENTRY
name_text_id: TEXT_SCENE_NAME_00_ENTRY
location_id: iwakura_upper_hallway
is_takeover: true
dialogue:
  - speaker: SPEAKER_NONE
    text_id: TEXT_PROLOGUE_LINE_1
    delay: 0.0
  - speaker: SPEAKER_NONE
    text_id: TEXT_PROLOGUE_LINE_2
    delay: 1.0
choices:
  - text_id: TEXT_CHOICE_OPEN_DOOR
    action_id: open_door_broken
    target_scene: SCENE_01_LAIN_ROOM_BROKEN
    delay: 0.0
"#;
        let scene = Scene::from_yaml(yaml).unwrap();
        assert_eq!(scene.scene_id, "SCENE_00_ENTRY");
        assert!(scene.is_takeover);
        assert_eq!(scene.dialogue.len(), 2);
        assert_eq!(scene.dialogue[1].delay_ms(), 1000);
        assert_eq!(scene.choices.len(), 1);
        assert_eq!(scene.choices[0].action_id, "open_door_broken");
        assert_eq!(scene.choices[0].target_scene, Some("SCENE_01_LAIN_ROOM_BROKEN".to_string()));
    }

    #[test]
    fn test_find_choice() {
        let yaml = r#"
scene_id: TEST
target_scene: null
name_text_id: TEXT_TEST
location_id: test
choices:
  - action_id: go_left
    text_id: TEXT_LEFT
  - action_id: go_right
    text_id: TEXT_RIGHT
"#;
        let scene = Scene::from_yaml(yaml).unwrap();
        assert!(scene.find_choice("go_left").is_some());
        assert!(scene.find_choice("go_up").is_none());
    }
}
