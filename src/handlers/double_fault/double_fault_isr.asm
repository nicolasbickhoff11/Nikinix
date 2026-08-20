bits 64

global isr_double_fault
extern double_fault_handler

isr_double_fault:
	push rax
	push rcx
	push rdx
	push rbx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	cld
    mov rdi, rsp
	sub rsp, 8
	call double_fault_handler
	add rsp, 8
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rbx
	pop rdx
	pop rcx
	pop rax
	iretq
