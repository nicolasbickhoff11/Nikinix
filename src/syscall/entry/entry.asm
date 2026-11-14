bits 64
global syscall_entry

extern syscall_main
extern kernel_int_stack

extern preemptive_enable
extern preemptive_disable
extern current

syscall_entry:
    ; ------------------------------------------------------------
    ; Salva o RSP do usuário
    ;
    ; ATENÇÃO:
    ; tmp_reg é temporário e não é SMP/reentrancy-safe.
    ; Depois, substituir por swapgs/per-CPU.
    ; ------------------------------------------------------------

    mov cr2, rax
    mov rax, [current]
    mov rax, [rax + 0xA8]
    xchg rsp, rax

    push rax
    mov rax, cr2

    ; ------------------------------------------------------------
    ; Salva os registradores na ordem correspondente a:
    ;
    ; struct user_regs {
    ;     rax, rbx, rcx, rdx, rsi, rdi, rbp,
    ;     r8, r9, r10, r11, r12, r13, r14, r15
    ; };
    ; ------------------------------------------------------------
    push r15
    push r14
    push r13
    push r12
    push r11        ; RFLAGS original
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx        ; RIP original
    push rbx
    push rax

    mov rbp, rsp

    ; Alinha a stack para a ABI System V AMD64.
    and rsp, -16

    call preemptive_disable

    ; RDI é caller-saved.
    ; Recupera o ponteiro do frame depois da chamada anterior.
    mov rdi, rbp

    call syscall_main
    call preemptive_enable

    ; Recupera o frame original.
    mov rsp, rbp

    ; ------------------------------------------------------------
    ; Restaura registradores.
    ;
    ; Depois destes pops:
    ;
    ; RCX = RIP do usuário
    ; R11 = RFLAGS do usuário
    ;
    ; exatamente o que SYSRETQ exige.
    ; ------------------------------------------------------------
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15

    ; ------------------------------------------------------------
    ; DEBUG:
    ; Neste ponto:
    ;   RCX = endereço de retorno do usuário
    ;   R11 = RFLAGS do usuário
    ;
    ; Se quiser conferir no GDB, deixe o int3 aqui.
    ; ------------------------------------------------------------

    ; Restaura a stack do usuário
    pop rsp

    ; RCX = user RIP
    ; R11 = user RFLAGS
    o64 sysret

.loop:
    sysret
    jmp .loop

