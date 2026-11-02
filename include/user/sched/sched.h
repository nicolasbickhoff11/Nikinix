#ifndef _USER_SCHED_H
#define _USER_SCHED_H 1

#include <stdint.h>
#include "isr.h"
#include "vfs/vfs.h"

#define TIMER_TICK_MS 10
#define QUANTUM_TICKS 3

#define STATE_READY 0
#define STATE_RUNNING 1
#define STATE_STOPPED 2
#define STATE_WAITING 3
#define STATE_DEAD 4

#define MAX_FDS 256

#define FILE_NORMAL 0
#define FILE_DIR 1
#define FILE_CONSOLE 2

typedef enum {
    CONSOLE_NONE = 0,
    CONSOLE_STDIN = 1,
    CONSOLE_STDOUT = 2,
    CONSOLE_STDERR = 3
} console_type_t;

typedef struct file {
    vnode_t *vnode;
    uint64_t offset;
    int flags;
    int type;
    int refcount;
    console_type_t console_type; // IF the file_t is I/O Consoles
} file_t;

typedef struct cwd {
    vnode_t *dir;
} cwd_t;

typedef struct task_struct {
    uint64_t pid;
    struct interrupt_frame frame;
    uint64_t kernel_rsp;
    uint64_t user_cr3;
    uint64_t quantum;
    uint64_t ticks;
    int state;
    file_t *fds[MAX_FDS];
    cwd_t current_cwd;
    struct task_struct *next;
    struct task_struct *parent;
} task_t;

extern task_t *current;
extern task_t *start;

void scheduler(struct interrupt_frame *frame);

int fd_alloc(file_t *file);
file_t *fd_get(int fd);
int fd_free(int fd);

#endif