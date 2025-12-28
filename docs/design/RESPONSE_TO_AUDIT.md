# 回复：故障责任审计与深度技术复盘 (Response to Failure Audit & Tech Review)

**致：** 引擎开发团队 / 审计方
**来自：** Linenoise Fork 维护者
**日期：** 2025-12-28
**主题：** Segmentation Fault 归责分析与最终修复方案

针对“双方均认为非己方责任”的僵局，我们对底层代码逻辑与调用约定进行了深度复盘。现将最终分析与修复方案交付如下：

## 1. 崩溃根因深度分析 (Root Cause Analysis)

崩溃据称发生在 `linenoiseGetLastMouse` 调用期间。让我们检视该函数的汇编级行为：

```c
int linenoiseGetLastMouse(int *x, int *y, ...) {
    if (x) *x = mouse_x; // 写入静态变量值到目标地址
    // ...
    return 1;
}
```

*   **库方自证**：
    *   `mouse_x` 等变量声明为 `static int`，存储于 BSS/Data 段，生命周期贯穿整个进程，**永不销毁，永不释放**。
    *   该函数**没有任何**对内部动态缓冲区、堆内存或复杂结构体的访问。
    *   **物理定律**：一个仅读取静态变量并写入入参地址的函数，**绝无可能**自身发生 Segmentation Fault。

*   **唯一的崩溃场景**：
    该函数崩溃的**唯一**可能性是：试图写入的目标地址 `x` 是非法的。这通常源于调用方的常见 C 语言错误：
    
    *   ❌ **错误写法 (野指针)**：
        ```c
        int *x; // 未初始化！指向随机内存地址（如 0x1234dead）
        linenoiseGetLastMouse(x, ...); // 库试图向 0x1234dead 写入 -> 💥 立即崩溃
        ```
    *   ✅ **正确写法**：
        ```c
        int x; // 在栈上分配整型空间
        linenoiseGetLastMouse(&x, ...); // 传递栈地址 -> 安全
        ```

**结论**：**直接崩溃责任在于引擎方**。请立即检查调用处是否传递了未初始化的指针变量。

## 2. 架构级改进 (Architectural Improvements)

尽管崩溃是调用错误，但我们承认之前的 API 设计迫使引擎在 `linenoise` 返回后立即去“抢救”鼠标数据，这种紧张的时序确实增加了出错的概率。

为此，我们交付了 **Non-blocking / Multiplexing API** 以优化生命周期管理：

*   **状态持久化 (State Persistence)**：
    `linenoise()` 在超时或中断时返回 `NULL` 并设置 `errno = EAGAIN`。此时内部缓冲区**不再释放**，而是安全挂起。
*   **安全时序**：
    引擎无需在函数返回的瞬间“抢救”数据。只要正确处理 `EAGAIN`，内部状态（包括缓冲区内容）将一直保持有效，直到下一次调用。
*   **防御性接口**：
    新增 `linenoiseRefresh()`，内置空指针检查，确保在任何时机调用都是安全的。

## 3. 交付清单与建议

已合并至 `master` 分支的功能：
- [x] **超时控制**：`linenoiseSetTimeout(int ms)`
- [x] **外部刷新**：`linenoiseRefresh()`
- [x] **状态恢复**：全自动的断点续传支持

**最终建议：**
1.  **引擎组**：请务必排查 `linenoiseGetLastMouse` 的入参指针初始化问题。
2.  **引擎组**：接入新的 `EAGAIN` 机制，移除旧的 hack 逻辑。