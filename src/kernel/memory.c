#include <kernel/memory.h>
#include <drivers/vga.h>

// ================= 原有 PMM 与 VMM 核心代码 =================

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
            // 必须是 | 7 (User + Read/Write + Present)
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


// ================= Nova Allocator V2 实现 =================

#define MAX_MEM_DESCRIPTORS 1024
static mem_desc_t mem_descriptors[MAX_MEM_DESCRIPTORS];
static mem_desc_t* root_desc = 0;

// 【改进3】热缓存 (单核暂时用全局代替 Per-CPU)
static hot_cache_t cpu_hot_cache;

// 【改进7】粒度分层，动态可调
static uint32_t current_min_granularity = 16;

// 【改进2 & 5】水位线与惰性合并控制
#define LAZY_FREE_HIGH_WATERMARK 20
static int lazy_free_count = 0;

// 工具：获取空闲盘子
static mem_desc_t* get_unused_descriptor() {
    for (int i = 0; i < MAX_MEM_DESCRIPTORS; i++) {
        if (!mem_descriptors[i].is_valid) return &mem_descriptors[i];
    }
    return 0;
}

// 初始化高级堆
void kheap_init() {
    for (int i = 0; i < MAX_MEM_DESCRIPTORS; i++) mem_descriptors[i].is_valid = 0;
    cpu_hot_cache.count = 0;

    // 申请大块内存 (128 页 = 512KB 作为基础堆池)
    void* heap_start = pmm_alloc_page();
    for(int i = 0; i < 127; i++) pmm_alloc_page();

    // 【改进6】预留隔离分区：初始化时直接划分
    // 假设 512KB 分为：300KB NORMAL, 150KB DMA, 62KB EMERGENCY

    // 1. NORMAL Zone
    mem_desc_t* desc_normal = &mem_descriptors[0];
    desc_normal->is_valid = 1;
    desc_normal->zone_id = ZONE_NORMAL;
    desc_normal->flags = MEM_FLAG_FREE;
    desc_normal->start_addr = (uint32_t)heap_start;
    desc_normal->size = 300 * 1024;
    desc_normal->prev = 0;

    // 2. DMA Zone (连续大块保护)
    mem_desc_t* desc_dma = &mem_descriptors[1];
    desc_dma->is_valid = 1;
    desc_dma->zone_id = ZONE_DMA;
    desc_dma->flags = MEM_FLAG_FREE | MEM_FLAG_DMA_ONLY | MEM_FLAG_LOCKED; // 锁死，禁止小块切分
    desc_dma->start_addr = desc_normal->start_addr + desc_normal->size;
    desc_dma->size = 150 * 1024;

    desc_normal->next = desc_dma;
    desc_dma->prev = desc_normal;

    // 3. EMERGENCY Zone
    mem_desc_t* desc_emg = &mem_descriptors[2];
    desc_emg->is_valid = 1;
    desc_emg->zone_id = ZONE_EMERGENCY;
    desc_emg->flags = MEM_FLAG_FREE;
    desc_emg->start_addr = desc_dma->start_addr + desc_dma->size;
    desc_emg->size = 62 * 1024; // 剩下的大约62K

    desc_dma->next = desc_emg;
    desc_emg->prev = desc_dma;
    desc_emg->next = 0;

    root_desc = desc_normal;
    vga_print("[HEAP V2] Zones Initialized. Normal/DMA/Emergency ready.\n");
}

// 核心分配逻辑 (支持指定 Zone)
void* kmalloc_zone(uint32_t size, mem_zone_t zone) {
    if (size == 0) return 0;

    // 【改进3】热块缓存直接拦截极速分配 (仅限 NORMAL 区且尺寸相符)
    if (zone == ZONE_NORMAL && size <= HOT_CACHE_SIZE && cpu_hot_cache.count > 0) {
        cpu_hot_cache.count--;
        return cpu_hot_cache.blocks[cpu_hot_cache.count];
    }

    uint32_t aligned_size = (size + (current_min_granularity - 1)) & ~(current_min_granularity - 1);
    mem_desc_t* curr = root_desc;

    while (curr != 0) {
        // 【改进4】精准匹配：必须是 FREE，且 Zone 匹配
        if ((curr->flags & MEM_FLAG_FREE) && curr->zone_id == zone && curr->size >= aligned_size) {

            // 【改进6 & 4】检查是否是被锁死的大块（比如 DMA 且不许切分）
            // 如果是 DMA Zone，且请求大小不等于整个块，且被锁死，则禁止切分！
            if ((curr->flags & MEM_FLAG_LOCKED) && curr->size > aligned_size) {
                curr = curr->next;
                continue; // 保护连续大块，继续找下一个
            }

            // 尝试切分
            if (curr->size > aligned_size + current_min_granularity) {
                mem_desc_t* new_desc = get_unused_descriptor();
                if (new_desc) {
                    new_desc->is_valid = 1;
                    new_desc->zone_id = curr->zone_id;
                    new_desc->flags = MEM_FLAG_FREE; // 剩下的是自由的
                    new_desc->start_addr = curr->start_addr + aligned_size;
                    new_desc->size = curr->size - aligned_size;

                    new_desc->prev = curr;
                    new_desc->next = curr->next;
                    if (curr->next) curr->next->prev = new_desc;
                    curr->next = new_desc;

                    curr->size = aligned_size;
                }
            }

            // 分配成功，清除 FREE 和 LAZY 标记
            curr->flags &= ~(MEM_FLAG_FREE | MEM_FLAG_LAZY);
            return (void*)curr->start_addr;
        }
        curr = curr->next;
    }
    return 0; // OOM 或没有合适的块
}

