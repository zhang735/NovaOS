#ifndef NOVA_GDT_H
#define NOVA_GDT_H
#include <stdint.h>

void gdt_init();
void tss_set_kernel_stack(uint32_t esp0);

#endif