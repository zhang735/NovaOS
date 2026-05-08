#include <kernel/types.h>
#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/memory.h>
#include <kernel/task.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <drivers/vga.h>
#include <drivers/pit.h>

// 声明外部汇编函数或链接器符号
extern uint32_t end; // 内核结束地址

// ================= 用户态测试程序 (Sandbox) =================
// 模拟一个简单的 Ring 3 任务
void user_task_A() {
    while(1) {
        // 使用系统调用打印信息，证明内核态/用户态隔离成功
        asm volatile("mov $0, %%eax; int $0x80" : : "b"("Hello from Ring 3 Sandbox!\n"));

        // 简单的延迟
        for(volatile int i = 0; i < 1000000; i++);
    }
}

// 辅助函数：由于我们还没有 ELF 加载器，暂时在内核态完成跳转准备
void launch_app_wrapper() {
    vga_print("[TASK] Transitioning to Ring 3...\n");
    switch_to_user_mode(user_task_A);
}

// ================= 内核主入口 =================

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    vga_clear();
    vga_print("Welcome to NovaOS Evolution - Kernel V2 Alpha\n");
    vga_print("---------------------------------------------\n");

    // 1. 基础架构初始化
    gdt_init();
    idt_init();

    // 2. 内存管理体系初始化 (PMM -> VMM -> HEAP)
    pmm_init(mbd);
    vmm_init();

    // 【关键】激活 Nova Allocator V2 (8大改进版)
    kheap_init();

    // ==========================================================
    // 【新特性测试 1】：验证 Zone 隔离 (改进 1 & 6)
    // ==========================================================
    vga_print("[TEST] Testing ZONE_DMA (Locked Continuous Block)...\n");
    void* dma_ptr = kmalloc_zone(4096, ZONE_DMA);
    if (dma_ptr) {
        vga_print("  - DMA allocation at: ");
        vga_print_hex((uint32_t)dma_ptr);
        vga_print(" SUCCESS\n");
    }

    // ==========================================================
    // 【新特性测试 2】：验证批量分配 (改进 8)
    // ==========================================================
    vga_print("[TEST] Testing Batch Allocation (Improved Throughput)...\n");
    void* batch_ptrs[5];
    int count = kmalloc_batch(32, 5, batch_ptrs);
    if (count == 5) {
        vga_print("  - Batch 5x32B allocation SUCCESS\n");
        // 立即批量释放，测试回收逻辑
        kfree_batch(5, batch_ptrs);
        vga_print("  - Batch Release SUCCESS\n");
    }

    // ==========================================================
    // 【新特性测试 3】：验证热块缓存 (改进 3)
    // ==========================================================
    vga_print("[TEST] Testing Hot Cache (Zero-Lock Path)...\n");
    void* p1 = kmalloc(32); // 第一次分配，从链表拿
    kfree(p1);              // 释放，进入 Hot Cache
    void* p2 = kmalloc(32); // 第二次分配，应该直接从 Hot Cache 极速获取
    if (p1 == p2) {
        vga_print("  - Hot Cache Hit (O(1) Path) SUCCESS\n");
    }

    // 3. 硬件中断初始化
    pit_init(100); // 100Hz 时钟中断用于抢占式调度

    // 4. 多任务系统启动
    vga_print("\n[INIT] Initializing Multitasking...\n");

    // 创建第一个用户态进程（沙箱）
    process_t* p_user = create_process(1, true);
    create_thread(p_user, launch_app_wrapper, "UserApp");

    // 【改进 5】：我们可以在这里设置一个低优先级的后台线程专门负责 kheap_compact_lazy()
    // 暂且由时钟中断或主循环触发

    vga_print("[INIT] NovaOS is now under Preemptive Scheduling.\n\n");

    // 5. 移交控制权给调度器
    scheduler_run();

    // 除非发生严重错误，否则不会运行到这里
    while(1) {
        __asm__ volatile("hlt");
    }
}