// 默认 kmalloc 走普通区
void* kmalloc(uint32_t size) {
    return kmalloc_zone(size, ZONE_NORMAL);
}

// 【改进8】批量分配
int kmalloc_batch(uint32_t size, int count, void** out_array) {
    int allocated = 0;
    for (int i = 0; i < count; i++) {
        out_array[i] = kmalloc(size);
        if (!out_array[i]) break; // 如果分配失败则中断
        allocated++;
    }
    return allocated;
}

// 释放内存
void kfree(void* ptr) {
    if (!ptr) return;

    mem_desc_t* target = 0;
    for (int i = 0; i < MAX_MEM_DESCRIPTORS; i++) {
        if (mem_descriptors[i].is_valid && mem_descriptors[i].start_addr == (uint32_t)ptr) {
            target = &mem_descriptors[i];
            break;
        }
    }
    if (!target) return;

    // 【改进3】热缓存回收：如果是常用小尺寸，直接塞进 Per-CPU 缓存，完全不走链表操作！
    if (target->zone_id == ZONE_NORMAL && target->size <= HOT_CACHE_SIZE && cpu_hot_cache.count < HOT_CACHE_MAX) {
        cpu_hot_cache.blocks[cpu_hot_cache.count++] = ptr;
        // 注意：这里连 target->flags 都不改，保持它是“已分配”状态，伪装被 CPU 吃掉了
        return;
    }

    // 【改进5】惰性合并：标记为空闲和惰性状态，不立刻合并
    target->flags |= (MEM_FLAG_FREE | MEM_FLAG_LAZY);
    lazy_free_count++;

    // 【改进2】高水位触发：当散块堆积超过阈值，主动触发一次全局合并
    if (lazy_free_count > LAZY_FREE_HIGH_WATERMARK) {
        kheap_compact_lazy();
    }
}

// 【改进8】批量释放
void kfree_batch(int count, void** ptr_array) {
    for (int i = 0; i < count; i++) {
        kfree(ptr_array[i]);
    }
}

// 【改进5 & 4】后台定时规整/水位规整 (消除碎片)
void kheap_compact_lazy() {
    mem_desc_t* curr = root_desc;
    while (curr != 0) {
        if (curr->flags & MEM_FLAG_FREE) {
            // 向后看，如果下一个块也是空闲的，且属于同一个 Zone，且都不是 LOCKED（不可合并）的块
            if (curr->next != 0 && (curr->next->flags & MEM_FLAG_FREE) &&
                curr->zone_id == curr->next->zone_id &&
                !(curr->flags & MEM_FLAG_LOCKED) && !(curr->next->flags & MEM_FLAG_LOCKED)) {

                mem_desc_t* drop = curr->next;
                curr->size += drop->size; // 吃掉下一个块

                curr->next = drop->next;
                if (drop->next) drop->next->prev = curr;

                drop->is_valid = 0; // 回收盘子

                // 去除惰性标记，表示已经规整过了
                curr->flags &= ~MEM_FLAG_LAZY;
                if (lazy_free_count > 0) lazy_free_count--;

                // 注意：合并后继续检查当前的块（因为它变大了，可能还能和更后面的合并）
                continue;
            }
        }
        curr = curr->next;
    }
    // 规整完毕，水位清零
    lazy_free_count = 0;
}

// 【改进7】动态调控最小切分粒度
void kheap_set_min_granularity(uint32_t bytes) {
    if (bytes >= 4 && bytes <= 4096) {
        current_min_granularity = bytes;
    }
}