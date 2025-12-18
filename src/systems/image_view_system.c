#include "../include/systems/image_view_system.h"

#ifdef ENABLE_FULL_GRAPHICS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
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
} RawImage;

static struct termios g_prev_termios;
static bool g_raw_mode_active = false;

// --- Helper: Terminal Control ---

static void disable_img_raw_mode() {
    if (!g_raw_mode_active) return;
    printf(MOUSE_DISABLE);
    printf(SHOW_CURSOR);
    printf("\x1b[0m\n"); // Reset colors
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_prev_termios);
    g_raw_mode_active = false;
}

static void enable_img_raw_mode() {
    if (g_raw_mode_active) return;
    tcgetattr(STDIN_FILENO, &g_prev_termios);
    
    struct termios raw = g_prev_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); 
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // 100ms timeout

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    printf(MOUSE_ENABLE);
    printf(HIDE_CURSOR);
    fflush(stdout);
    g_raw_mode_active = true;
}

static void get_term_size(int *w, int *h) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        *w = 80;
        *h = 24;
    } else {
        *w = ws.ws_col;
        *h = ws.ws_row;
    }
}

// --- Helper: Image Logic ---

static int load_raw_image(const char *path, RawImage *img) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    if (fread(&img->width, 4, 1, f) != 1) { fclose(f); return 0; }
    if (fread(&img->height, 4, 1, f) != 1) { fclose(f); return 0; }

    size_t data_size = img->width * img->height * 3;
    img->data = malloc(data_size);
    if (!img->data) { fclose(f); return 0; }

    if (fread(img->data, 1, data_size, f) != data_size) {
        free(img->data);
        fclose(f);
        return 0;
    }

    fclose(f);
    return 1;
}

static void render_image_to_term(const RawImage *img, int term_w, int term_h, float *out_scale, int *out_off_x, int *out_off_y) {
    // Buffer size estimation
    size_t buf_cap = term_w * term_h * 24 + 4096;
    char *out_buf = malloc(buf_cap);
    if (!out_buf) return;
    
    char *ptr = out_buf;
    
    // Clear screen
    ptr += sprintf(ptr, "\x1b[2J\x1b[H");

    int term_px_w = term_w / 2; // 2 chars per pixel width
    int term_px_h = term_h;

    float scale_x = (float)img->width / term_px_w;
    float scale_y = (float)img->height / term_px_h;
    float scale = (scale_x > scale_y) ? scale_x : scale_y;

    // Output scale params for click mapping
    *out_scale = scale;

    int draw_w = (int)(img->width / scale);
    int draw_h = (int)(img->height / scale);
    int offset_x = (term_px_w - draw_w) / 2;
    int offset_y = (term_px_h - draw_h) / 2;
    
    *out_off_x = offset_x;
    *out_off_y = offset_y;

    int current_r = -1, current_g = -1, current_b = -1;

    for (int y = 0; y < term_h; y++) {
        for (int x = 0; x < term_px_w; x++) {
            if (y >= offset_y && y < offset_y + draw_h && 
                x >= offset_x && x < offset_x + draw_w) {

                int img_x = (int)((x - offset_x) * scale);
                int img_y = (int)((y - offset_y) * scale);

                if (img_x >= img->width) img_x = img->width - 1;
                if (img_y >= img->height) img_y = img->height - 1;

                uint8_t *p = &img->data[(img_y * img->width + img_x) * 3];
                uint8_t r = p[0], g = p[1], b = p[2];

                if (r != current_r || g != current_g || b != current_b) {
                    ptr += sprintf(ptr, "\x1b[48;2;%d;%d;%dm", r, g, b);
                    current_r = r; current_g = g; current_b = b;
                }
                *ptr++ = ' '; *ptr++ = ' ';
            } else {
                if (current_r != -1) {
                    ptr += sprintf(ptr, "\x1b[0m");
                    current_r = -1; current_g = -1; current_b = -1;
                }
                *ptr++ = ' '; *ptr++ = ' ';
            }
        }
        ptr += sprintf(ptr, "\x1b[0m\n");
        current_r = -1; current_g = -1; current_b = -1;
    }

    // Overlay Instructions
    ptr += sprintf(ptr, "\x1b[7m [Touch/Click] Interact  [Q] Back \x1b[0m");

    write(STDOUT_FILENO, out_buf, ptr - out_buf);
    free(out_buf);
}

// --- Main Interface ---

void image_view_init() {
    // Can check for file existence or init global resources if needed
}

ImageViewResult show_image_interactive(const char* raw_filepath) {
    ImageViewResult result = {0, 0, false};
    RawImage img;

    if (!load_raw_image(raw_filepath, &img)) {
        fprintf(stderr, "Failed to load image: %s\n", raw_filepath);
        return result;
    }

    enable_img_raw_mode();
    
    int term_w, term_h;
    float scale = 1.0f;
    int off_x = 0, off_y = 0;

    get_term_size(&term_w, &term_h);
    render_image_to_term(&img, term_w, term_h, &scale, &off_x, &off_y);

    char ibuf[64];
    while (true) {
        int n = read(STDIN_FILENO, ibuf, sizeof(ibuf) - 1);
        if (n > 0) {
            ibuf[n] = '\0';
            
            // Quit conditions
            if (ibuf[0] == 'q' || ibuf[0] == 'Q' || ibuf[0] == 3) {
                result.quit = true;
                break;
            }

            // Mouse Event
            if (ibuf[0] == '\x1b' && ibuf[1] == '[' && ibuf[2] == '<') {
                int btn, tx, ty;
                char type;
                if (sscanf(ibuf + 3, "%d;%d;%d%c", &btn, &tx, &ty, &type) == 4) {
                    if (type == 'M') { // Press
                        // Note: Term coords are 1-based usually, scan lines map to Y
                        // Our render logic used 0-based loops.
                        // Terminal reports X (col), Y (row).
                        
                        // Convert Term X to "Pixel X" (2 chars per pixel)
                        // tx is 1-based from terminal
                        int logical_x = (tx - 1) / 2;
                        int logical_y = (ty - 1); // If rendering starts at top

                        // Check bounds relative to image offset
                        if (logical_x >= off_x && logical_y >= off_y) {
                            int hit_x = (int)((logical_x - off_x) * scale);
                            int hit_y = (int)((logical_y - off_y) * scale);
                            
                            if (hit_x < img.width && hit_y < img.height) {
                                result.x = hit_x;
                                result.y = hit_y;
                                result.quit = false;
                                break; // Return the click!
                            }
                        }
                    }
                }
            }
            
            // TODO: Handle Resize Signal properly (requires global handler hook)
        }
        // Minimal sleep to yield
        usleep(10000);
    }

    disable_img_raw_mode();
    if (img.data) free(img.data);
    
    // Clear screen on exit to restore clean state for main game
    printf("\x1b[2J\x1b[H"); 
    fflush(stdout);

    return result;
}

#endif // ENABLE_FULL_GRAPHICS
