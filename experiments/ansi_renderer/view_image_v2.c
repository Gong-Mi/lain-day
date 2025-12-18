#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <termios.h>

// --- Definitions ---
#define MOUSE_ENABLE  "\x1b[?1000h\x1b[?1006h"
#define MOUSE_DISABLE "\x1b[?1000l\x1b[?1006l"
#define HIDE_CURSOR   "\x1b[?25l"
#define SHOW_CURSOR   "\x1b[?25h"

typedef struct {
    uint32_t width;
    uint32_t height;
    uint8_t *data; // RGBRGB...
} Image;

// --- Globals ---
Image g_img;
struct termios g_orig_termios;
volatile int g_need_resize = 1;
int g_term_w = 0;
int g_term_h = 0;

// --- Terminal Control ---

void disable_raw_mode() {
    printf(MOUSE_DISABLE);
    printf(SHOW_CURSOR);
    printf("\x1b[0m\n"); // Reset colors
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); // ISIG: Disable Ctrl-C/Z processing
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(MOUSE_ENABLE);
    printf(HIDE_CURSOR);
    fflush(stdout);
}

void update_terminal_size() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        g_term_w = 80;
        g_term_h = 24;
    } else {
        g_term_w = ws.ws_col;
        g_term_h = ws.ws_row;
    }
}

void sigwinch_handler(int sig) {
    (void)sig;
    g_need_resize = 1;
}

// --- Image Logic ---

int load_image(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (fread(&g_img.width, 4, 1, f) != 1) return 0;
    if (fread(&g_img.height, 4, 1, f) != 1) return 0;

    size_t data_size = g_img.width * g_img.height * 3;
    g_img.data = malloc(data_size);
    if (!g_img.data) return 0;

    if (fread(g_img.data, 1, data_size, f) != data_size) {
        free(g_img.data);
        return 0;
    }

    fclose(f);
    return 1;
}

// Draw the image scaled to fit the terminal
void render() {
    if (g_term_w <= 0 || g_term_h <= 0) return;

    // Use a large buffer to minimize 'write' syscalls (prevents flickering)
    size_t buf_cap = g_term_w * g_term_h * 24 + 4096;
    char *out_buf = malloc(buf_cap);
    if (!out_buf) return;
    
    char *ptr = out_buf;
    
    // Clear screen and go home
    ptr += sprintf(ptr, "\x1b[2J\x1b[H");

    // Effective Terminal Canvas Size in "Pixels" (1 pixel = 2 chars width)
    int term_px_w = g_term_w / 2; 
    int term_px_h = g_term_h;

    // Calculate scaling factors to FIT image (Preserve Aspect Ratio)
    float scale_x = (float)g_img.width / term_px_w;
    float scale_y = (float)g_img.height / term_px_h;
    float scale = (scale_x > scale_y) ? scale_x : scale_y;

    // Optional: Avoid upscaling small images to prevent pixelation? 
    // For now, let it fill the screen as much as possible.

    // Calculate drawn dimensions in terminal "pixels"
    int draw_w = (int)(g_img.width / scale);
    int draw_h = (int)(g_img.height / scale);

    // Center the image
    int offset_x = (term_px_w - draw_w) / 2;
    int offset_y = (term_px_h - draw_h) / 2;

    // Track current color to minimize ANSI codes
    int current_r = -1, current_g = -1, current_b = -1;

    for (int y = 0; y < g_term_h; y++) {
        // Move cursor to start of line (implicit in simple output, but good for safety)
        // Actually, just writing sequentially is faster and simpler for full redraws.

        for (int x = 0; x < term_px_w; x++) {
            // Check if we are inside the image bounds
            if (y >= offset_y && y < offset_y + draw_h && 
                x >= offset_x && x < offset_x + draw_w) {

                // Map Screen Coordinate -> Image Coordinate (Nearest Neighbor)
                int img_x = (int)((x - offset_x) * scale);
                int img_y = (int)((y - offset_y) * scale);

                // Clamp coordinates to be safe
                if (img_x < 0) img_x = 0;
                if (img_x >= g_img.width) img_x = g_img.width - 1;
                if (img_y < 0) img_y = 0;
                if (img_y >= g_img.height) img_y = g_img.height - 1;

                // Get Pixel Color
                uint8_t *p = &g_img.data[(img_y * g_img.width + img_x) * 3];
                uint8_t r = p[0], g = p[1], b = p[2];

                // Update ANSI Color if changed
                if (r != current_r || g != current_g || b != current_b) {
                    ptr += sprintf(ptr, "\x1b[48;2;%d;%d;%dm", r, g, b);
                    current_r = r;
                    current_g = g;
                    current_b = b;
                }
                
                // Draw Pixel (2 spaces)
                *ptr++ = ' ';
                *ptr++ = ' ';

            } else {
                // Outside Image Area (Letterboxing)
                if (current_r != -1) {
                    ptr += sprintf(ptr, "\x1b[0m"); // Reset to default background
                    current_r = -1; current_g = -1; current_b = -1;
                }
                *ptr++ = ' ';
                *ptr++ = ' ';
            }
        }
        
        // End of line: Reset color for safety and newline
        ptr += sprintf(ptr, "\x1b[0m\n");
        current_r = -1; current_g = -1; current_b = -1;
    }

    // Status Bar (Debug Info)
    ptr += sprintf(ptr, "\x1b[7m [Q] Quit  Term: %dx%d  Img: %dx%d  Scale: %.2f  Offset: %d,%d \x1b[0m", 
                   g_term_w, g_term_h, g_img.width, g_img.height, scale, offset_x, offset_y);

    write(STDOUT_FILENO, out_buf, ptr - out_buf);
    free(out_buf);
}

// --- Main ---

int main(int argc, char *argv[]) {
    if (!load_image("image.raw")) {
        fprintf(stderr, "Failed to load image.raw. Run python script first.\n");
        return 1;
    }

    enable_raw_mode();
    signal(SIGWINCH, sigwinch_handler);
    update_terminal_size();
    render();

    char ibuf[64];
    while (1) {
        // Check for resize
        if (g_need_resize) {
            g_need_resize = 0;
            update_terminal_size();
            render();
        }

        int n = read(STDIN_FILENO, ibuf, sizeof(ibuf) - 1);
        if (n > 0) {
            ibuf[n] = '\0';
            if (ibuf[0] == 'q' || ibuf[0] == 'Q' || ibuf[0] == 3) break; // Q or Ctrl-C

            // Mouse Event Parsing
            if (ibuf[0] == '\x1b' && ibuf[1] == '[' && ibuf[2] == '<') {
                int btn, tx, ty;
                char type;
                if (sscanf(ibuf + 3, "%d;%d;%d%c", &btn, &tx, &ty, &type) == 4) {
                    if (type == 'M') { // Press
                        // Calculate Image Coordinates
                        // Logic must match render() scaling!
                        // This is tricky because we need the scale factor again.
                        // Ideally, we'd store render params in global state.
                        // For this demo, just printing Term coords is enough proof.
                        
                        char msg[64];
                        int len = snprintf(msg, sizeof(msg), "\033[1;1H\033[41m CLICK: %d,%d \033[0m", tx, ty);
                        write(STDOUT_FILENO, msg, len);
                    }
                }
            }
        }
        usleep(10000); // Prevent 100% CPU usage loop
    }

    return 0;
}
