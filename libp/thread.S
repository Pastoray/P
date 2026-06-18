.global thread_clone
.type thread_clone, @function
.text

thread_clone:

  /*
    %rdi = flags
    %rsi = child_stack
    %rdx = wrapper (function pointer)
    %rcx = fn (function pointer)
    %r8  = ctx (struct pointer)
  */

    subq $16, %rsi
    movq %rcx, (%rsi)
    movq %r8,  8(%rsi)

    // SYS_CLONE
    movq $56, %rax
    syscall

    testq %rax, %rax
    jnz .Lpar

    popq %rdi
    popq %rsi
    call *%rdx
    hlt

.Lpar:
    ret

