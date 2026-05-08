cmake_minimum_required(VERSION 3.16)
project(NovaOS C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(KERNEL_COMPILE_FLAGS
        -m32
        -ffreestanding
        -fno-stack-protector
        -fno-pic
        -fno-pie
        -Wall
        -Wextra
)

include_directories(include)

set(SOURCES
        src/boot/boot.S
        src/boot/switch.S
        src/boot/interrupt.S
        src/boot/paging.S
        src/kernel/main.c
        src/kernel/memory.c
        src/kernel/scheduler.c
        src/kernel/idt.c
        src/drivers/vga.c
        src/drivers/pit.c
        src/drivers/keyboard.c
        src/kernel/gdt.c
        src/kernel/syscall.c
)

add_executable(NovaOS.elf ${SOURCES})

target_compile_options(NovaOS.elf PRIVATE ${KERNEL_COMPILE_FLAGS})

# 【关键核心】加上了 -Wl,--build-id=none
target_link_options(NovaOS.elf PRIVATE
        -m32
        -T "${CMAKE_SOURCE_DIR}/linker.ld"
        -nostdlib
        -no-pie
        -Wl,--build-id=none
)

add_custom_command(TARGET NovaOS.elf POST_BUILD
        COMMAND objcopy -O binary NovaOS.elf NovaOS.bin
)