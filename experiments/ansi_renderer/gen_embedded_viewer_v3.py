import sys
from PIL import Image

def generate_c_viewer(image_path, output_c_path, target_width=200):
    try:
        img = Image.open(image_path).convert("RGB")
    except Exception as e:
        print(f"Error opening image: {e}")
        sys.exit(1)

    # Resize logic
    w, h = img.size
    ratio = h / w
    new_w = target_width
    new_h = int(new_w * ratio)
    
    # Resize with high quality
    img = img.resize((new_w, new_h), Image.Resampling.LANCZOS)
    data = list(img.getdata())

    # Generate C Array Content
    c_array_parts = []
    for i, pixel in enumerate(data):
        c_array_parts.append(f"0x{pixel[0]:02X},0x{pixel[1]:02X},0x{pixel[2]:02X}")
    
    c_array_str = ",".join(c_array_parts)

    # C Source Template (Using simple replacement to avoid f-string syntax hell)
    c_template = r"""
/**
 * lain_view.c
 * Standalone Embedded Image Viewer
 */

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
#define CLEAR_SCREEN  "\x1b[2J\x1b[H"
#define RESET_COLOR   "\x1b[0m"

// --- Embedded Image Data ---
#define IMG_W __WIDTH__
#define IMG_H __HEIGHT__

static const uint8_t IMG_DATA[] = { __DATA__ };

// --- Globals ---
struct termios g_orig_termios;
volatile int g_need_resize = 1;
int g_term_w = 0;
int g_term_h = 0;

// --- Terminal Control ---

void disable_raw_mode() {
    printf("%s", MOUSE_DISABLE);
    printf("%s", SHOW_CURSOR);
    printf("%s\n", RESET_COLOR);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf("%s", MOUSE_ENABLE);
    printf("%s", HIDE_CURSOR);
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

// --- Render Logic ---

void render() {
    if (g_term_w <= 0 || g_term_h <= 0) return;

    // Buffer to hold the entire frame string
    size_t buf_cap = g_term_w * g_term_h * 32 + 4096;
    char *out_buf = malloc(buf_cap);
    if (!out_buf) return;
    
    char *ptr = out_buf;
    
    // Clear screen
    ptr += sprintf(ptr, "%s", CLEAR_SCREEN);

    int term_px_w = g_term_w / 2; // 2 chars per pixel
    int term_px_h = g_term_h;

    // Calculate Scale
    float scale_x = (float)IMG_W / term_px_w;
    float scale_y = (float)IMG_H / term_px_h;
    float scale = (scale_x > scale_y) ? scale_x : scale_y;
    
    // Don't upscale if image is smaller than screen
    // if (scale < 1.0f) scale = 1.0f; 

    // Calculate drawn dimensions
    int draw_w = (int)(IMG_W / scale);
    int draw_h = (int)(IMG_H / scale);
    
    // Clamp
    if (draw_w > term_px_w) draw_w = term_px_w;
    if (draw_h > term_px_h) draw_h = term_px_h;

    // Center offsets
    int offset_x = (term_px_w - draw_w) / 2;
    int offset_y = (term_px_h - draw_h) / 2;

    int current_r = -1, current_g = -1, current_b = -1;

    for (int y = 0; y < g_term_h; y++) {
        // Optimization: move cursor only if needed, but sequential write is fast enough with buffer
        for (int x = 0; x < term_px_w; x++) {
            if (y >= offset_y && y < offset_y + draw_h && 
                x >= offset_x && x < offset_x + draw_w) {

                // Map screen -> image
                int img_x = (int)((x - offset_x) * scale);
                int img_y = (int)((y - offset_y) * scale);

                if (img_x < 0) img_x = 0;
                if (img_x >= IMG_W) img_x = IMG_W - 1;
                if (img_y < 0) img_y = 0;
                if (img_y >= IMG_H) img_y = IMG_H - 1;

                const uint8_t *p = &IMG_DATA[(img_y * IMG_W + img_x) * 3];
                uint8_t r = p[0];
                uint8_t g = p[1];
                uint8_t b = p[2];

                // ANSI Color Optimization
                if (r != current_r || g != current_g || b != current_b) {
                    ptr += sprintf(ptr, "\x1b[48;2;%d;%d;%dm", r, g, b);
                    current_r = r; current_g = g; current_b = b;
                }
                *ptr++ = ' '; *ptr++ = ' ';

            } else {
                // Background (Black/Default)
                if (current_r != -1) {
                    ptr += sprintf(ptr, "%s", RESET_COLOR);
                    current_r = -1; current_g = -1; current_b = -1;
                }
                *ptr++ = ' '; *ptr++ = ' ';
            }
        }
        // Newline at end of row
        ptr += sprintf(ptr, "%s\n", RESET_COLOR);
        current_r = -1; current_g = -1; current_b = -1;
    }
    
    // Status Bar
    ptr += sprintf(ptr, "\x1b[7m [Lain] %dx%d -> %dx%d (Scale: %.2f) | Q: Quit \x1b[0m", 
                   IMG_W, IMG_H, draw_w, draw_h, scale);

    write(STDOUT_FILENO, out_buf, ptr - out_buf);
    free(out_buf);
}

int main() {
    enable_raw_mode();
    signal(SIGWINCH, sigwinch_handler);
    update_terminal_size();
    render();

    char ibuf[64];
    while (1) {
        if (g_need_resize) {
            g_need_resize = 0;
            update_terminal_size();
            render();
        }

        int n = read(STDIN_FILENO, ibuf, sizeof(ibuf) - 1);
        if (n > 0) {
            ibuf[n] = '\0';
            // Q or Ctrl-C
            if (ibuf[0] == 'q' || ibuf[0] == 'Q' || ibuf[0] == 3) break;

            // Mouse events
            if (ibuf[0] == '\x1b' && ibuf[1] == '[' && ibuf[2] == '<') {
                int btn, tx, ty;
                char type;
                if (sscanf(ibuf + 3, "%d;%d;%d%c", &btn, &tx, &ty, &type) == 4) {
                    if (type == 'M') {
                        // Just verify it works
                        char msg[64];
                        int len = snprintf(msg, sizeof(msg), "\033[1;1H\033[41m CLICK: %d,%d \033[0m", tx, ty);
                        write(STDOUT_FILENO, msg, len);
                    }
                }
            }
        }
        usleep(10000);
    }
    return 0;
}
"
    
    # Fill in the template
    c_source = c_template.replace("__WIDTH__", str(new_w)) \
                         .replace("__HEIGHT__", str(new_h)) \
                         .replace("__DATA__", c_array_str)

    with open(output_c_path, 'w') as f:
        f.write(c_source)
    
    print(f"Generated {output_c_path} with {new_w}x{new_h} embedded image.")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python gen.py <img_path> <out_c_path>")
        sys.exit(1)
    
    generate_c_viewer(sys.argv[1], sys.argv[2])
