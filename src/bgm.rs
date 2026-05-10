//! BGM（背景音乐）播放器 —— 纯 Rust 实现（rodio）
//!
//! 设计原则：
//! 1. 不依赖外部命令（mpv/play-audio 等），纯 Rust 音频播放
//! 2. 不阻塞主线程：rodio 在独立音频线程运行
//! 3. 场景绑定：通过 `bgm.yaml` 配置场景/位置 → 曲目映射
//! 4. 支持循环播放和音量控制
//!
//! 支持的格式：mp3, ogg/vorbis, wav, flac, aac, mp4（由 symphonia 解码）

use std::cell::RefCell;
use std::collections::HashMap;
use std::fs::File;
use std::io::{self, BufReader};
use std::path::{Path, PathBuf};
use std::rc::Rc;

use rodio::source::Source;
use serde::Deserialize;

// =============================================================================
// 后端与播放器状态
// =============================================================================

/// 音频后端状态
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Backend {
    /// rodio 成功初始化了音频输出
    Rodio,
    /// 音频设备不可用（无法初始化）
    None,
}

impl Backend {
    /// 是否已启用
    pub fn is_available(&self) -> bool {
        matches!(self, Backend::Rodio)
    }

    pub fn name(&self) -> &'static str {
        match self {
            Backend::Rodio => "rodio",
            Backend::None => "none",
        }
    }
}

/// 内部可变状态
struct BgmPlayerInner {
    backend: Backend,
    /// 音频设备句柄，必须保持存活否则播放会停止
    #[allow(dead_code)]
    device: Option<rodio::MixerDeviceSink>,
    /// 播放器控制器
    player: Option<rodio::Player>,
    current_track: Option<String>,
    volume: u8,
}

/// BGM 播放器
///
/// 使用 `Rc<RefCell<...>>` 包装内部可变状态，使其支持 `Clone`，
/// 以便在需要 `Clone` 的上下文中使用（如 `App` 的测试）。
#[derive(Clone)]
pub struct BgmPlayer {
    inner: Rc<RefCell<BgmPlayerInner>>,
}

impl BgmPlayer {
    /// 创建新播放器（自动尝试初始化 rodio）
    ///
    /// 在 Android/Termux 等无 JNI 上下文的环境中，rodio 初始化可能会 panic。
    /// 我们用 `catch_unwind` 捕获这种情况并优雅回退到 `Backend::None`。
    pub fn new() -> Self {
        match Self::try_init_rodio() {
            Some((device, player)) => Self {
                inner: Rc::new(RefCell::new(BgmPlayerInner {
                    backend: Backend::Rodio,
                    device: Some(device),
                    player: Some(player),
                    current_track: None,
                    volume: 70,
                })),
            },
            None => Self::with_backend(Backend::None),
        }
    }

    /// 使用指定后端创建播放器（主要用于测试）
    pub fn with_backend(backend: Backend) -> Self {
        let (device, player) = if backend == Backend::Rodio {
            Self::try_init_rodio()
                .map(|(d, p)| (Some(d), Some(p)))
                .unwrap_or((None, None))
        } else {
            (None, None)
        };

        Self {
            inner: Rc::new(RefCell::new(BgmPlayerInner {
                backend,
                device,
                player,
                current_track: None,
                volume: 70,
            })),
        }
    }

    /// 尝试初始化 rodio 音频设备。
    /// 单独放在函数里并用 catch_unwind 包裹，防止 ndk-context panic 扩散。
    fn try_init_rodio() -> Option<(rodio::MixerDeviceSink, rodio::Player)> {
        use std::panic;

        let result = panic::catch_unwind(|| {
            match rodio::DeviceSinkBuilder::open_default_sink() {
                Ok(dev) => {
                    let mixer = dev.mixer();
                    let player = rodio::Player::connect_new(&mixer);
                    Some((dev, player))
                }
                Err(e) => {
                    eprintln!("[BGM] Audio device unavailable: {}", e);
                    None
                }
            }
        });

        match result {
            Ok(opt) => opt,
            Err(_) => {
                eprintln!("[BGM] Audio initialization panicked (missing Android context?). Disabling BGM.");
                None
            }
        }
    }

