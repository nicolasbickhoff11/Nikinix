#include "syscall/regs.h"
#include "vfs/vfs.h"
#include "user/sched/sched.h"
#include "print.h"
#include "errno.h"
#include "malloc.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

long (*syscall_table[256])(struct user_regs *regs) = {0};

extern char last_char;
extern uint64_t timer_ticks;

typedef long ssize_t;

#define O_RDONLY  0x00
#define O_WRONLY  0x01
#define O_RDWR    0x02
#define O_CREATE  0x40
#define O_TRUNC   0x200
#define O_APPEND  0x400

#ifndef ERR_PTR
#define ERR_PTR(error) ((vnode_t *)(intptr_t)(error))
#endif

#ifndef IS_ERR
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#endif

#ifndef PTR_ERR
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#endif

int sys_open(const char *name, int flags, mode_t mode)
{
    if (!name)
        return -EFAULT;

    int access = flags & 0x03;

    if (access != O_RDONLY &&
        access != O_WRONLY &&
        access != O_RDWR)
        return -EINVAL;

    vnode_t *node = vfs_resolve(name);

    if (IS_ERR(node))
        return PTR_ERR(node);

    if (!node && (flags & O_CREATE)) {
        int ret = vfs_create_user_path(name, mode, &node);

        if (ret < 0)
            return ret;

        if (!node)
            return -EIO;
    }

    if (!node)
        return -ENOENT;

    if ((flags & O_TRUNC) && access == O_RDONLY)
        return -EACCES;

    file_t *file = malloc(sizeof(file_t));

    if (!file)
        return -ENOMEM;

    file->vnode = node;
    file->offset = 0;
    file->flags = flags;
    file->type = FILE_NORMAL;
    file->refcount = 1;
    file->console_type = CONSOLE_NONE;

    if (flags & O_APPEND)
        file->offset = node->size;

    if (flags & O_TRUNC) {
        int ret = vfs_truncate(node, 0);

        if (ret < 0) {
            free(file);
            return ret;
        }

        file->offset = 0;
    }

    int fd = fd_alloc(file);

    if (fd < 0) {
        free(file);
        return fd;
    }

    return fd;
}

ssize_t sys_write(int fd, const void *buf, size_t size)
{
    if (!buf && size)
        return -EFAULT;

    if (size == 0)
        return 0;

    file_t *file = fd_get(fd);

    if (!file)
        return -EBADF;

    int access = file->flags & 0x03;

    if (access != O_WRONLY && access != O_RDWR)
        return -EBADF;

    if (file->type == FILE_CONSOLE) {
        if (file->console_type != CONSOLE_STDOUT &&
            file->console_type != CONSOLE_STDERR)
            return -EBADF;

        const char *str = buf;

        for (size_t i = 0; i < size; i++)
            kprintf("%c", str[i]);

        return (ssize_t)size;
    }

    if (file->type == FILE_NORMAL) {
        ssize_t written = vfs_write(
            file->vnode,
            buf,
            size,
            file->offset
        );

        if (written < 0)
            return written;

        file->offset += written;

        return written;
    }

    return -EINVAL;
}

ssize_t sys_read(int fd, void *buf, size_t size)
{
    if (!buf && size)
        return -EFAULT;

    if (size == 0)
        return 0;

    file_t *file = fd_get(fd);

    if (!file)
        return -EBADF;

    int access = file->flags & 0x03;

    if (access != O_RDONLY && access != O_RDWR)
        return -EBADF;

    if (file->type == FILE_CONSOLE) {
        if (file->console_type != CONSOLE_STDIN)
            return -EBADF;

        char *str = buf;
        size_t counter = 0;

        while (counter < size) {
            asm volatile("sti; hlt" ::: "memory");

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

                continue;
            }

            str[counter++] = c;
            kprintf("%c", c);
        }

        return (ssize_t)counter;
    }

    if (file->type == FILE_NORMAL) {
        ssize_t read = vfs_read(
            file->vnode,
            buf,
            size,
            file->offset
        );

        if (read < 0)
            return read;

        file->offset += read;

        return read;
    }

    return -EINVAL;
}

