#include <kernel/multiboot.h>
#include <kernel/memory.h>
#include <kernel/scheduler.h>
#include <kernel/idt.h>
#include <kernel/io.h>
#include <drivers/vga.h>
#include <stdint.h>
#include <kernel/gdt.h>

extern void pit_init(uint32_t hz);
extern void enter_user_mode(uint32_t entry_point, uint32_t user_stack);

void request_burst_copy(uint32_t size) {
    vga_print_color("\n[INFO] Burst copy disabled in Ring 3 test.\n", VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
}

// ============ Ring 3 应用程序 ============
void user_sys_print(const char* msg) {
    // 【修改】强制指定 eax=1 (系统调用号), ebx=msg (字符串参数)
    // 避免 GCC 随机分配寄存器导致指针丢失
    __asm__ volatile(
        "int $0x80"
        :
        : "a"(1), "b"(msg)
    );
}

void user_task_A() {
    while (1) {
        user_sys_print("Hello from Ring 3 Sandbox!\n");
        for(volatile int i=0; i<10000000; i++);
    }
}

// 内核辅助函数
void launch_app_wrapper() {
    enter_user_mode((uint32_t)user_task_A, 0x800000);
}
// =========================================

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    vga_init();
    vga_print("Booting NovaOS...\n");

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        vga_print_color("Invalid Magic Number!\n", VGA_COLOR_RED, VGA_COLOR_BLACK);
        return;
    }

    gdt_init();
    pmm_init(mbd);
    vmm_init();
    pic_init();
    idt_init();
    pit_init(100);

    vga_print("Entering Ring 3 User Mode Sandbox...\n");

    process_t* p1 = create_process(1, true);
    create_thread(p1, launch_app_wrapper, "AppA");

    scheduler_run();
}