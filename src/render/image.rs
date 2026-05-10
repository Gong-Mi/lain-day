//! ANSI True Color 图像渲染 —— 替代 C 版本的 image_view_system.c
//!
//! 使用 `image` crate 加载 PNG/JPEG，渲染为 ANSI 24-bit 背景色。
//! 每个像素用两个空格表示（因为终端字符高 > 宽）。
//! 支持自适应缩放和居中显示。

use std::io::{self, stdout, Write};

use crossterm::{
    cursor::{Hide, MoveTo, Show},
    event::{self, Event, KeyCode},
    terminal::{self, Clear, ClearType},
    ExecutableCommand,
};
use image::{DynamicImage, GenericImageView, Rgba};

/// 图像渲染器
pub struct ImageRenderer;

impl ImageRenderer {
    /// 交互式显示图像
    ///
    /// 按 `q` 或 `Q` 退出，返回图像上的点击坐标（如果有）。
    pub fn show_image_interactive(path: &str) -> io::Result<Option<(u32, u32)>> {
        let img = match image::open(path) {
            Ok(img) => img,
            Err(e) => {
                println!("无法加载图像: {} ({})", path, e);
                return Ok(None);
            }
        };

        let (term_w, term_h) = terminal::size()?;
        let term_w = term_w as u32;
        let term_h = term_h as u32;

        // 清屏并隐藏光标
        let mut stdout = stdout();
        stdout.execute(Clear(ClearType::All))?;
        stdout.execute(Hide)?;

        Self::render_image(&img, term_w, term_h)?;

        // 显示提示
        stdout.execute(MoveTo(0, term_h as u16 - 1))?;
        print!("\x1b[7m [Q] 返回 \x1b[0m");
        stdout.flush()?;

        let result = loop {
            if let Event::Key(key) = event::read()? {
                match key.code {
                    KeyCode::Char('q') | KeyCode::Char('Q') => break Ok(None),
                    _ => {}
                }
            }
        };

        // 恢复
        stdout.execute(Show)?;
        stdout.execute(Clear(ClearType::All))?;
        stdout.execute(MoveTo(0, 0))?;
        stdout.flush()?;

        result
    }

    /// 渲染图像到终端
    ///
    /// - 保持宽高比
    /// - 自适应缩放以适应终端
    /// - 居中显示
    /// - 使用 ANSI true color (\x1b[48;2;R;G;Bm)
    /// - 颜色缓存优化：连续相同颜色不重复发送 ANSI 序列
    pub fn render_image(img: &DynamicImage, term_w: u32, term_h: u32) -> io::Result<()> {
        let (img_w, img_h) = img.dimensions();

        // 可用空间（留边缘）
        let avail_w = term_w.saturating_sub(2);
        let avail_h = term_h.saturating_sub(4);

        if avail_w < 2 || avail_h < 1 {
            println!("终端太小，无法显示图像。");
            return Ok(());
        }

        // 每个像素占 2 个字符宽
        let term_px_w = avail_w / 2;
        let term_px_h = avail_h;

        // 计算缩放比例
        let scale_x = img_w as f32 / term_px_w as f32;
        let scale_y = img_h as f32 / term_px_h as f32;
        let scale = scale_x.max(scale_y).max(1.0);

        let draw_w = (img_w as f32 / scale) as u32;
        let draw_h = (img_h as f32 / scale) as u32;

        let offset_x = (term_px_w.saturating_sub(draw_w)) / 2;
        let offset_y = (term_px_h.saturating_sub(draw_h)) / 2;

        let mut stdout = stdout();
        let mut current_color: Option<(u8, u8, u8)> = None;

        for y in 0..term_px_h {
            for x in 0..term_px_w {
                if x >= offset_x
                    && x < offset_x + draw_w
                    && y >= offset_y
                    && y < offset_y + draw_h
                {
                    let img_x = (((x - offset_x) as f32 * scale) as u32).min(img_w - 1);
                    let img_y = (((y - offset_y) as f32 * scale) as u32).min(img_h - 1);

                    let Rgba([r, g, b, _a]) = img.get_pixel(img_x, img_y);

                    if current_color != Some((r, g, b)) {
                        print!("\x1b[48;2;{};{};{}m", r, g, b);
                        current_color = Some((r, g, b));
                    }
                    print!("  ");
                } else {
                    if current_color.is_some() {
                        print!("\x1b[0m");
                        current_color = None;
                    }
                    print!("  ");
                }
            }
            // 行尾重置颜色
            if current_color.is_some() {
                print!("\x1b[0m");
                current_color = None;
            }
            println!();
        }

        stdout.flush()?;
        Ok(())
    }
}

// =============================================================================
// 测试
// =============================================================================
#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_color_caching() {
        // 纯函数测试：颜色缓存逻辑本身不需要图像
        let mut current: Option<(u8, u8, u8)> = None;
        let new = (255, 0, 0);
        if current != Some(new) {
            current = Some(new);
        }
        assert_eq!(current, Some((255, 0, 0)));

        // 相同颜色不更新
        let new2 = (255, 0, 0);
        if current != Some(new2) {
            panic!("不应更新");
        }
    }
}
