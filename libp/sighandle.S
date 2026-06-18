.global signal_restorer
.type signal_restorer, @function
.text

signal_restorer:
  movq $15, %rax
  syscall

