project(libp LANGUAGES C ASM)

add_library(
  p STATIC
  print.c
  malloc.c
  memory.c
  string.c
  sys_wrapper.c
  thread.c
  thread.S
  sighandle.c
  sighandle.S
)

target_compile_options(
  p PRIVATE
  -ffreestanding
  -fno-stack-protector
  -fno-builtin
)

target_link_options(p PRIVATE -nostdlib)
