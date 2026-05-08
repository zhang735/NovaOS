#ifndef DICTATOR_VGA_H
#define DICTATOR_VGA_H

#include <kernel/types.h>

// VGA 颜色常量定义
typedef enum {
    VGA_COLOR_BLACK = 0,
    VGA_COLOR_BLUE = 1,
    VGA_COLOR_GREEN = 2,
    VGA_COLOR_CYAN = 3,
    VGA_COLOR_RED = 4,
    VGA_COLOR_MAGENTA = 5,
    VGA_COLOR_BROWN = 6,
    VGA_COLOR_LIGHT_GREY = 7,
    VGA_COLOR_DARK_GREY = 8,
    VGA_COLOR_LIGHT_BLUE = 9,
    VGA_COLOR_LIGHT_GREEN = 10,
    VGA_COLOR_LIGHT_CYAN = 11,
    VGA_COLOR_LIGHT_RED = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN = 14,
    VGA_COLOR_WHITE = 15,
} vga_color_t;

// 初始化屏幕
void vga_init(void);

// 基础打印函数
void vga_print(const char* str);

// 带颜色的打印（为你以后的“蓝屏死机”或“警告信息”准备）
void vga_print_color(const char* str, vga_color_t fg, vga_color_t bg);

// 清屏
void vga_clear(void);
// 【新增】单字符输出，支持退格删除！
void vga_putchar(char c);
#endif