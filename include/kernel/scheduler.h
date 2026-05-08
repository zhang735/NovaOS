#ifndef NOVA_SCHEDULER_H
#define NOVA_SCHEDULER_H

#include <kernel/types.h>

// 1. 虚拟线程 (Virtual Thread / Fiber) - 极速切换
typedef struct vthread {
    uint32_t esp;
    void (*entry)();
    struct vthread* next;
} vthread_t;

// 2. 进程 (Process) - 资源隔离
typedef struct process {
    uint32_t pid;
    uint32_t* page_dir;
    bool is_foreground;
} process_t;

// 3. 内核线程 (Thread) - 抢占单位
typedef struct thread {
    uint32_t esp;           // 栈指针
    uint32_t tid;
    process_t* parent;      // 所属进程
    vthread_t* v_list;      // 拥有的虚拟线程
    struct thread* next;
} thread_t;

process_t* create_process(uint32_t pid, bool foreground);
thread_t* create_thread(process_t* parent, void (*entry)(), char* name);
void scheduler_run();
void schedule();

// 虚拟线程切换接口
void vthread_create(thread_t* owner, void (*entry)());
void vthread_yield();

#endif