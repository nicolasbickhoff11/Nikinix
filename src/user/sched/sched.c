#include "user/sched/sched.h"
#include "user/sched/fork/fork.h"
#include "isr.h"
#include "errno.h"
#include "map.h"
#include "tss.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

uint64_t pid = 0;
task_t *current = NULL;
task_t *start = NULL;

extern struct tss_entry_struct tss;

uint64_t pid_alloc(void) {
    return ++pid;
}

int fd_alloc(file_t *file) {
    if (!current || !file)
        return -EINVAL;

    for (int i = 0; i < MAX_FDS; i++) {
        if (!current->fds[i]) {
            current->fds[i] = file;
            return i;
        }
    }

    return -EMFILE;
}

file_t *fd_get(int fd) {
    if (!current)
        return NULL;

    if (fd < 0 || fd >= MAX_FDS)
        return NULL;

    return current->fds[fd];
}

int fd_free(int fd) {
    if (!current)
        return -EINVAL;

    if (fd < 0 || fd >= MAX_FDS)
        return -EBADF;

    file_t *file = current->fds[fd];

    if (!file)
        return -EBADF;

    current->fds[fd] = NULL;

    if (file->refcount > 1) {
        file->refcount--;
    } else {
        free(file);
    }

    return 0;
}

int scheduler_remove_task(task_t *task) {
    if (!task || !start)
        return -EINVAL;

    /* Não remover uma tarefa que ainda está executando. */
    if (task == current)
        return -EBUSY;

    task_t *prev = NULL;
    task_t *iter = start;

    while (iter) {
        if (iter == task) {
            if (prev)
                prev->next = iter->next;
            else
                start = iter->next;

            iter->next = NULL;
            return 0;
        }

        prev = iter;
        iter = iter->next;
    }

    return -ESRCH;
}

void wake_up(task_t *task) {
    if (!task)
        return;

    if (task->state == STATE_WAITING)
        task->state = STATE_READY;
}

task_t *get_task(int pid)
{
    task_t *task = start;

    while (task != NULL) {
        if (task->pid == (uint64_t)pid)
            return task;

        task = task->next;
    }

    return NULL;
}

void scheduler_wake_parent(task_t *child) {
    if (!child || !child->parent)
        return;

    task_t *parent = child->parent;

    if (parent->state != STATE_WAITING)
        return;

    if (parent->waiting_pid != child->pid)
        return;

    parent->waiting_pid = 0;
    wake_up(parent);
}

static int get_offset_child(int pid)
{
    for (uint64_t i = 0; i < current->child_count; i++) {
        task_t *task = current->childs[i];

        if (task->pid != (uint64_t)pid)
            continue;

        return (int)i;
    }

    return -ECHILD;
}

int reap_process(uint64_t pid, int *exit_status) {
    task_t *process = get_task(pid);

    if (!process)
        return -ECHILD;

    if (process->parent != current)
        return -ECHILD;

    if (process->state != STATE_ZOMBIE)
        return -EBUSY;

    int status = process->exit_status;
    int offset = get_offset_child(pid);

    if (offset < 0)
        return -ECHILD;

    for (int i = 0; i < MAX_FDS; i++) {
        file_t *file = process->fds[i];

        if (!file)
            continue;

        process->fds[i] = NULL;

        if (file->refcount > 1)
            file->refcount--;
        else
            free(file);
    }

    /*
     * Não liberar filhos que ainda podem estar executando.
     * A adoção dos filhos precisa ser tratada separadamente.
     */

    for (uint64_t i = offset; i + 1 < current->child_count; i++)
        current->childs[i] = current->childs[i + 1];

    current->child_count--;

    scheduler_remove_task(process);

    free(process->childs);

    /*
     * Liberar a pilha kernel e a estrutura task_t
     * somente quando não estiverem mais em uso.
     */

    free(process);

    if (exit_status)
        *exit_status = status;

    return 0;
}

int scheduler_waitpid(uint64_t pid, int *status, int options) {
    if (!current)
        return -ECHILD;

    if (options != 0)
        return -EINVAL; // WNOHANG ainda não implementado

    if (status &&
        !is_mapped(virt_read_cr3(), (uint64_t)status)) {
        return -EFAULT;
    }

    int offset = get_offset_child(pid);

    if (offset < 0)
        return -ECHILD;

    task_t *child = current->childs[offset];

    while (child->state != STATE_ZOMBIE) {
        current->waiting_pid = child->pid;
        current->state = STATE_WAITING;

        asm volatile("int $0x81" ::: "memory");

        /*
         * A tarefa pode ter ficado suspensa.
         * Ao voltar, precisamos verificar novamente.
         */
        offset = get_offset_child(pid);

        if (offset < 0)
            return -ECHILD;

        child = current->childs[offset];
    }

    uint64_t child_pid = child->pid;
    int exit_status;

    int result = reap_process(child_pid, &exit_status);

    if (result < 0)
        return result;

    if (status)
        *status = (exit_status & 0xFF) << 8;

    return (int)child_pid;
}

__attribute__((noreturn))
void scheduler_proc_exit(int error_code) {
    if (!current) {
        for (;;)
            asm volatile("cli; hlt");
    }
    current->exit_status = error_code;
    current->state = STATE_ZOMBIE;

    scheduler_wake_parent(current);

    /*
     * O timer precisa escalonar outra tarefa.
     * Esta tarefa nunca deve voltar a executar.
     */
    asm volatile("int $0x81" ::: "memory");
    for (;;) {
        asm volatile("cli; hlt");
    }
}

void scheduler(struct interrupt_frame *frame) {
    if (!frame || !start)
        return;

    /* Salva o contexto da tarefa atual. */
    if (current) {
        current->frame = *frame;

        if (current->state == STATE_RUNNING)
            current->state = STATE_READY;
    }

    /* Começa pela próxima tarefa ou pela primeira da lista. */
    task_t *p = current && current->next
        ? current->next
        : start;

    task_t *pivot = p;

    do {
        if (p->state == STATE_READY) {
            current = p;

            current->ticks = 0;
            current->quantum = QUANTUM_TICKS;
            current->state = STATE_RUNNING;

            /* Troca o espaço de endereçamento, se necessário. */
            if (current->user_cr3) {
                asm volatile(
                    "mov %0, %%cr3"
                    :
                    : "r"(current->user_cr3)
                    : "memory"
                );
            }

            /* Restaura o contexto da nova tarefa. */
            *frame = current->frame;
            tss.rsp0 = current->kernel_rsp;
            return;
        }

        p = p->next ? p->next : start;

    } while (p != pivot);

    /*
     * Nenhuma tarefa READY foi encontrada.
     * O sistema precisa ter uma tarefa idle disponível.
     */
    for (;;) {
        asm volatile("sti; hlt");
    }
}