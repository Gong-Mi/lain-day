#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>

void print_env(const char* name) {
    const char* val = getenv(name);
    printf("%-15s : %s\n", name, val ? val : "(null)");
}

struct termios orig_termios;

void disable_raw_mode() {
    printf("\x1b[?1000l\x1b[?1006l"); // Disable mouse tracking
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    printf("\nTerminal restored.\n");
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    // Enable Mouse Tracking (Normal + SGR)
    printf("\x1b[?1000h\x1b[?1006h");
    fflush(stdout);
}

int main() {
    printf("=== Lain-Day Terminal Probe ===\n");
    print_env("TERM");
    print_env("COLORTERM");
    print_env("TERM_PROGRAM");
    print_env("SHELL");
    printf("%-15s : %s\n", "isatty", isatty(STDIN_FILENO) ? "Yes" : "No");
    
    printf("\nInstructions:\n");
    printf("1. I will now enable mouse tracking.\n");
    printf("2. Try CLICKING or TAPPING the screen.\n");
    printf("3. You should see hexadecimal and raw strings appearing.\n");
    printf("4. Press 'q' to quit.\n\n");
    
    printf("Press ANY KEY to start probing...");
    fflush(stdout);
    getchar();

    enable_raw_mode();

    char c;
    printf("\r\x1b[K--- PROBING ACTIVE (Press 'q' to quit) ---\n");
    
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (c == '\x1b') {
            printf("\nESC sequence: \\x1b");
        } else if (c < 32 || c > 126) {
            printf("[%02x]", (unsigned char)c);
        } else {
            printf("%c", c);
        }
        fflush(stdout);
    }

    return 0;
}
