project(libp LANGUAGES C)

add_library(p STATIC print.c malloc.c exit.c)

target_compile_options(p PRIVATE
        -ffreestanding
        -fno-stack-protector
        -fno-builtin
)

target_link_options(p PRIVATE -nostdlib)
