#ifndef NOVA_SCHEDULER_H
#define NOVA_SCHEDULER_H

// 直接引入任务结构体定义，不再重复定义！
#include <kernel/task.h>

// 进程与线程创建接口
process_t* create_process(uint32_t pid, bool foreground);
thread_t* create_thread(process_t* parent, void (*entry)(), char* name);

// 调度接口
void scheduler_run();
void schedule();

// 虚拟线程切换接口
void vthread_create(thread_t* owner, void (*entry)());
void vthread_yield();

#endif