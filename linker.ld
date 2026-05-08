ENTRY(_start)

SECTIONS
{
    /* 内核加载到物理地址 1MB */
    . = 1M;

    .text : {
        /* 强制放在可执行代码的最顶端 */
        KEEP(*(.multiboot_header))
        *(.text)
    }

    .rodata ALIGN(4K) : {
        *(.rodata)
    }

    .data ALIGN(4K) : {
        *(.data)
    }

    .bss ALIGN(4K) : {
        *(COMMON)
        *(.bss)
    }

    _kernel_end = .;
}