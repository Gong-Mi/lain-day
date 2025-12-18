#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

// 启用鼠标追踪的转义序列
#define MOUSE_ENABLE  "\x1b[?1000h\x1b[?1006h" // 1000: Report mouse click; 1006: SGR Extended Mouse Mode
#define MOUSE_DISABLE "\x1b[?1000l\x1b[?1006l"

struct termios orig_termios;

void disable_raw_mode() {
    printf(MOUSE_DISABLE);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\nExiting viewer.\n");
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON); // Disable echo and canonical mode
    raw.c_cc[VMIN] = 0;  // Return immediately if input is available
    raw.c_cc[VTIME] = 1; // Wait 100ms

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(MOUSE_ENABLE);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    const char *filename = "image.bin";
    if (argc > 1) filename = argv[1];

    // 1. Load Image
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Failed to open image file");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(fsize + 1);
    fread(buffer, 1, fsize, f);
    fclose(f);
    buffer[fsize] = 0;

    // 2. Enable Raw Mode & Mouse
    enable_raw_mode();

    // 3. Draw Image (Fastest Output)
    // Clear screen first? \x1b[2J\x1b[H
    write(STDOUT_FILENO, "\x1b[2J\x1b[H", 7);
    write(STDOUT_FILENO, buffer, fsize);
    
    char msg[] = "\n[TOUCH TEST MODE] Click anywhere. Press 'q' to quit.\n";
    write(STDOUT_FILENO, msg, strlen(msg));

    // 4. Input Loop
    char ibuf[32];
    while (1) {
        int n = read(STDIN_FILENO, ibuf, sizeof(ibuf) - 1);
        if (n > 0) {
            ibuf[n] = '\0';
            
            // Check for Quit
            if (ibuf[0] == 'q' || ibuf[0] == 'Q') break;

            // Check for ANSI Escape Sequence (Mouse)
            // Format (SGR): \x1b[<0;x;yM  (press) or \x1b[<0;x;ym (release)
            if (ibuf[0] == '\x1b' && ibuf[1] == '[' && ibuf[2] == '<') {
                int btn, x, y;
                char type;
                // Parse: <btn>;<x>;<y><M/m>
                if (sscanf(ibuf + 3, "%d;%d;%d%c", &btn, &x, &y, &type) == 4) {
                    if (type == 'M') { // Press
                        char output[64];
                        int len = snprintf(output, sizeof(output), "\033[s\033[1;1H\033[41m CLICK: X=%d Y=%d \033[0m\033[u", x, y);
                        write(STDOUT_FILENO, output, len);
                    }
                }
            }
        }
    }

    free(buffer);
    return 0;
}
