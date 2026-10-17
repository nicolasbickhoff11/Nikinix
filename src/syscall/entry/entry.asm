bits 64
global syscall_entry

extern syscall_main
extern kernel_int_stack

syscall_entry:
    ; ------------------------------------------------------------
    ; Salva o RSP do usuário
    ;
    ; ATENÇÃO:
    ; tmp_reg é temporário e não é SMP/reentrancy-safe.
    ; Depois, substituir por swapgs/per-CPU.
    ; ------------------------------------------------------------

    mov cr2, rax
    mov rax, [kernel_int_stack]
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

    ; ------------------------------------------------------------
    ; RSP aponta exatamente para struct user_regs
    ; ------------------------------------------------------------
    mov rdi, rsp

    ; Guarda o endereço do frame.
    ; RBP está salvo no próprio frame, portanto podemos usá-lo
    ; como âncora durante a chamada C.
    mov rbp, rsp

    ; ------------------------------------------------------------
    ; SysV x86-64:
    ; antes do CALL, RSP deve estar alinhado em 16 bytes.
    ;
    ; O frame possui 15 * 8 = 120 bytes.
    ; Se a kernel stack começa alinhada, após os pushes
    ; RSP fica desalinhado por 8 bytes.
    ; ------------------------------------------------------------
    sub rsp, 8

    call syscall_main

    ; Remove o padding usado para alinhamento
    add rsp, 8

    ; Recupera exatamente o endereço do frame
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

