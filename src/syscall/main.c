#include "syscall/regs.h"
#include "vfs/vfs.h"
#include "user/sched/sched.h"
#include "print.h"
#include "errno.h"
#include "malloc.h"
#include "pmm.h"
#include "map.h"
#include "user/sched/fork/fork.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

/*
 * Inclua também o header real do seu paging.c,
 * caso ele declare is_mapped() e phys_to_virt().
 */

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

#define PAGE_SIZE 4096ULL
#define PAGE_ADDR 0x000FFFFFFFFFF000ULL
#define USER_TOP  0x00007FFFFFFFFFFFULL

#ifndef ERR_PTR
#define ERR_PTR(error) ((vnode_t *)(intptr_t)(error))
#endif

#ifndef IS_ERR
#define IS_ERR(pointer) \
    ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#endif

#ifndef PTR_ERR
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#endif

/*
 * is_mapped() recebe um ponteiro virtual para a PML4.
 * user_cr3 é considerado um endereço físico.
 */
//extern bool is_mapped(uint64_t *pml4, uint64_t virt_addr);

/* ============================================================
 * USER MEMORY VALIDATION
 * ============================================================ */

static bool user_range_mapped(const void *ptr, size_t len)
{
    if (!current)
        return false;

    if (len == 0)
        return true;

    if (!ptr)
        return false;

    uintptr_t start = (uintptr_t)ptr;

    if (start > USER_TOP)
        return false;

    if (len - 1 > USER_TOP - start)
        return false;

    uint64_t *pml4 = (uint64_t *)(uintptr_t)
        phys_to_virt(current->user_cr3 & PAGE_ADDR);

    uintptr_t page = start & ~(uintptr_t)(PAGE_SIZE - 1);
    uintptr_t last = (start + len - 1) &
                     ~(uintptr_t)(PAGE_SIZE - 1);

    for (;;) {
        if (!is_mapped(pml4, page))
            return false;

        if (page == last)
            break;

        page += PAGE_SIZE;
    }

    return true;
}

static bool user_string_mapped(const char *str, size_t max)
{
    if (!current || !str)
        return false;

    uintptr_t start = (uintptr_t)str;

    if (start > USER_TOP)
        return false;

    size_t offset = 0;

    while (offset < max) {
        uintptr_t addr = start + offset;

        if (addr > USER_TOP)
            return false;

        size_t chunk = PAGE_SIZE -
                       (addr & (PAGE_SIZE - 1));

        if (chunk > max - offset)
            chunk = max - offset;

        if (!user_range_mapped((const void *)addr, chunk))
            return false;

        for (size_t i = 0; i < chunk; i++) {
            if (str[offset + i] == '\0')
                return true;
        }

        offset += chunk;
    }

    return false;
}

/* ============================================================
 * OPEN
 * ============================================================ */

int sys_open(const char *name, int flags, mode_t mode)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
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

/* ============================================================
 * WRITE
 * ============================================================ */

ssize_t sys_write(int fd, const void *buf, size_t size)
{
    if (!current)
        return -ESRCH;

    if (size == 0)
        return 0;

    if (!user_range_mapped(buf, size))
        return -EFAULT;

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

/* ============================================================
 * READ
 * ============================================================ */

ssize_t sys_read(int fd, void *buf, size_t size)
{
    if (!current)
        return -ESRCH;

    if (size == 0)
        return 0;

    if (!user_range_mapped(buf, size))
        return -EFAULT;

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

/* ============================================================
 * CLOSE
 * ============================================================ */

int sys_close(int fd)
{
    if (!current)
        return -ESRCH;

    if (fd <= 2)
        return -EBADF;

    if (!fd_get(fd))
        return -EBADF;

    return fd_free(fd);
}

/* ============================================================
 * UNAME
 * ============================================================ */

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
    if (!current)
        return -ESRCH;

    if (!user_range_mapped(ptr, sizeof(*ptr)))
        return -EFAULT;

    strcpy(ptr->sysname, "Nikinix");

    strncpy(ptr->nodename, hostname, 64);
    ptr->nodename[64] = '\0';

    strcpy(ptr->release, "v1.0.0");
    strcpy(ptr->version, "No SIGSEGV so far v1.0.0");
    strcpy(ptr->machine, "x86_64");

    return 0;
}

/* ============================================================
 * GET DIRENTRIES
 * ============================================================ */

ssize_t sys_getdirentries(
    const char *name,
    dirent_t *entries,
    size_t max_entries
)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
        return -EFAULT;

    if (max_entries > SIZE_MAX / sizeof(*entries))
        return -EINVAL;

    size_t bytes = max_entries * sizeof(*entries);

    if (bytes && !user_range_mapped(entries, bytes))
        return -EFAULT;

    vnode_t *node = vfs_resolve(name);

    if (IS_ERR(node))
        return PTR_ERR(node);

    if (!node)
        return -ENOENT;

    if (!S_ISDIR(node->mode))
        return -ENOTDIR;

    return vfs_readdir(node, entries, max_entries);
}

