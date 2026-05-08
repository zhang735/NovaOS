#include <kernel/memory.h>
#include <drivers/vga.h>

__attribute__((aligned(4096))) static uint32_t kernel_pdir[1024];
__attribute__((aligned(4096))) static uint32_t kernel_ptable[4][1024];

static uint32_t* bitmap = (uint32_t*)0x200000;
static uint32_t total_pages = 0;

extern void load_page_directory(uint32_t addr);
extern void enable_paging();

#define SET_BIT(i) (bitmap[i / 32] |= (1 << (i % 32)))
#define GET_BIT(i) (bitmap[i / 32] & (1 << (i % 32)))

void pmm_init(multiboot_info_t* mbd) {
    uint32_t mem_kb = mbd->mem_lower + mbd->mem_upper;
    total_pages = (mem_kb * 1024) / 4096;

    for(uint32_t i = 0; i < 1024; i++) bitmap[i] = 0;

    // 标记前 4MB 为已占用（内核空间）
    for(uint32_t i = 0; i < 1024; i++) {
        SET_BIT(i);
    }
    vga_print("[PMM] Physical Memory Initialized.\n");
}

void* pmm_alloc_page() {
    for (uint32_t i = 1024; i < 4096; i++) {
        if (!GET_BIT(i)) {
            SET_BIT(i);
            return (void*)(i * 4096);
        }
    }
    vga_print_color("OOM PANIC!", 15, 4);
    while(1);
    return 0;
}

void vmm_init() {
    // 设置页目录，2 代表 Present=0, RW=1, US=0 (内核)
    for(int i = 0; i < 1024; i++) kernel_pdir[i] = 2;

    // 映射前 16MB 内存 (4 个页表)
    for(int t = 0; t < 4; t++) {
        for(int i = 0; i < 1024; i++) {
            // 【关键修改】：这里必须是 | 7 (User + Read/Write + Present)
            // 否则 Ring 3 用户态一旦访问 0x100000 之后的代码或 0x800000 的栈就会触发 PAGE FAULT
            kernel_ptable[t][i] = ((t * 1024 + i) * 4096) | 7;
        }
        // 页目录项也要同步开启 User 权限 (| 7)
        kernel_pdir[t] = ((uint32_t)kernel_ptable[t]) | 7;
        kernel_pdir[768 + t] = ((uint32_t)kernel_ptable[t]) | 7;
    }

    load_page_directory((uint32_t)kernel_pdir);
    enable_paging();
    vga_print("[VMM] Paging Enabled.\n");
}

uint32_t* vmm_create_process_dir() {
    uint32_t* new_dir = (uint32_t*)pmm_alloc_page();
    for(int i = 0; i < 1024; i++) {
        new_dir[i] = kernel_pdir[i];
    }
    return new_dir;
}