#include <kernel/idt.h>
#include <kernel/io.h>
#include <drivers/vga.h>

struct idt_entry idt[256];
struct idt_ptr idtp;

extern void irq0();   // 时钟
extern void irq1();   // 键盘
extern void isr14();  // 缺页异常
extern void isr128(); // 系统调用

// 辅助函数：将数字转为十六进制字符串
void itoa_hex(uint32_t num, char* buf) {
    buf[0] = '0'; buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 7; i >= 0; i--) {
        buf[2 + i] = hex_chars[num & 0xF];
        num >>= 4;
    }
    buf[10] = '\0';
}

void page_fault_handler() {
    uint32_t cr2;
    // CR2 寄存器保存了引发缺页异常的内存地址！
    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));

    char cr2_str[12];
    itoa_hex(cr2, cr2_str);

    vga_print_color("\n[FATAL] PAGE FAULT at ", VGA_COLOR_WHITE, VGA_COLOR_RED);
    // 【修改点】：将 VGA_COLOR_YELLOW 替换为 VGA_COLOR_LIGHT_BROWN，这是 VGA 16 色模式中的标准黄色
    vga_print_color(cr2_str, VGA_COLOR_LIGHT_BROWN, VGA_COLOR_RED);
    vga_print_color(" ! SYSTEM HALTED.\n", VGA_COLOR_WHITE, VGA_COLOR_RED);

    while(1) __asm__ volatile("hlt");
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void pic_init() {
    outb(0x20, 0x11); outb(0xA0, 0x11);
    outb(0x21, 0x20); outb(0xA1, 0x28);
    outb(0x21, 0x04); outb(0xA1, 0x02);
    outb(0x21, 0x01); outb(0xA1, 0x01);
    outb(0x21, 0x0);  outb(0xA1, 0x0);
}

void idt_init() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;
    for(int i=0; i<256; i++) idt_set_gate(i, 0, 0, 0);

    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E); // 捕获缺页
    idt_set_gate(32, (uint32_t)irq0, 0x08, 0x8E);  // 时钟中断
    idt_set_gate(33, (uint32_t)irq1, 0x08, 0x8E);  // 键盘中断
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE); // 系统调用 int 0x80 (DPL=3)

    __asm__ volatile("lidt %0" : : "m"(idtp));
}