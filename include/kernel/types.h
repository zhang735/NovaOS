#ifndef NOVA_TASK_H
#define NOVA_TASK_H

#include <kernel/types.h>

// 提前声明结构体，解决互相引用的编译报错
struct process;
struct thread;

// 1. 虚拟线程 (V-Thread / Fiber)
typedef struct vthread {
    uint32_t esp;           // 虚拟线程栈顶
    void* stack_base;       // 栈底
    void (*entry)();        // 入口函数
    struct vthread* next;
} vthread_t;

// 2. 进程 (Process)
typedef struct process {
    uint32_t pid;
    uint32_t* page_dir;         // 进程独立的页目录 (物理地址)
    struct thread* main_thread; // 主线程指针
    bool is_foreground;         // 铁腕标记：是否为霸权进程
} process_t;

// 3. 内核线程 (Thread)
typedef struct thread {
    uint32_t esp;           // 内核栈顶
    uint32_t tid;           // 线程 ID
    vthread_t* v_list;      // 拥有的虚拟线程链表
    process_t* parent;      // 所属进程
    struct thread* next;
} thread_t;

#endif