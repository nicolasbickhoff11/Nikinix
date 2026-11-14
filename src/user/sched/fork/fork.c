
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "print.h"
#include "map.h"
#include "pmm.h"
#include "malloc.h"
#include "user/sched/fork/copy-cr3.h"
#include "user/sched/sched.h"
#include "errno.h"
#include "syscall/regs.h"

extern uint64_t *setup_user_cr3(void);
extern task_t *start;

#define MAX_CHILDREN 256
#define PAGE_SIZE 4096ULL
#define KERNEL_STACK_PAGES 4

static inline uint64_t read_cr3(void) {
    uint64_t cr3;

    asm volatile (
        "mov %%cr3, %0"
        : "=r"(cr3)
    );

    return cr3;
}

uint64_t *virt_read_cr3(void) {
    return phys_to_virt(read_cr3() & PAGE_ADDR);
}

/*
 * Adiciona uma tarefa ao final da lista global.
 * task->next pertence à lista do scheduler,
 * não à árvore de processos.
 */
static void scheduler_append_task(task_t *task) {
    task->next = NULL;

    if (!start) {
        start = task;
        return;
    }

    task_t *iter = start;

    while (iter->next)
        iter = iter->next;

    iter->next = task;
}

int do_fork(
    task_t *parent,
    const struct user_regs *regs,
    task_t **child_out
) {
    if (!parent || !regs || !child_out)
        return -EFAULT;

    *child_out = NULL;

    uint64_t *current_pml4 = virt_read_cr3();

    if (!is_mapped(current_pml4, (uint64_t)(uintptr_t)parent))
        return -EFAULT;

    if (!is_mapped(current_pml4, (uint64_t)(uintptr_t)child_out))
        return -EFAULT;

    if (parent->child_count >= MAX_CHILDREN)
        return -EAGAIN;

    /* Cria a estrutura da tarefa. */
    task_t *child = malloc(sizeof(*child));

    if (!child)
        return -ENOMEM;

    /* Copia os dados básicos do pai. */
    *child = *parent;

    /* Campos exclusivos do filho. */
    child->pid = 0;
    child->parent = parent;
    child->childs = NULL;
    child->child_count = 0;
    child->next = NULL;

    child->state = STATE_READY;
    child->ticks = 0;
    child->quantum = QUANTUM_TICKS;
    child->waiting_pid = 0;
    child->exit_status = 0;

    /*
     * O filho precisa retornar 0 da syscall fork().
     * IMPORTANTE: parent->frame precisa conter o contexto
     * atual da syscall, não um contexto antigo do scheduler.
     */
    /*
 * Constrói o contexto inicial do filho
 * a partir do contexto REAL da syscall fork.
 *
 * SYSRETQ:
 *   RCX = RIP de retorno
 *   R11 = RFLAGS de retorno
 *
 * O filho começa na instrução seguinte ao syscall.
 */

child->frame.rax = 0;

child->frame.rbx = regs->rbx;
child->frame.rcx = regs->rcx;
child->frame.rdx = regs->rdx;

child->frame.rsi = regs->rsi;
child->frame.rdi = regs->rdi;
child->frame.rbp = regs->rbp;

child->frame.r8  = regs->r8;
child->frame.r9  = regs->r9;
child->frame.r10 = regs->r10;
child->frame.r11 = regs->r11;

child->frame.r12 = regs->r12;
child->frame.r13 = regs->r13;
child->frame.r14 = regs->r14;
child->frame.r15 = regs->r15;

/* Segmentos de usuário da GDT do Nikinix. */
child->frame.cs = 0x23;
child->frame.ss = 0x1B;

/* Não permitir DF, IOPL, NT ou VM herdados. */
child->frame.rflags =
    (regs->r11 & 0x0000000000240FD5ULL) | 0x202ULL;

child->frame.rip = regs->rcx;
child->frame.rsp = regs->user_rsp;

    /* Cria um novo espaço de endereçamento. */
    uint64_t *new_cr3 = setup_user_cr3();

    if (!new_cr3) {
        free(child);
        return -ENOMEM;
    }

    uint64_t *parent_pml4 =
        phys_to_virt(parent->user_cr3 & PAGE_ADDR);

    if (!parent_pml4) {
        free(child);
        return -EFAULT;
    }

    int ret = clone_address_space(parent_pml4, new_cr3);

    if (ret < 0) {
        free(child);
        return ret;
    }

    child->user_cr3 = virt_to_phys(new_cr3);

    /*
     * Aloca uma pilha kernel independente.
     * bitmap_alloc(4) deve retornar o endereço físico
     * de quatro páginas contíguas.
     */
    uint64_t stack_phys = bitmap_alloc(KERNEL_STACK_PAGES);

    if (!stack_phys) {
        free(child);
        return -ENOMEM;
    }

    uintptr_t stack_base =
        (uintptr_t)phys_to_virt(stack_phys);

    if (!stack_base) {
        free(child);
        return -ENOMEM;
    }

    uintptr_t stack_top =
        stack_base + KERNEL_STACK_PAGES * PAGE_SIZE;

    child->kernel_rsp = stack_top;

    /*
     * Prepara o vetor de filhos.
     */
    bool allocated_children = false;

    if (!parent->childs) {
        parent->childs = malloc(
            sizeof(*parent->childs) * MAX_CHILDREN
        );

        if (!parent->childs) {
            free(child);
            return -ENOMEM;
        }

        allocated_children = true;
    }

    /*
     * Herda os descritores de arquivo.
     * O pai e o filho compartilham a mesma descrição
     * de arquivo aberta, incluindo seu offset.
     */
    for (int fd = 0; fd < MAX_FDS; fd++) {
        file_t *file = parent->fds[fd];

        if (file)
            file->refcount++;
    }

    /* Aloca o PID do filho. */
    uint64_t child_pid = pid_alloc();

    if (!child_pid) {
        for (int fd = 0; fd < MAX_FDS; fd++) {
            file_t *file = parent->fds[fd];

            if (file)
                file->refcount--;
        }

        if (allocated_children) {
            free(parent->childs);
            parent->childs = NULL;
        }

        free(child);
        return -EAGAIN;
    }

    child->pid = child_pid;

    /* Registra na árvore de processos. */
    parent->childs[parent->child_count++] = child;

    /* Registra na lista global do scheduler. */
    scheduler_append_task(child);

    *child_out = child;

    return 0;
}
