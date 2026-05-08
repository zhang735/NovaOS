#include <drivers/vga.h>

static uint16_t* const VGA_BUFFER = (uint16_t*)0xB8000;
static int cursor_x = 0;
static int cursor_y = 0;

void vga_clear() {
    for (int i = 0; i < 80 * 25; i++) {
        VGA_BUFFER[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    }
    cursor_x = 0;
    cursor_y = 0;
}

void vga_init() {
    vga_clear();
}

void vga_print_color(const char* str, vga_color_t fg, vga_color_t bg) {
    uint8_t color = (uint8_t)fg | (uint8_t)bg << 4;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            const int index = cursor_y * 80 + cursor_x;
            VGA_BUFFER[index] = (uint16_t)str[i] | (uint16_t)color << 8;
            cursor_x++;
        }
        // 简单的滚屏处理
        if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }
    }
}

// 兼容旧的打印函数
void vga_print(const char* str) {
    vga_print_color(str, VGA_COLOR_WHITE, VGA_COLOR_BLACK);
}
// 【新增核心】单字符处理引擎
void vga_putchar(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') { // 处理退格键
        if (cursor_x > 0) cursor_x--;
        else if (cursor_y > 0) { cursor_y--; cursor_x = 79; }
        // 视觉擦除：在回退的位置写一个黑色的空格
        VGA_BUFFER[cursor_y * 80 + cursor_x] = (uint16_t)' ' | (uint16_t)0x0F << 8;
    } else {
        VGA_BUFFER[cursor_y * 80 + cursor_x] = (uint16_t)c | (uint16_t)0x0F << 8;
        cursor_x++;
    }

    if (cursor_x >= 80) { cursor_x = 0; cursor_y++; }

    // 滚屏逻辑：如果写到底部了，把所有行往上移一行
    if (cursor_y >= 25) {
        for (int i = 0; i < 80 * 24; i++) VGA_BUFFER[i] = VGA_BUFFER[i + 80];
        for (int i = 80 * 24; i < 80 * 25; i++) VGA_BUFFER[i] = (uint16_t)' ' | (uint16_t)0x0F << 8;
        cursor_y = 24;
    }
}
