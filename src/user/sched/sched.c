#include "user/sched/sched.h"
#include "isr.h"
#include "errno.h"
#include <stdint.h>
#include <stddef.h>

uint64_t pid = 0;
task_t *current = NULL;
task_t *start = NULL;

uint64_t pid_alloc() {
    return ++pid;
}

int fd_alloc(file_t *file) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (!current->fds[i]) {
            current->fds[i] = file;
            return i;
        }
    }
    return -EMFILE;
}

int fd_free(int fd)
{
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
    // 1. ANTES DE MUDAR DE TAREFA: Salva o frame da tarefa antiga
    current->frame = *frame; 

    // 2. Se a tarefa antiga ainda estava rodando, ela volta a ficar PRONTA.
    // Se ela foi bloqueada por sleep/IO, o estado dela NÃO será alterado aqui!
    if (current->state == STATE_RUNNING) {
        current->state = STATE_READY;
    }

    task_t *p = NULL;
    if (!current->next)
        p = start;
    else
        p = current->next;

    task_t *pivot = p; // Guarda o ponto de início para detectar lista vazia/ocupada

    for (;;) {
        if (p->state == STATE_READY) {
            // 3. Atualiza o ponteiro global para a nova tarefa
            current = p;
            current->ticks = 0;
            current->quantum = QUANTUM_TICKS;
            
            // 4. Altera o estado da nova tarefa para RUNNING
            current->state = STATE_RUNNING; 

            if (current->user_cr3) {
                asm volatile ("mov %0, %%cr3" :: "r"(current->user_cr3) : "memory");
            }
            
            // 5. Envia o frame da nova tarefa de volta para o manipulador em assembly
            *frame = current->frame;
            return; 
        }

        // Avança na lista circular
        if (!p->next) {
            p = start;
        } else {
            p = p->next;
        }

        // Se checou todo mundo e ninguém está pronto
        if (p == pivot) {
            break;
        }
    }

    // Fallback caso todas as tarefas estejam bloqueadas
    asm volatile("sti; hlt");
}
