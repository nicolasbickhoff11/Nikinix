#include "user/sched/sched.h"
#include "isr.h"
#include "errno.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

uint64_t pid = 0;
task_t *current = NULL;
task_t *start = NULL;

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