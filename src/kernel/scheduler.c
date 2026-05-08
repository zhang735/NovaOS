#include <kernel/scheduler.h>
#include <kernel/memory.h>
#include <drivers/vga.h>

extern void switch_context(uint32_t* old_esp, uint32_t new_esp);
extern void load_page_directory(uint32_t phys_addr);
extern void tss_set_kernel_stack(uint32_t esp0);

static thread_t* thread_queue = NULL;
thread_t* current_thread = NULL;

process_t* create_process(uint32_t pid, bool foreground) {
    process_t* proc = (process_t*)pmm_alloc_page();
    proc->pid = pid;
    proc->is_foreground = foreground;
    proc->page_dir = (uint32_t*)vmm_create_process_dir();
    return proc;
}

thread_t* create_thread(process_t* parent, void (*entry)(), char* name) {
    (void)name;

    thread_t* t = (thread_t*)pmm_alloc_page();
    uint32_t* stack = (uint32_t*)pmm_alloc_page();

    for(int i=0; i<1024; i++) stack[i] = 0;

    uint32_t* esp = (uint32_t*)((uint32_t)stack + 4096);
    *(--esp) = (uint32_t)entry; // EIP
    *(--esp) = 0; // EBP
    *(--esp) = 0; // EBX
    *(--esp) = 0; // ESI
    *(--esp) = 0; // EDI

    t->esp = (uint32_t)esp;
    t->parent = parent;
    t->v_list = NULL;

    t->next = thread_queue;
    thread_queue = t;

    return t;
}

// 降级进入 Ring 3
void enter_user_mode(uint32_t entry_point, uint32_t user_stack) {
    __asm__ volatile("cli");
    __asm__ volatile(
        "mov $0x23, %ax\n\t"
        "mov %ax, %ds\n\t"
        "mov %ax, %es\n\t"
        "mov %ax, %fs\n\t"
        "mov %ax, %gs\n\t"
    );

    __asm__ volatile(
        "pushl $0x23\n\t"       // 用户数据段选择子 SS
        "pushl %0\n\t"          // 用户态栈顶指针 ESP
        "pushfl\n\t"            // EFLAGS
        "popl %%eax\n\t"
        "orl $0x200, %%eax\n\t" // 开启中断标志 (IF)
        "pushl %%eax\n\t"
        "pushl $0x1B\n\t"       // 用户代码段选择子 CS
        "pushl %1\n\t"          // 用户程序入口点 EIP
        "iret\n\t"
        : : "r"(user_stack), "r"(entry_point) : "eax"
    );
}

void schedule() {
    if (!current_thread) return;

    thread_t* old = current_thread;
    thread_t* next = old->next;

    if (!next) next = thread_queue;
    if (old == next) return;

    current_thread = next;

    // 更新 TSS，确保 Ring 3 被中断时 CPU 知道往哪个内核栈压寄存器
    // 假设 t->esp 分配时的页顶地址是向下 4096 处
    tss_set_kernel_stack(((uint32_t)next->esp & 0xFFFFF000) + 4096);

    if (old->parent != next->parent) {
        load_page_directory((uint32_t)next->parent->page_dir);
    }

    switch_context(&(old->esp), next->esp);
}

void scheduler_run() {
    if (!thread_queue) return;
    current_thread = thread_queue;

    // 【关键修改】：在第一次运行进程前，必须设置好 TSS 的内核栈！
    // 这样哪怕 Ring 3 一进去就报错，CPU 也能找到正确的栈来保存异常信息。
    tss_set_kernel_stack(((uint32_t)current_thread->esp & 0xFFFFF000) + 4096);

    load_page_directory((uint32_t)current_thread->parent->page_dir);

    static uint32_t dummy_esp;
    switch_context(&dummy_esp, current_thread->esp);
}