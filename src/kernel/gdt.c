#include <kernel/gdt.h>

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt, trap, iomap_base;
} __attribute__((packed));

struct gdt_entry gdt[6];
struct gdt_ptr gp;
struct tss_entry tss;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high = (base >> 24) & 0xFF;
    gdt[num].limit_low = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

static void write_tss(int num, uint16_t ss0, uint32_t esp0) {
    uint32_t base = (uint32_t)&tss;
    uint32_t limit = sizeof(tss);

    gdt_set_gate(num, base, limit, 0xE9, 0x00); // 0xE9: 32-bit TSS

    // 【修改】将 int i 改为 unsigned int i，消除警告
    for(unsigned int i = 0; i < sizeof(tss); i++) ((uint8_t*)&tss)[i] = 0;

    tss.ss0 = ss0;
    tss.esp0 = esp0;
    tss.cs = 0x0B;
    tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x13;
    tss.iomap_base = sizeof(tss);
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}

void gdt_init() {
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base = (uint32_t)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null 段
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Ring 0 代码段 (0x08)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Ring 0 数据段 (0x10)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // Ring 3 代码段 (0x1B) -> DPL=3
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // Ring 3 数据段 (0x23) -> DPL=3
    write_tss(5, 0x10, 0);                      // TSS 段 (0x2B)

    __asm__ volatile(
        "lgdt %0\n\t"
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        "ljmp $0x08, $1f\n\t"
        "1:\n\t"
        "mov $0x2B, %%ax\n\t" // 加载 TSS
        "ltr %%ax\n\t"
        : : "m"(gp) : "memory"
    );
}