//! 选项（Choice）和自动事件（AutoEvent）

use serde::Deserialize;
use super::conditions::Condition;

/// 玩家选项
#[derive(Debug, Clone, PartialEq, Deserialize)]
pub struct Choice {
    pub text_id: String,
    pub action_id: String,
    #[serde(default)]
    pub target_scene: Option<String>,
    #[serde(default)]
    pub delay: f32,
    #[serde(default)]
    pub conditions: Vec<Condition>,
}

impl Choice {
    pub fn delay_ms(&self) -> u64 {
        (self.delay * 1000.0) as u64
    }
}

/// 自动触发的事件（基于时间）
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct AutoEvent {
    pub target_scene: String,
    #[serde(default)]
    pub wait_time: u64, // 秒
    #[serde(default)]
    pub flag_set: Option<String>,
    #[serde(default)]
    pub conditions: Vec<Condition>,
}