/* ============================================================
 * MKDIR
 * ============================================================ */

int sys_mkdir(const char *name, mode_t mode)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
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

/* ============================================================
 * CREAT
 * ============================================================ */

int sys_creat(const char *name, mode_t mode)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
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

/* ============================================================
 * RMDIR
 * ============================================================ */

int sys_rmdir(const char *name)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
        return -EFAULT;

    vnode_t *cwd = current->current_cwd.dir;

    if (!cwd)
        cwd = vfs_root;

    if (!cwd)
        return -ENOENT;

    vnode_t *node = NULL;

    int ret = vfs_lookup(cwd, name, &node);

    if (ret < 0)
        return ret;

    if (!node)
        return -ENOENT;

    if (!S_ISDIR(node->mode))
        return -ENOTDIR;

    return vfs_remove_path(cwd, name);
}

/* ============================================================
 * UNLINK
 * ============================================================ */

int sys_unlink(const char *name)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(name, 4096))
        return -EFAULT;

    vnode_t *cwd = current->current_cwd.dir;

    if (!cwd)
        cwd = vfs_root;

    if (!cwd)
        return -ENOENT;

    vnode_t *node = NULL;

    int ret = vfs_lookup(cwd, name, &node);

    if (ret < 0)
        return ret;

    if (!node)
        return -ENOENT;

    if (S_ISDIR(node->mode))
        return -EISDIR;

    return vfs_remove_path(cwd, name);
}

/* ============================================================
 * SETHOSTNAME
 * ============================================================ */

int sys_sethostname(const char *new_hostname, size_t len)
{
    if (!current)
        return -ESRCH;

    if (len >= sizeof(hostname))
        return -ENAMETOOLONG;

    if (len && !user_range_mapped(new_hostname, len))
        return -EFAULT;

    if (len)
        memcpy(hostname, new_hostname, len);

    hostname[len] = '\0';

    return 0;
}

/* ============================================================
 * GETCWD
 * ============================================================ */

int sys_getcwd(char *buf, size_t len)
{
    if (!current)
        return -ESRCH;

    if (len == 0)
        return -EINVAL;

    if (!user_range_mapped(buf, len))
        return -EFAULT;

    vnode_t *node = current->current_cwd.dir;

    if (!node)
        return -ENOENT;

    return vnode_get_path(node, buf, len);
}

/* ============================================================
 * CHDIR
 * ============================================================ */

int sys_chdir(const char *dir)
{
    if (!current)
        return -ESRCH;

    if (!user_string_mapped(dir, 4096))
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

/* ============================================================
 * SLEEP
 * ============================================================ */

int sys_sleep(int seconds)
{
    if (!current)
        return -ESRCH;

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

int sys_fork(struct user_regs *regs)
{
    if (!current || !regs)
        return -EFAULT;

    if (!is_mapped(
            virt_read_cr3(),
            (uint64_t)(uintptr_t)current
        ))
        return -EFAULT;

    task_t *child = NULL;

    int ret = do_fork(current, regs, &child);

    if (ret < 0)
        return ret;

    /*
     * O pai receberá child->pid através
     * do retorno normal de syscall_main().
     *
     * O filho já possui RAX = 0 em seu frame.
     */

    return (int)child->pid;
}

typedef int pid_t;

int sys_waitpid(pid_t pid, int *status, int options) {
    return scheduler_waitpid((uint64_t)pid, status, options);
}

__attribute__((noreturn))
void sys_exit(int error_code) {
    scheduler_proc_exit(error_code);
}

pid_t sys_getpid() {
    return (pid_t)current->pid;
}

/* ============================================================
 * SYSCALL WRAPPERS
 * ============================================================ */

long _x64_sys_getpid(struct user_regs *regs) {
    return sys_getpid();
}

long _x64_sys_exit(struct user_regs *regs) {
    sys_exit((int)regs->rdi);
    // WRAPPER
    return 0xDEADBEEF;
}

long _x64_sys_waitpid(struct user_regs *regs) {
    return sys_waitpid((pid_t)regs->rdi, (int*)regs->rsi, (int)regs->rdx);
}

long _x64_sys_fork(struct user_regs *regs) {
    return sys_fork(regs);
}

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
    return sys_sethostname(
        (const char *)regs->rdi,
        (size_t)regs->rsi
    );
}

long _x64_sys_mkdir(struct user_regs *regs)
{
    return sys_mkdir(
        (const char *)regs->rdi,
        (mode_t)regs->rsi
    );
}

long _x64_sys_creat(struct user_regs *regs)
{
    return sys_creat(
        (const char *)regs->rdi,
        (mode_t)regs->rsi
    );
}

long _x64_sys_rmdir(struct user_regs *regs)
{
    return sys_rmdir((const char *)regs->rdi);
}

long _x64_sys_unlink(struct user_regs *regs)
{
    return sys_unlink((const char *)regs->rdi);
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

/* ============================================================
 * SYSCALL MAIN
 * ============================================================ */

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
