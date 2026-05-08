#define MULTIBOOT_MAGIC    0x1BADB002
#define MULTIBOOT_FLAGS    0x00000003
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

/* 加上了 "a" (allocatable)，这是解决一切报错的钥匙！ */
.section .multiboot_header, "a"
.align 4
.long MULTIBOOT_MAGIC
.long MULTIBOOT_FLAGS
.long MULTIBOOT_CHECKSUM

.section .bss
.align 16
stack_bottom:
.skip 16384 # 16KB 栈
stack_top:

.section .text
.global _start
_start:
    cli
    mov $stack_top, %esp

    push %ebx
    push %eax

    call kernel_main

    cli
1:  hlt
    jmp 1b