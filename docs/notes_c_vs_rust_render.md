# C 版 vs Rust 版渲染差异

> 2026-05-10 | 对比 `main` 分支 C 代码 vs `rust-rewrite` 分支 Rust 代码

## C 版有、Rust 版缺失的关键功能

### 1. 原地更新时间显示（状态栏）

**C 版**: `render_utils.c:243-253`
```c
void update_time_display_inplace(uint32_t time_of_day) {
    printf("\033[s");          // 保存光标位置
    move_cursor(1, 1);        // 移动到第一行
    print_game_time(time_of_day); // 覆盖打印时间
    printf("\033[u");          // 恢复光标位置
}
```

→ **C 版已经实现了"手机左上角"式常驻时间显示！** 用 ANSI escape 在固定位置刷新。

**Rust 版**: `src/render.rs` 中没有 `update_time_display_inplace`。
只有 `print_time`，在场景切换时调用一次。

---

### 2. Takeover 模式的严格终端流控制

**C 版** (`render_utils.c:298-360`) 做了这些：
- `set_terminal_echo(false)` — 关闭键盘回显
- `tcflush(STDIN_FILENO, TCIFLUSH)` — 丢弃播放期间的所有键盘输入（防止方向键乱码 `^[[A`）
- `\033[s` / `\033[u` — 保存/恢复光标绝对位置
- `\033[L` — 插入新行（把下面内容往下推，而不是滚动）
- `\033[u\033[B` — 恢复光标后下移一行匹配物理位移
- `flush_input_buffer()` — 播放结束后彻底清空输入噪声
- `set_terminal_echo(true)` — 恢复键盘回显

**Rust 版**: `render_takeover` 只是简单地在增量行出现时打印，没有：
- 关闭终端回显
- 清空输入缓冲
- 光标保存/恢复
- 插入行（`\033[L`）

→ **Rust 版 takeover 模式下，玩家按键会打断显示、方向键会产生乱码。**

---

### 3. 图像自适应渲染

**C 版**: `render_image_adaptively` (`render_utils.c:390+`)
- `ioctl(STDOUT_FILENO, TIOCGWINSZ)` 获取终端实际宽高
- 按比例缩放图像到终端可用区域
- 留出安全边距避免自动换行

**Rust 版**: 没有图像自适应渲染逻辑。Logo 和图像是静态的。

---

### 4. Transient Message（短暂提示）

**C 版**: `_render_transient_message`
- 打印一条临时消息，下次渲染时自动清除

**Rust 版**: 没有 transient message 系统。

---

### 5. Raw Mode 终端控制

**C 版**: 用到 `termios.h` — `set_terminal_echo`, `tcflush`

**Rust 版**: Rust 代码中没有 raw mode 控制。`crossterm` 库有这能力，但代码里没有调用。

---

## 结论

C 版渲染层比 Rust 版**复杂得多**。不只是简单的 `printf`，而是：
- **精确的光标管理**（save/restore/move）
- **终端 raw mode 控制**（回显开关、输入缓冲清空）
- **原地刷新**（时间状态栏）
- **终端尺寸自适应**

Rust 版目前只实现了最基础的功能（清屏 + 顺序打印 + 简单颜色）。

---

## 你的决策

这些功能是要在 Rust 版里重写，还是保留 C 版渲染层、Rust 只负责逻辑？
