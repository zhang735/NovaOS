#include <kernel/io.h>
#include <kernel/scheduler.h>
#include <drivers/vga.h>

void timer_handler() {
    // 每次时钟滴答，发送 End of Interrupt 给控制器
    outb(0x20, 0x20);

    // 铁腕逻辑：强行触发调度
    schedule();
}

void pit_init(uint32_t hz) {
    uint32_t divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
}