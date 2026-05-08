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

// ================= Nova Allocator V2 究极进化版 =================

// 【改进6】内存分区 (Zones)
typedef enum {
    ZONE_NORMAL    = 0, // 普通可切分池
    ZONE_DMA       = 1, // DMA/大页预留隔离池 (不切碎)
    ZONE_EMERGENCY = 2, // 应急备用池
    MAX_ZONES      = 3
} mem_zone_t;

// 【改进4】块染色/归属标记 (Flags)
#define MEM_FLAG_FREE       (1 << 0) // 是否空闲
#define MEM_FLAG_LOCKED     (1 << 1) // 硬件锁死(不可合并/切分)
#define MEM_FLAG_LAZY       (1 << 2) // 处于惰性释放队列
#define MEM_FLAG_DMA_ONLY   (1 << 3) // 仅限 DMA 使用

// 内存分配单元（带染色和水位标记）
typedef struct mem_descriptor {
    uint8_t  is_valid;      // 是否为有效的盘子
    uint8_t  zone_id;       // 【改进6】所属分区
    uint16_t flags;         // 【改进4】状态染色标签
    uint32_t start_addr;
    uint32_t size;

    struct mem_descriptor* prev;
    struct mem_descriptor* next;
} mem_desc_t;

// 【改进3】Per-CPU 私有链 (冷热分离热块缓存)
#define HOT_CACHE_SIZE 32   // 常规 32 字节小块
#define HOT_CACHE_MAX  64   // 缓存池最大容量
typedef struct {
    void* blocks[HOT_CACHE_MAX];
    int   count;
} hot_cache_t;

// ================= 分配器 API =================
void kheap_init();
void* kmalloc(uint32_t size);
void* kmalloc_zone(uint32_t size, mem_zone_t zone); // 指定区域分配
void kfree(void* ptr);

// 【改进8】批量分配 / 批量释放
int kmalloc_batch(uint32_t size, int count, void** out_array);
void kfree_batch(int count, void** ptr_array);

// 【改进5】主动触发规整 (后台时钟中断可调用)
void kheap_compact_lazy();

// 【改进7】动态调整最小切分粒度
void kheap_set_min_granularity(uint32_t bytes);

#endif