int sys_close(int fd)
{
    if (fd <= 2)
        return -EBADF;

    if (!fd_get(fd))
        return -EBADF;

    return fd_free(fd);
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

char hostname[512] = "nikinix";

int sys_uname(struct utsname *ptr)
{
    if (!ptr)
        return -EFAULT;

    strcpy(ptr->sysname, "Nikinix");

    strncpy(ptr->nodename, hostname, 64);
    ptr->nodename[64] = '\0';

    strcpy(ptr->release, "v0.9");
    strcpy(ptr->version, "No SIGSEGV so far v0.9");
    strcpy(ptr->machine, "x86_64");

    return 0;
}

ssize_t sys_getdirentries(
    const char *name,
    dirent_t *entries,
    size_t max_entries
)
{
    if (!name || (!entries && max_entries))
        return -EFAULT;

    if (!current)
        return -ESRCH;

    vnode_t *node = vfs_resolve(name);

    if (IS_ERR(node))
        return PTR_ERR(node);

    if (!node)
        return -ENOENT;

    if (!S_ISDIR(node->mode))
        return -ENOTDIR;

    return vfs_readdir(node, entries, max_entries);
}

int sys_mkdir(const char *name, mode_t mode)
{
    if (!current)
        return -ESRCH;

    if (!name)
        return -EFAULT;

    vnode_t *cwd = current->current_cwd.dir;

    if (!cwd)
        cwd = vfs_root;

    if (!cwd)
        return -ENOENT;

    vnode_t *res = NULL;

    return vfs_create_path(
        cwd,
        name,
        mode | S_IFDIR,
        &res
    );
}

int sys_create(const char *name, mode_t mode)
{
    if (!current)
        return -ESRCH;

    if (!name)
        return -EFAULT;

    vnode_t *cwd = current->current_cwd.dir;

    if (!cwd)
        cwd = vfs_root;

    if (!cwd)
        return -ENOENT;

    vnode_t *res = NULL;

    return vfs_create_path(
        cwd,
        name,
        mode | S_IFREG,
        &res
    );
}

int sys_remove(const char *name)
{
    if (!current)
        return -ESRCH;

    if (!name)
        return -EFAULT;

    vnode_t *cwd = current->current_cwd.dir;

    if (!cwd)
        cwd = vfs_root;

    if (!cwd)
        return -ENOENT;

    return vfs_remove_path(cwd, name);
}

int sys_sethostname(const char *new_hostname)
{
    if (!new_hostname)
        return -EFAULT;

    size_t len = strlen(new_hostname);

    if (len >= sizeof(hostname))
        return -ENAMETOOLONG;

    strncpy(hostname, new_hostname, sizeof(hostname) - 1);
    hostname[sizeof(hostname) - 1] = '\0';

    return 0;
}

int sys_getcwd(char *buf, size_t len)
{
    if (!current)
        return -ESRCH;

    if (!buf)
        return -EFAULT;

    if (len == 0)
        return -EINVAL;

    vnode_t *node = current->current_cwd.dir;

    if (!node)
        return -ENOENT;

    return vnode_get_path(node, buf, len);
}

int sys_chdir(const char *dir)
{
    if (!current)
        return -ESRCH;

    if (!dir)
        return -EFAULT;

    vnode_t *node = vfs_resolve(dir);

    if (IS_ERR(node))
        return PTR_ERR(node);

    if (!node)
        return -ENOENT;

    if (!S_ISDIR(node->mode))
        return -ENOTDIR;

    current->current_cwd.dir = node;

    return 0;
}

int sys_sleep(int seconds)
{
    if (seconds < 0)
        return -EINVAL;

    if (seconds == 0)
        return 0;

    uint64_t ticks = (uint64_t)seconds * 100;
    uint64_t start = timer_ticks;

    while ((timer_ticks - start) < ticks)
        asm volatile("sti; hlt" ::: "memory");

    return 0;
}

/* Syscall wrappers */

long _x64_sys_open(struct user_regs *regs)
{
    return sys_open(
        (const char *)regs->rdi,
        (int)regs->rsi,
        (mode_t)regs->rdx
    );
}

long _x64_sys_close(struct user_regs *regs)
{
    return sys_close((int)regs->rdi);
}

long _x64_sys_chdir(struct user_regs *regs)
{
    return sys_chdir((const char *)regs->rdi);
}

long _x64_sys_getcwd(struct user_regs *regs)
{
    return sys_getcwd(
        (char *)regs->rdi,
        (size_t)regs->rsi
    );
}

long _x64_sys_sethostname(struct user_regs *regs)
{
    return sys_sethostname((const char *)regs->rdi);
}

long _x64_sys_mkdir(struct user_regs *regs)
{
    return sys_mkdir(
        (const char *)regs->rdi,
        (mode_t)regs->rsi
    );
}

long _x64_sys_create(struct user_regs *regs)
{
    return sys_create(
        (const char *)regs->rdi,
        (mode_t)regs->rsi
    );
}

long _x64_sys_remove(struct user_regs *regs)
{
    return sys_remove((const char *)regs->rdi);
}

long _x64_sys_sleep(struct user_regs *regs)
{
    return sys_sleep((int)regs->rdi);
}

long _x64_sys_getdirentries(struct user_regs *regs)
{
    return sys_getdirentries(
        (const char *)regs->rdi,
        (dirent_t *)regs->rsi,
        (size_t)regs->rdx
    );
}

long _x64_sys_write(struct user_regs *regs)
{
    return sys_write(
        (int)regs->rdi,
        (const void *)regs->rsi,
        (size_t)regs->rdx
    );
}

long _x64_sys_read(struct user_regs *regs)
{
    return sys_read(
        (int)regs->rdi,
        (void *)regs->rsi,
        (size_t)regs->rdx
    );
}

long _x64_sys_uname(struct user_regs *regs)
{
    return sys_uname((struct utsname *)regs->rdi);
}

void syscall_main(struct user_regs *regs)
{
    if (!regs)
        return;

    uint64_t num = regs->rax;

    if (num >= 256 || !syscall_table[num]) {
        regs->rax = (uint64_t)(int64_t)-ENOSYS;
        return;
    }

    regs->rax = (uint64_t)syscall_table[num](regs);
}