    pub fn backend(&self) -> Backend {
        self.inner.borrow().backend
    }

    pub fn is_available(&self) -> bool {
        self.backend().is_available()
    }

    /// 是否正在播放
    pub fn is_playing(&self) -> bool {
        let inner = self.inner.borrow();
        if let Some(ref player) = inner.player {
            !player.empty() && !player.is_paused()
        } else {
            false
        }
    }

    /// 当前播放的曲目路径（如果有）
    pub fn current_track(&self) -> Option<String> {
        self.inner.borrow().current_track.clone()
    }

    /// 设置音量（0-100，映射到 rodio 的 0.0-1.0）
    pub fn set_volume(&self, volume: u8) {
        let vol = volume.min(100);
        let mut inner = self.inner.borrow_mut();
        inner.volume = vol;
        if let Some(ref player) = inner.player {
            player.set_volume(vol as f32 / 100.0);
        }
    }

    /// 播放指定音频文件（自动循环）
    ///
    /// 如果已有曲目在播放，会先停止旧曲目。
    pub fn play(&self, track_path: impl AsRef<Path>) -> io::Result<()> {
        let path = track_path.as_ref();
        let path_str = path.to_string_lossy().to_string();

        {
            let inner = self.inner.borrow();
            if let Some(ref current) = inner.current_track {
                if current == &path_str {
                    drop(inner);
                    if self.is_playing() {
                        return Ok(());
                    }
                }
            }
        }

        self.stop()?;

        let mut inner = self.inner.borrow_mut();
        if inner.backend == Backend::None {
            return Ok(());
        }

        let file = File::open(path)?;
        let reader = BufReader::new(file);
        let decoder = rodio::Decoder::new(reader)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;
        let source = decoder.repeat_infinite();

        if let Some(ref player) = inner.player {
            player.set_volume(inner.volume as f32 / 100.0);
            player.append(source);
            inner.current_track = Some(path_str);
        }

        Ok(())
    }

    /// 停止当前播放
    pub fn stop(&self) -> io::Result<()> {
        let mut inner = self.inner.borrow_mut();
        if let Some(ref player) = inner.player {
            player.stop();
        }
        inner.current_track = None;
        Ok(())
    }
}

impl Drop for BgmPlayerInner {
    fn drop(&mut self) {
        if let Some(ref player) = self.player {
            player.stop();
        }
    }
}

// =============================================================================
// BGM 配置（场景/位置 → 曲目映射）
// =============================================================================

/// BGM 配置文件结构（bgm.yaml）
#[derive(Debug, Clone, Deserialize, Default, PartialEq)]
pub struct BgmConfig {
    #[serde(default = "default_true")]
    pub enabled: bool,
    pub default_track: Option<String>,
    #[serde(default)]
    pub tracks: HashMap<String, String>,
    #[serde(default)]
    pub location_tracks: HashMap<String, String>,
}

fn default_true() -> bool {
    true
}

impl BgmConfig {
    pub fn from_file(path: impl AsRef<Path>) -> io::Result<Self> {
        let content = std::fs::read_to_string(path)?;
        serde_yaml::from_str(&content)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))
    }

    pub fn track_for_scene(&self, scene_id: &str) -> Option<&str> {
        if !self.enabled {
            return None;
        }
        self.tracks.get(scene_id).map(|s| s.as_str())
    }

    pub fn track_for_location(&self, location_id: &str) -> Option<&str> {
        if !self.enabled {
            return None;
        }
        self.location_tracks
            .get(location_id)
            .map(|s| s.as_str())
            .or(self.default_track.as_deref())
    }
}

// =============================================================================
// BGM 管理器
// =============================================================================

#[derive(Clone)]
pub struct BgmManager {
    pub player: BgmPlayer,
    pub config: BgmConfig,
    data_dir: PathBuf,
}

impl BgmManager {
    pub fn new(data_dir: impl AsRef<Path>) -> io::Result<Self> {
        let data_dir = data_dir.as_ref().to_path_buf();
        let config_path = data_dir.join("bgm.yaml");
        let config = if config_path.exists() {
            BgmConfig::from_file(&config_path)?
        } else {
            BgmConfig::default()
        };

        Ok(Self {
            player: BgmPlayer::new(),
            config,
            data_dir,
        })
    }

