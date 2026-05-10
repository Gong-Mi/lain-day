use std::path::PathBuf;

use lain_day::render::emergency_cleanup;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // 确保 panic 时终端不会残留备用屏幕
    std::panic::set_hook(Box::new(|info| {
        emergency_cleanup();
        eprintln!("Panic: {}", info);
    }));

    // 启动序列（标准终端模式）
    let args: Vec<String> = std::env::args().collect();
    let mut boot_config = lain_day::systems::boot::parse_boot_args(&args);
    if !boot_config.fast_boot && !boot_config.test_mode {
        lain_day::systems::boot::perform_boot_sequence(&mut boot_config)?;
    }

    // 定位 data/ 目录
    let data_dir = find_data_dir()?;
    println!("Using data directory: {}", data_dir.display());

    let mut app = lain_day::app::App::new(data_dir, Some(&boot_config))?;
    app.run()?;

    Ok(())
}

fn find_data_dir() -> Result<PathBuf, Box<dyn std::error::Error>> {
    // 1. 尝试当前目录下的 data/
    let local = PathBuf::from("data");
    if local.is_dir() {
        return Ok(local);
    }

    // 2. 尝试可执行文件所在目录的 data/
    if let Ok(exe) = std::env::current_exe() {
        if let Some(dir) = exe.parent() {
            let sibling = dir.join("data");
            if sibling.is_dir() {
                return Ok(sibling);
            }
        }
    }

    // 3. 尝试上级目录（开发时从 target/debug/ 运行的情况）
    let parent = PathBuf::from("../data");
    if parent.is_dir() {
        return Ok(parent);
    }

    Err("Could not find data/ directory. Please run from project root or ensure data/ is next to the executable.".into())
}
