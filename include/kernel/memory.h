#ifndef NOVA_MEMORY_H
#define NOVA_MEMORY_H

#include <kernel/types.h>
#include <kernel/multiboot.h>

#define PAGE_SIZE       4096
#define KERNEL_VIRT_BASE 0xC0000000

// 分页标志位
#define PG_PRESENT 0x1
#define PG_WRITE   0x2
#define PG_USER    0x4

// 快速索引宏
#define PAGE_DIR_INDEX(addr) (((uint32_t)addr) >> 22)
#define PAGE_TBL_INDEX(addr) ((((uint32_t)addr) >> 12) & 0x3FF)

typedef uint32_t pde_t;
typedef uint32_t pte_t;

// 物理内存
void pmm_init(multiboot_info_t* mbd);
void* pmm_alloc_page();

// 虚拟内存
void vmm_init();
pde_t* vmm_create_process_dir();

#endif