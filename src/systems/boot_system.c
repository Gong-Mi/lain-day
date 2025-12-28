#include "systems/boot_system.h"
#include "render_utils.h"
#include "string_table.h"
#include "logo_raw_data.h"
#include "character_data.h"
#include "ansi_colors.h"
#include "linenoise.h"
#include "data_loader.h"
#include "game_paths.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <termios.h>
#include <ctype.h>

bool is_valid_session_name(const char* name) {
    if (!name || !*name) return false;
    while (*name) {
        unsigned char c = (unsigned char)*name++;
        if (!isalnum(c) && c != '_' && c != '-') return false;
    }
    return true;
}

bool perform_boot_sequence(GameState* gs, int argc, char** argv, int* arg_index, char* session_file_path_out) {
    bool is_test_mode = false;
    int local_idx = *arg_index;
    int lang_choice = 2; 

    while (local_idx < argc) {
        if (strcmp(argv[local_idx], "--test") == 0) {
            is_test_mode = true;
            local_idx++;
        } else break;
    }
    *arg_index = local_idx;

    enter_fullscreen_mode();
    if (!is_test_mode && *arg_index >= argc) {
        render_image_adaptively(LOGO_DATA, LOGO_WIDTH, LOGO_HEIGHT);
        printf("\n\n%sPress any key or click the image to start...%s", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        fflush(stdout);
        
        enable_raw_mode();
        char c;
        read(STDIN_FILENO, &c, 1);
        tcflush(STDIN_FILENO, TCIFLUSH);
        disable_raw_mode();
        
        set_terminal_echo(false);
        printf("\033[2J\033[H");
        fflush(stdout);
        usleep(100000);

        // --- 启动动画 (全面包裹) ---
        printf("\r\n\r\n");
        printf("%s   CLOSE THE WORLD,%s\r\n", ANSI_COLOR_CYAN, ANSI_COLOR_RESET);
        usleep(1000000); 
        printf("%s           OPEN THE NExT.%s\r\n", ANSI_COLOR_MAGENTA, ANSI_COLOR_RESET);
        usleep(1200000); 
        printf("\r\n");
        
        printf("%s   [ %s WARN %s ]   NO TRUSTED ENVIRONMENT DETECTED.%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_YELLOW, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(500000); 
        printf("%s   [ %s OK %s ]     %sSYSTEM INTEGRITY CHECK COMPLETE.%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_GREEN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(400000); 
        
        printf("%s   [ %s SYSTEM %s ] %sINITIALIZING PROTOCOLS...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_CYAN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(300000); 
        printf("%s   [ %s NET %s ]    %sCONNECTING TO DEFAULT GATEWAY...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_CYAN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(800000); 
        printf("%s   [ %s ERROR %s ]  %sCONNECTION REFUSED (TIMEOUT).%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RED, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(600000); 
        printf("%s   [ %s WARN %s ]   %sREROUTING VIA BACKUP GATEWAY (IPv4)...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_YELLOW, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(700000); 
        printf("%s   [ %s NET %s ]    %sHANDSHAKE INITIATED...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_CYAN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(300000); 
        printf("%s   [ %s OK %s ]     %sCONNECTION ESTABLISHED.%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_GREEN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(500000); 
        printf("%s   [ %s WARN %s ]   %sTARGET_IDENTITY IS NULL (null).%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RED, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(400000); 
        printf("%s   [ %s WARN %s ]   %sATTEMPTING SESSION RECOVERY...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_YELLOW, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(900000); 
        printf("%s   [ %s ERROR %s ]  %sNO ARCHIVE HISTORY FOUND.%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RED, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(500000); 
        printf("%s   [ %s WARN %s ]   %sINITIALIZING WORKSPACE...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_BRIGHT_BLACK, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(300000);
        printf("\r\n");

        printf("%s   [ %s SYSTEM %s ] %sDETECTING LOCALE...%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_CYAN, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(400000);
        printf("%s   [ %s WARN %s ]   %sUNSUPPORTED ENCODING. FALLBACK TO UTF-8.%s\r\n\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_YELLOW, ANSI_COLOR_MID_GRAY, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        usleep(300000);
        
        printf("%s   [ %s QUERY %s ]  SELECT INTERFACE LANGUAGE:%s\r\n", ANSI_COLOR_MID_GRAY, ANSI_COLOR_MAGENTA, ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        printf("%s              1. English (Standard)\n%s", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        printf("%s              2. 简体中文 (Local)\n\n%s", ANSI_COLOR_MID_GRAY, ANSI_COLOR_RESET);
        
        tcflush(STDIN_FILENO, TCIFLUSH);
        disable_raw_mode();

        int chosen = 0;
        char input_prompt[128];
        snprintf(input_prompt, sizeof(input_prompt), ANSI_COLOR_MID_GRAY "   [" ANSI_COLOR_CYAN " INPUT " ANSI_COLOR_MID_GRAY "]  " ANSI_COLOR_MID_GRAY "language_id: ");

        while (chosen == 0) {
            char* line = linenoise(input_prompt);
            if (line == NULL) { restore_terminal_state(); return false; }
            printf(ANSI_COLOR_RESET); // Reset after input ends
            if (strcmp(line, "1") == 0) { lang_choice = 1; chosen = 1; }
            else if (strcmp(line, "2") == 0) { lang_choice = 2; chosen = 1; }
            else {
                printf("\033[A\r\033[K");
                fflush(stdout);
            }
            free(line);
        }
        
        printf(ANSI_COLOR_MID_GRAY "\r\n   [" ANSI_COLOR_GREEN " OK " ANSI_COLOR_MID_GRAY "]     " ANSI_COLOR_MID_GRAY "LOCALIZATION SET TO %s." ANSI_COLOR_RESET "\r\n\r\n", 
               (lang_choice == 1) ? "ENGLISH" : "CHINESE");
        usleep(500000);
    }

    // 3. 会话初始化 (确保输入环境纯净)
    if (is_test_mode) {
        strncpy(gs->session_name, "test_user", MAX_NAME_LENGTH - 1);
        strcpy(session_file_path_out, "test_char.json");
        write_string_to_file(CHARACTER_JSON_DATA, session_file_path_out);
    } else {
        if (is_mouse_supported()) linenoiseSetMouseSupport(1); 
        tcflush(STDIN_FILENO, TCIFLUSH); 

        int error_state = 0;
        while (gs->session_name[0] == '\0') {
            char session_prompt[128];
            if (lang_choice == 1) {
                snprintf(session_prompt, sizeof(session_prompt), ANSI_COLOR_MID_GRAY "   [" ANSI_COLOR_CYAN " INPUT " ANSI_COLOR_MID_GRAY "]  " ANSI_COLOR_MID_GRAY "Session ID: ");
            } else {
                snprintf(session_prompt, sizeof(session_prompt), ANSI_COLOR_MID_GRAY "   [" ANSI_COLOR_CYAN " INPUT " ANSI_COLOR_MID_GRAY "]  " ANSI_COLOR_MID_GRAY "请输入会话 ID: ");
            }

            char* line = linenoise(session_prompt);
            if (line == NULL) return false;
            printf(ANSI_COLOR_RESET); // Reset after input
            
            if (strlen(line) == 0 || strstr(line, "[<") || strstr(line, ";")) {
                free(line);
                printf("\033[A\r\033[K");
                fflush(stdout);
                continue;
            }

            if (is_valid_session_name(line)) {
                strncpy(gs->session_name, line, MAX_NAME_LENGTH - 1);
                error_state = 0;
            } else {
                if (error_state) printf("\033[2A\r\033[K");
                else printf("\033[A\r\033[K");
                const char* err = (lang_choice == 1) ? "   Error: Invalid Session ID." : "   错误：无效的会话名称。";
                printf("%s%s%s\n", ANSI_COLOR_RED, err, ANSI_COLOR_RESET);
                fflush(stdout);
                error_state = 1;
            }
            free(line);
        }

        char session_dir[MAX_PATH_LENGTH];
        snprintf(session_dir, MAX_PATH_LENGTH, "%s/%s", gs->paths.session_root_dir, gs->session_name);
        snprintf(session_file_path_out, MAX_PATH_LENGTH, "%s/character.json", session_dir);
        
        struct stat st = {0};
        if (stat(session_dir, &st) == -1) {
            mkdir(session_dir, 0700);
            write_string_to_file(CHARACTER_JSON_DATA, session_file_path_out);
        } else {
            if (lang_choice == 1) printf("%s   Resuming session '%s'...%s\n", ANSI_COLOR_MID_GRAY, gs->session_name, ANSI_COLOR_RESET);
            else printf("%s   正在恢复会话 '%s'...%s\n", ANSI_COLOR_MID_GRAY, gs->session_name, ANSI_COLOR_RESET);
            usleep(300000);
        }
    }

    return true;
}