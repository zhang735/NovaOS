#include <stdint.h>
#include <drivers/vga.h>

// 中断压栈的寄存器结构
struct registers {
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // pusha 压入的
    uint32_t eip, cs, eflags, useresp, ss;           // CPU 自动压入的
};

void syscall_handler(struct registers* regs) {
    // EAX 存放系统调用号，EBX, ECX, EDX 存放参数
    switch (regs->eax) {
        case 1: // sys_print (测试系统调用)
            vga_print_color((char*)regs->ebx, VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            break;
        case 2: // sys_exit
            vga_print_color("\n[Syscall] Process exited.\n", VGA_COLOR_RED, VGA_COLOR_BLACK);
            while(1) __asm__ volatile("hlt");
            break;
        default:
            vga_print_color("Unknown Syscall!\n", VGA_COLOR_RED, VGA_COLOR_BLACK);
            break;
    }
}