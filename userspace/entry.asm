bits 64
extern main
global entry

entry:
    call main

    ; O retorno de main() vira o status de exit().
    mov edi, eax

    ; _NR_exit = 16
    mov eax, 16
    syscall

.returned:
    ud2
    jmp .returned