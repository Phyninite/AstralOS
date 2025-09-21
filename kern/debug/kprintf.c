#include "kprintf.h"
#include "font.h"
#include "lib.h"

#define KPRINTF_BUFFER_SIZE 256

static uint32_t* kprintf_framebuffer = 0;
static uint32_t kprintf_fb_width = 0;
static uint32_t kprintf_fb_height = 0;
static uint32_t kprintf_fb_pitch = 0;

static uint32_t cursor_x = 0;
static uint32_t cursor_y = 0;

static void kprintf_draw_char(uint32_t x, uint32_t y, char c, uint32_t color) {
    if (kprintf_framebuffer == 0) {
        uart_putc(c);
        return;
    }

    if (c < 32 || c > 126) return;
    
    const uint8_t* glyph = proggy_clean[c - 32];
    
    for (int row = 0; row < FONT_HEIGHT; row++) {
        for (int col = 0; col < FONT_WIDTH; col++) {
            if (glyph[row] & (0x80 >> col)) {
                uint32_t px = x + col;
                uint32_t py = y + row;
                if (px < kprintf_fb_width && py < kprintf_fb_height) {
                    kprintf_framebuffer[(py * (kprintf_fb_pitch / 4)) + px] = color;
                }
            }
        }
    }
}

static void kprintf_scroll() {
    if (kprintf_framebuffer == 0) return;

    uint32_t line_size = kprintf_fb_width * 4;
    uint32_t scroll_amount = FONT_HEIGHT * (kprintf_fb_pitch / 4);

    memcpy(kprintf_framebuffer, kprintf_framebuffer + scroll_amount, (kprintf_fb_height - FONT_HEIGHT) * (kprintf_fb_pitch / 4) * 4);

    memset(kprintf_framebuffer + ((kprintf_fb_height - FONT_HEIGHT) * (kprintf_fb_pitch / 4)), 0, FONT_HEIGHT * (kprintf_fb_pitch / 4) * 4);
}

void kprintf_init(uint32_t* fb, uint32_t width, uint32_t height, uint32_t pitch) {
    kprintf_framebuffer = fb;
    kprintf_fb_width = width;
    kprintf_fb_height = height;
    kprintf_fb_pitch = pitch;
    cursor_x = 0;
    cursor_y = 0;
}

void kprintf_varg(const char* fmt, va_list args) {
    char buffer[KPRINTF_BUFFER_SIZE];
    vsnprintf(buffer, KPRINTF_BUFFER_SIZE, fmt, args);

    for (int i = 0; buffer[i] != '\0'; i++) {
        char c = buffer[i];
        if (kprintf_framebuffer == 0) {
            uart_putc(c);
            if (c == '\n') {
                uart_putc('\r');
            }
        } else {
            if (c == '\n') {
                cursor_x = 0;
                cursor_y += FONT_HEIGHT;
            } else {
                kprintf_draw_char(cursor_x, cursor_y, c, 0xFFFFFFFF);
                cursor_x += FONT_WIDTH;
            }

            if (cursor_x + FONT_WIDTH > kprintf_fb_width) {
                cursor_x = 0;
                cursor_y += FONT_HEIGHT;
            }

            if (cursor_y + FONT_HEIGHT > kprintf_fb_height) {
                kprintf_scroll();
                cursor_y -= FONT_HEIGHT;
            }
        }
    }
}

void kprintf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    kprintf_varg(fmt, args);
    va_end(args);
}


