#include <stdio.h>
#include "art_batch.h" // 确保你已经运行了 python convert_batch.py 生成了这个头文件

// 简单的颜色结构体
typedef struct {
    unsigned char r, g, b;
} Color;

// 比较颜色是否相同，用于去重
int color_eq(Color c1, unsigned char r, unsigned char g, unsigned char b) {
    return c1.r == r && c1.g == g && c1.b == b;
}

int main() {
    // 缓冲区优化
    // 16KB (16384) 是一个在 Termux 上比较平衡的值
    // 太小会导致系统调用频繁（慢），太大(128KB)会导致视觉卡顿（一顿一顿的）
    char buffer[163840]; 
    setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));

    // 初始化状态
    Color last_fg = {0, 0, 0};
    Color last_bg = {0, 0, 0};
    int is_first = 1;

    // 隐藏光标
    printf("\x1b[?25l");

    // 遍历所有预处理好的块 (CHUNK_COUNT 在 art_batch.h 中定义)
    for (int i = 0; i < CHUNK_COUNT; i++) {
        DrawChunk c = art_chunks[i];

        if (c.is_newline) {
            // 换行前重置颜色，防止背景色溢出
            printf("\x1b[0m\n");
            is_first = 1; // 换行后强制刷新颜色状态
            continue;
        }

        // 只有颜色改变时，才发送 ANSI 指令
        if (is_first || !color_eq(last_fg, c.fr, c.fg, c.fb)) {
            printf("\x1b[38;2;%d;%d;%dm", c.fr, c.fg, c.fb);
            last_fg.r = c.fr; last_fg.g = c.fg; last_fg.b = c.fb;
        }

        if (is_first || !color_eq(last_bg, c.br, c.bg, c.bb)) {
            printf("\x1b[48;2;%d;%d;%dm", c.br, c.bg, c.bb);
            last_bg.r = c.br; last_bg.g = c.bg; last_bg.b = c.bb;
        }

        is_first = 0;

        // 输出内容块
        printf("%s", c.content);
    }

    // 恢复光标和颜色
    printf("\x1b[0m");
    printf("\x1b[?25h");
    printf("\n");
    
    // 强制刷新剩余缓冲区
    fflush(stdout);

    return 0;
}