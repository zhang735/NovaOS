#ifndef NOVA_TASK_H
#define NOVA_TASK_H

#include <kernel/types.h>

// 1. 虚拟线程 (V-Thread) - 极其轻量，只保存最少的寄存器
typedef struct vthread {
    uint32_t esp;           // 虚拟线程栈顶
    void* stack_base;
    struct vthread* next;
} vthread_t;

// 2. 内核线程 (Thread) - 抢占单位
typedef struct thread {
    uint32_t esp;           // 内核栈顶
    uint32_t tid;           // 线程 ID
    vthread_t* v_list;      // 拥有的虚拟线程链表
    struct process* parent; // 所属进程
    struct thread* next;
} thread_t;

// 3. 进程 (Process) - 资源单位
typedef struct process {
    uint32_t pid;
    uint32_t* page_dir;     // 进程独立的页目录 (物理地址)
    thread_t* main_thread;
    bool is_foreground;     // 铁腕标记：是否为霸权进程
} process_t;

#endif