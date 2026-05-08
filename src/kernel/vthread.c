// 虚拟线程切换：手动触发，用于“秒传文件”后的后台静默同步
void vthread_switch(vthread_t* old, vthread_t* new) {
    __asm__ volatile (
        "pushfl; pushal; "     // 保存标志位和所有寄存器
        "movl %%esp, %0; "     // 保存当前栈指针
        "movl %1, %%esp; "     // 载入新虚拟线程栈指针
        "popal; popfl; "       // 恢复寄存器
        : "=m"(old->esp) : "r"(new->esp) : "memory"
    );
}