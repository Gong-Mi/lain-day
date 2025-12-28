#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <signal.h>
#include <pthread.h>

#include "game_types.h"
#include "game_paths.h"
#include "render_utils.h"
#include "string_table.h"
#include "string_id_names.h"
#include "systems/boot_system.h"

// 引用全局状态
extern struct GameState* game_state;
extern const char* g_embedded_strings[TEXT_COUNT];

void handle_signal(int sig) { (void)sig; }

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    init_terminal_state();
    
    // 初始化字符串表（开机文字需要）
    init_string_table(g_embedded_strings, TEXT_COUNT);
    
    // 分配最基础的 GameState
    game_state = malloc(sizeof(GameState));
    memset(game_state, 0, sizeof(GameState));
    init_paths(argv[0], &game_state->paths);

    printf("=== Boot Sequence Debugger ===\n");

    char character_file_path[MAX_PATH_LENGTH] = {0};
    int arg_index = 1;

    // 执行你想要调试的开机序列
    if (perform_boot_sequence(game_state, argc, argv, &arg_index, character_file_path)) {
        printf("\n[DEBUG] Boot Sequence Successful!\n");
        printf("[DEBUG] Session Name: %s\n", game_state->session_name);
        printf("[DEBUG] Character File: %s\n", character_file_path);
    } else {
        printf("\n[DEBUG] Boot Sequence Cancelled or Failed.\n");
    }

    // 无论成功失败，恢复终端
    restore_terminal_state();
    
    free(game_state);
    return 0;
}
