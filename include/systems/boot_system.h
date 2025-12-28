#ifndef BOOT_SYSTEM_H
#define BOOT_SYSTEM_H

#include "game_types.h"

/**
 * 执行完整的开机引导程序：
 * 1. 全屏 Logo 展示
 * 2. "CLOSE THE WORLD" 启动序列
 * 3. 会话名称输入与目录创建
 * 
 * 返回：如果初始化成功并准备进入游戏，返回 true；如果用户取消或出错，返回 false。
 */
bool perform_boot_sequence(GameState* gs, int argc, char** argv, int* arg_index, char* session_file_path_out);
bool is_valid_session_name(const char* name);

#endif
