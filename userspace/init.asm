bits 64

global _start
extern main

_start:
    xor rax, rax
    mov rdi, 1
    mov rsi, buf
    mov rdx, size

    int 0x80

    call main

    jmp $

buf: db "No SIGSEGV so far, I hate SIGSEGVs and I love Nikinix", 10
size: equ $ - buf

