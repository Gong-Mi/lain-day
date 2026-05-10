//! 资源加载 —— 字符串表 + 场景库
//!
//! 设计原则：
//! 1. 启动时一次性加载所有 .ssl 和 .json，运行时只读
//! 2. 兼容现有数据格式（strings_extra/*.json 是 {"key": "value"}）
//! 3. 加载失败时给出具体文件路径，方便调试

use std::collections::HashMap;
use std::fs;
use std::path::{Path, PathBuf};

use crate::narrative::scene::Scene;
use crate::world::location::Location;

/// 游戏资源包
#[derive(Debug, Clone)]
pub struct Assets {
    /// 字符串表：text_id -> 文本内容
    pub strings: HashMap<String, String>,
    /// 场景库：scene_id -> Scene
    pub scenes: HashMap<String, Scene>,
    /// 地图：location_id -> Location
    pub locations: HashMap<String, Location>,
}

impl Assets {
    /// 从项目 data/ 目录加载所有资源
    pub fn load_from_data_dir(data_dir: impl AsRef<Path>) -> Result<Self, AssetError> {
        let data_dir = data_dir.as_ref();
        let strings = load_strings(data_dir)?;
        let scenes = load_scenes(data_dir)?;
        let locations = load_locations(data_dir)?;
        Ok(Self {
            strings,
            scenes,
            locations,
        })
    }

    pub fn get_string<'a>(&'a self, id: &'a str) -> &'a str {
        self.strings.get(id).map(|s| s.as_str()).unwrap_or(id)
    }

    pub fn get_scene(&self, id: &str) -> Option<&Scene> {
        self.scenes.get(id)
    }

    pub fn get_location(&self, id: &str) -> Option<&Location> {
        self.locations.get(id)
    }
}

#[derive(Debug, thiserror::Error)]
pub enum AssetError {
    #[error("IO error: {0}")]
    Io(#[from] std::io::Error),
    #[error("JSON parse error in {path}: {source}")]
    Json {
        path: PathBuf,
        #[source]
        source: serde_json::Error,
    },
    #[error("YAML parse error in {path}: {source}")]
    Yaml {
        path: PathBuf,
        #[source]
        source: serde_yaml::Error,
    },
}

fn load_strings(data_dir: &Path) -> Result<HashMap<String, String>, AssetError> {
    let mut strings = HashMap::new();
    let strings_extra_dir = data_dir.join("strings_extra");

    if strings_extra_dir.is_dir() {
        for entry in fs::read_dir(&strings_extra_dir)? {
            let entry = entry?;
            let path = entry.path();
            if path.extension().and_then(|s| s.to_str()) == Some("json") {
                let content = fs::read_to_string(&path)?;
                let map: HashMap<String, String> =
                    serde_json::from_str(&content).map_err(|e| AssetError::Json {
                        path: path.clone(),
                        source: e,
                    })?;
                for (k, v) in map {
                    strings.insert(k, v);
                }
            }
        }
    }

    // 也加载主 strings.json（如果存在且有内容）
    let main_strings = data_dir.join("strings.json");
    if main_strings.is_file() {
        let content = fs::read_to_string(&main_strings)?;
        if !content.trim().is_empty() && content.trim() != "{}" {
            let map: HashMap<String, String> =
                serde_json::from_str(&content).map_err(|e| AssetError::Json {
                    path: main_strings.clone(),
                    source: e,
                })?;
            for (k, v) in map {
                strings.insert(k, v);
            }
        }
    }

    Ok(strings)
}

fn load_scenes(data_dir: &Path) -> Result<HashMap<String, Scene>, AssetError> {
    let mut scenes = HashMap::new();
    let scenes_dir = data_dir.join("scenes");

    if scenes_dir.is_dir() {
        load_scenes_recursive(&scenes_dir, &mut scenes)?;
    }

    Ok(scenes)
}

fn load_scenes_recursive(
    dir: &Path,
    out: &mut HashMap<String, Scene>,
) -> Result<(), AssetError> {
    for entry in fs::read_dir(dir)? {
        let entry = entry?;
        let path = entry.path();
        if path.is_dir() {
            load_scenes_recursive(&path, out)?;
        } else if path.extension().and_then(|s| s.to_str()) == Some("ssl") {
            let content = fs::read_to_string(&path)?;
            let scene: Scene = serde_yaml::from_str(&content).map_err(|e| AssetError::Yaml {
                path: path.clone(),
                source: e,
            })?;
            out.insert(scene.scene_id.clone(), scene);
        }
    }
    Ok(())
}

fn load_locations(data_dir: &Path) -> Result<HashMap<String, Location>, AssetError> {
    let mut locations = HashMap::new();

    for entry in fs::read_dir(data_dir)? {
        let entry = entry?;
        let path = entry.path();
        let fname = path.file_name().and_then(|s| s.to_str()).unwrap_or("");
        if !fname.ends_with(".json") { continue; }

        let content = fs::read_to_string(&path)?;
        let map: HashMap<String, String> = match serde_json::from_str(&content) {
            Ok(m) => m,
            Err(_) => continue, // 跳过非字符串映射的 JSON（如 character.json）
        };

        let mut names: HashMap<String, String> = HashMap::new();
        let mut descs: HashMap<String, String> = HashMap::new();

        for (key, val) in map {
            // 格式 A: MAP_LOCATION_XXX_NAME / MAP_LOCATION_XXX_DESC
            if let Some(base) = key.strip_prefix("MAP_LOCATION_") {
                if let Some(base2) = base.strip_suffix("_NAME") {
                    names.insert(base2.to_lowercase(), val);
                } else if let Some(base2) = base.strip_suffix("_DESC") {
                    descs.insert(base2.to_lowercase(), val);
                }
                continue;
            }
            // 格式 B: TEXT_SCENE_NAME_IWAKURA_UPPER_HALLWAY → iwakura_upper_hallway
            if let Some(base) = key.strip_prefix("TEXT_SCENE_NAME_") {
                names.insert(base.to_lowercase(), val);
                continue;
            }
            // 格式 C: TEXT_IWAKURA_UPPER_HALLWAY_DESC → iwakura_upper_hallway
            if key.starts_with("TEXT_") && key.ends_with("_DESC") {
                let base = &key[5..key.len()-5];
                descs.insert(base.to_lowercase(), val);
                continue;
            }
        }

        for (id, name) in names {
            let desc = descs.get(&id).cloned().unwrap_or_default();
            locations.entry(id.clone()).or_insert_with(|| crate::world::location::Location {
                id: id.clone(), name: name.clone(), description: desc, pois: vec![], connections: vec![],
            });
        }
    }

    Ok(locations)
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_load_real_data() {
        let assets = Assets::load_from_data_dir("data").unwrap();
        // 应该能加载到一些字符串
        assert!(!assets.strings.is_empty());
        // 应该能加载到场景
        assert!(!assets.scenes.is_empty());
        // 至少要有 entry 场景
        assert!(assets.get_scene("SCENE_00_ENTRY").is_some());
    }

    #[test]
    fn test_get_string_fallback() {
        let assets = Assets {
            strings: HashMap::new(),
            scenes: HashMap::new(),
            locations: HashMap::new(),
        };
        // 找不到时返回 id 本身
        assert_eq!(assets.get_string("UNKNOWN_ID"), "UNKNOWN_ID");
    }
}
