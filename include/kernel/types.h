#ifndef DICTATOR_TYPES_H
#define DICTATOR_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 以后在这里定义内核特有的属性，比如对齐、禁止优化等
#define __packed __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))

// 定义物理地址类型，方便后期从 32 位扩展到 64 位
typedef uint32_t phys_addr_t;
typedef uint32_t virt_addr_t;

#endif