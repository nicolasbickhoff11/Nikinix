#include "syscall/regs.h"
#include "vfs/vfs.h"
#include "print.h"
#include "errno.h"
#include <stdint.h>
#include <stddef.h>

long (*syscall_table[256])(struct user_regs *regs) = {0};
extern char last_char;

typedef long ssize_t;

ssize_t sys_write(int fd, const void *buf, size_t size)
{
    if (!buf)
        return -EFAULT; // EFAULT

    if (fd != 1 && fd != 2)
        return -EBADF;  // EBADF

    if (size == 0)
        return 0;

    const char *str = buf;

    for (size_t i = 0; i < size; i++)
        kprintf("%c", str[i]);

    return (ssize_t)size;
}

ssize_t sys_read(int fd, void *buf, size_t size)
{
    if (!buf)
        return -EFAULT; // EFAULT

    if (fd != 0)
        return -EBADF;  // EBADF

    if (size == 0)
        return 0;

    char *str = buf;
    size_t counter = 0;

    while (counter < size) {
        asm volatile ("sti; hlt");

        if (last_char == '\0')
            continue;

        char c = last_char;
        last_char = '\0';

        if (c == '\n')
            break;

        if (c == '\b') {
    if (counter > 0) {
        counter--;
        str[counter] = '\0';

        cursor_x -= 8;
        if (cursor_x < 0)
            cursor_x = 8;

        fb_print_char(' ');
        cursor_x -= 8;

        if (cursor_x < 0)
            cursor_x = 8;

        com1_puts("\b \b");
    }
        } else {

            str[counter++] = c;

            kprintf("%c", c);
        }
    }

    return (ssize_t)counter;
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

int sys_uname(struct utsname *ptr) {
    if (!ptr) return -EFAULT;

    strcpy(ptr->sysname, "Nikinix");
    strcpy(ptr->nodename, "nikinix");
    strcpy(ptr->release, "v0.5");
    strcpy(ptr->version, "No SIGSEGV so far v0.5");
    strcpy(ptr->machine, "x86_64");
    return 0;
}

ssize_t sys_readdir(const char *name, dirent_t *entries, size_t max_entries)
{
    if (!name || !entries) {
        return -EFAULT;
    }
    vnode_t *node = NULL;

    int ret = vfs_open(name, &node);
    if (ret < 0) {
        return ret;
    }
    ssize_t result = vfs_readdir(node, entries, max_entries);

    // se o VFS possuir refcount/close:
    // vnode_put(node);

    return result;
}

extern uint64_t timer_ticks;

int sys_sleep(int seconds) {
    if (!seconds) return 1;
    uint64_t future_tick = timer_ticks + 100 * seconds;
    while (timer_ticks < future_tick) {
        asm volatile ("hlt");
    }
    return 0;
}

long _x64_sys_sleep(struct user_regs *regs) {
    return sys_sleep(regs->rdi);
}

long _x64_sys_readdir(struct user_regs *regs) {
    return sys_readdir(regs->rdi, regs->rsi, regs->rdx);
}

long _x64_sys_write(struct user_regs *regs) {
    return sys_write(regs->rdi, regs->rsi, regs->rdx);
}

long _x64_sys_read(struct user_regs *regs) {
    return sys_read(regs->rdi, regs->rsi, regs->rdx);
}

long _x64_sys_uname(struct user_regs *regs) {
    return sys_uname(regs->rdi);
}

void syscall_main(struct user_regs *regs) {

    uint64_t num = regs->rax;
    if (!syscall_table[num]) {
        regs->rax = -ENOSYS; // ENOSYS
        return;
    } else {
        regs->rax = syscall_table[num](regs);
        return;
    }
}