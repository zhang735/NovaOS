#include <kernel/io.h>
#include <drivers/vga.h>

// 声明我们要调用的外部函数（在 main.c 中定义）
extern void request_burst_copy(uint32_t size);

// 1. 硬件扫描码 -> ASCII 字符映射表 (美式 QWERTY 键盘)
const char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0,
   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
  '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// 2. 终端输入缓冲区
static char cmd_buffer[256];
static int cmd_index = 0;

// 3. 自研字符串比较函数 (替代标准库的 strcmp)
int kstrcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 4. 键盘中断主处理
void keyboard_handler() {
    uint8_t scancode = inb(0x60); // 读取硬件键盘的电信号

    if (!(scancode & 0x80)) { // 如果是“按下”事件
        char ascii = kbd_us[scancode]; // 查表翻译成字符

        if (ascii) {
            if (ascii == '\n') { // 按下了回车键
                cmd_buffer[cmd_index] = '\0'; // 封口字符串
                vga_print("\n");

                // 解析指令！
                if (kstrcmp(cmd_buffer, "reboot") == 0) {
                    vga_print_color("[SYSTEM] Rebooting...\n", VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    sys_reboot();
                } else if (kstrcmp(cmd_buffer, "poweroff") == 0) {
                    vga_print_color("[SYSTEM] Shutting down...\n", VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
                    sys_shutdown();
                } else if (kstrcmp(cmd_buffer, "copy") == 0) {
                    request_burst_copy(30); // 触发我们的秒传并发测试
                } else if (cmd_index > 0) {
                    vga_print("Command not found: ");
                    vga_print(cmd_buffer);
                    vga_print("\n");
                }

                // 清空缓冲区，打出下一行的提示符
                cmd_index = 0;
                vga_print("NovaOS> ");

            } else if (ascii == '\b') { // 退格键
                if (cmd_index > 0) {
                    cmd_index--;
                    vga_putchar('\b'); // 在屏幕上擦除
                }
            } else { // 普通输入
                if (cmd_index < 255) {
                    cmd_buffer[cmd_index++] = ascii;
                    vga_putchar(ascii); // 显示在屏幕上
                }
            }
        }
    }
}