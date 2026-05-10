//! 子系统模块 —— NAVI、Train 等
//!
//! 当前原型中，flag 操作已内嵌在 GameState 中。
//! 未来 NAVI 邮件系统、Train 售票机等可在此扩展。

pub mod browser;
pub mod file_manager;
pub mod navi_mini;
pub mod boot;
pub mod events;
pub mod mail;
pub mod mystery;
pub mod train;