    pub fn on_scene_changed(&self, scene_id: &str) -> io::Result<()> {
        if let Some(track) = self.config.track_for_scene(scene_id) {
            let full_path = self.resolve_path(track);
            if full_path.exists() {
                self.player.play(&full_path)?;
            }
        }
        Ok(())
    }

    pub fn on_location_changed(&self, location_id: &str) -> io::Result<()> {
        if let Some(track) = self.config.track_for_location(location_id) {
            let full_path = self.resolve_path(track);
            if full_path.exists() {
                self.player.play(&full_path)?;
            }
        }
        Ok(())
    }

    fn resolve_path(&self, track: &str) -> PathBuf {
        let p = PathBuf::from(track);
        if p.is_absolute() {
            p
        } else {
            self.data_dir.join(track)
        }
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_backend_rodio_or_none() {
        // 只是确保构造函数不 panic
        let player = BgmPlayer::new();
        let _ = player.backend();
    }

    #[test]
    fn test_bgm_player_with_none_backend() {
        let player = BgmPlayer::with_backend(Backend::None);
        assert!(!player.is_available());
        assert!(!player.is_playing());
        player.play("/dev/null/nonexistent.ogg").unwrap();
        assert!(!player.is_playing());
    }

    #[test]
    fn test_bgm_player_stop_idempotent() {
        let player = BgmPlayer::with_backend(Backend::None);
        player.stop().unwrap();
        player.stop().unwrap();
    }

    #[test]
    fn test_volume_clamped() {
        let player = BgmPlayer::with_backend(Backend::None);
        player.set_volume(150);
        assert_eq!(player.inner.borrow().volume, 100);
        player.set_volume(50);
        assert_eq!(player.inner.borrow().volume, 50);
    }

    #[test]
    fn test_bgm_config_parse() {
        let yaml = r#"
enabled: true
default_track: bgm/ambient.ogg
tracks:
  SCENE_00_ENTRY: bgm/prologue.ogg
  SCENE_09_CYBERIA: bgm/cyberia.ogg
location_tracks:
  iwakura_lains_room: bgm/lain_room.ogg
"#;
        let config: BgmConfig = serde_yaml::from_str(yaml).unwrap();
        assert!(config.enabled);
        assert_eq!(config.default_track.as_deref(), Some("bgm/ambient.ogg"));
        assert_eq!(
            config.track_for_scene("SCENE_00_ENTRY"),
            Some("bgm/prologue.ogg")
        );
        assert_eq!(
            config.track_for_location("iwakura_lains_room"),
            Some("bgm/lain_room.ogg")
        );
    }

    #[test]
    fn test_bgm_config_disabled() {
        let yaml = r#"
enabled: false
tracks:
  SCENE_00_ENTRY: bgm/prologue.ogg
"#;
        let config: BgmConfig = serde_yaml::from_str(yaml).unwrap();
        assert!(!config.enabled);
        assert_eq!(config.track_for_scene("SCENE_00_ENTRY"), None);
    }

    #[test]
    fn test_bgm_config_default_true() {
        let yaml = r#"
tracks:
  SCENE_A: a.ogg
"#;
        let config: BgmConfig = serde_yaml::from_str(yaml).unwrap();
        assert!(config.enabled);
    }

    #[test]
    fn test_bgm_manager_resolve_path() {
        let dir = std::env::temp_dir();
        let manager = BgmManager {
            player: BgmPlayer::with_backend(Backend::None),
            config: BgmConfig::default(),
            data_dir: dir.clone(),
        };
        assert_eq!(
            manager.resolve_path("bgm/test.ogg"),
            dir.join("bgm/test.ogg")
        );
        assert_eq!(
            manager.resolve_path("/absolute/path.ogg"),
            PathBuf::from("/absolute/path.ogg")
        );
    }

    #[test]
    fn test_backend_name() {
        assert_eq!(Backend::Rodio.name(), "rodio");
        assert_eq!(Backend::None.name(), "none");
    }
}
