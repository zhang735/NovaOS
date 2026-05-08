.section .text
.global switch_context

# C 语言声明: void switch_context(uint32_t* old_esp, uint32_t new_esp);
switch_context:
    # 1. 保护当前线程的上下文
    push %ebp
    push %ebx
    push %esi
    push %edi

    # 2. 获取参数 old_esp (在栈的 20 偏移处)
    mov 20(%esp), %eax
    # 将当前的栈顶指针保存到 old_esp 指向的内存
    mov %esp, (%eax)

    # 3. 获取参数 new_esp (在栈的 24 偏移处)
    # 这一步是灵魂：CPU 的栈瞬间变成了下一个线程的栈！
    mov 24(%esp), %esp

    # 4. 恢复新线程的上下文
    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    # 5. 灵魂跳跃：弹出新线程栈中的 entry 地址，跳转过去！
    ret