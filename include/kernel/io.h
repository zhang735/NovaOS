#ifndef DICTATOR_IO_H
#define DICTATOR_IO_H

#include <stdint.h>

// 向端口写一个字节
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

// 从端口读一个字节
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// 简单的重启命令：通过键盘控制器 8042 强制复位 CPU
static inline void sys_reboot() {
    uint8_t good = 0x02;
    while (good & 0x02) good = inb(0x64);
    outb(0x64, 0xFE);
}

// 简单的关机命令：仅对 QEMU 等虚拟机有效 (ACPI 关机太复杂，这是捷径)
static inline void sys_shutdown() {
    __asm__ volatile ("outw %0, %1" : : "a"((uint16_t)0x2000), "d"((uint16_t)0x604));
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ( "outw %0, %1" : : "a"(val), "Nd"(port) );
}
#endif