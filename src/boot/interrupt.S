.section .text
.global irq0
.global irq1
.global isr14
.global isr128

.extern schedule
.extern keyboard_handler
.extern page_fault_handler
.extern syscall_handler

irq0: # 时钟
    pusha
    movb $0x20, %al
    outb %al, $0x20
    call schedule
    popa
    iret

irq1: # 键盘
    pusha
    call keyboard_handler
    movb $0x20, %al
    outb %al, $0x20
    popa
    iret

isr14: # 缺页异常
    cli
    call page_fault_handler
    hlt

isr128: # 系统调用 (int 0x80)
    cli
    pusha
    push %esp
    call syscall_handler
    add $4, %esp
    popa
    sti
    iret