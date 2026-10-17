#include <stdint.h>
#include <stddef.h>

/*
 * Nikinix syscall ABI — x86_64
 *
 * RAX = número do syscall
 * RDI = argumento 1
 * RSI = argumento 2
 * RDX = argumento 3
 * R10 = argumento 4
 * R8  = argumento 5
 * R9  = argumento 6
 *
 * Retorno em RAX.
 */

static inline long __nikinix_call(
    long number,
    uintptr_t a1,
    uintptr_t a2,
    uintptr_t a3,
    uintptr_t a4,
    uintptr_t a5,
    uintptr_t a6)
{
    long ret;

    /*
     * Força o compilador a usar estritamente a convenção de chamada do x86_64.
     * Vinculamos as variáveis locais diretamente aos registradores físicos.
     */
    register long r_ax __asm__("rax") = number;
    register uintptr_t r_di __asm__("rdi") = a1;
    register uintptr_t r_si __asm__("rsi") = a2;
    register uintptr_t r_dx __asm__("rdx") = a3;
    register uintptr_t r_10 __asm__("r10") = a4; /* Syscall usa R10 no lugar de RCX */
    register uintptr_t r_8  __asm__("r8")  = a5;
    register uintptr_t r_9  __asm__("r9")  = a6;

    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(r_ax), "D"(r_di), "S"(r_si), "d"(r_dx), "r"(r_10), "r"(r_8), "r"(r_9)
        : "rcx", "r11", "memory"
    );

    return ret;
}

/*
 * Wrappers por quantidade de argumentos.
 * Argumentos não utilizados são preenchidos com zero.
 */

#define NIKINIX_SC0(n) \
    __nikinix_call((long)(n), 0, 0, 0, 0, 0, 0)

#define NIKINIX_SC1(n, a1) \
    __nikinix_call((long)(n), (uintptr_t)(a1), 0, 0, 0, 0, 0)

#define NIKINIX_SC2(n, a1, a2) \
    __nikinix_call((long)(n), (uintptr_t)(a1), \
                   (uintptr_t)(a2), 0, 0, 0, 0)

#define NIKINIX_SC3(n, a1, a2, a3) \
    __nikinix_call((long)(n), (uintptr_t)(a1), \
                   (uintptr_t)(a2), (uintptr_t)(a3), \
                   0, 0, 0)

#define NIKINIX_SC4(n, a1, a2, a3, a4) \
    __nikinix_call((long)(n), (uintptr_t)(a1), \
                   (uintptr_t)(a2), (uintptr_t)(a3), \
                   (uintptr_t)(a4), 0, 0)

#define NIKINIX_SC5(n, a1, a2, a3, a4, a5) \
    __nikinix_call((long)(n), (uintptr_t)(a1), \
                   (uintptr_t)(a2), (uintptr_t)(a3), \
                   (uintptr_t)(a4), (uintptr_t)(a5), 0)

#define NIKINIX_SC6(n, a1, a2, a3, a4, a5, a6) \
    __nikinix_call((long)(n), (uintptr_t)(a1), \
                   (uintptr_t)(a2), (uintptr_t)(a3), \
                   (uintptr_t)(a4), (uintptr_t)(a5), \
                   (uintptr_t)(a6))

#define NIKINIX_SELECT(_1, _2, _3, _4, _5, _6, _7, NAME, ...) NAME

#define syscall(...) \
    NIKINIX_SELECT(__VA_ARGS__, \
        NIKINIX_SC6, NIKINIX_SC5, NIKINIX_SC4, \
        NIKINIX_SC3, NIKINIX_SC2, NIKINIX_SC1, \
        NIKINIX_SC0)(__VA_ARGS__)

/* Strings */

size_t strlen(const char *p)
{
    size_t i = 0;

    if (p == NULL)
        return 0;

    while (p[i] != '\0')
        i++;

    return i;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;

    while (i < n && s1[i] && s1[i] == s2[i])
        i++;

    if (i == n)
        return 0;

    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

/* uname ABI */

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

/* Directory entry ABI */

typedef struct dirent {
    uint64_t inode;
    uint8_t type;
    char name[256];
} dirent_t;

/* Errno values */

#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD         10
#define EAGAIN         11
#define EWOULDBLOCK EAGAIN
#define ENOMEM         12
#define EACCES         13
#define EFAULT         14
#define ENOTBLK        15
#define EBUSY          16
#define EEXIST         17
#define EXDEV          18
#define ENODEV         19
#define ENOTDIR        20
#define EISDIR         21
#define EINVAL         22
#define ENFILE         23
#define EMFILE         24
#define ENOTTY         25
#define ETXTBSY        26
#define EFBIG          27
#define ENOSPC         28
#define ESPIPE         29
#define EROFS          30
#define EMLINK         31
#define EPIPE          32

/* Output helpers */

static void puts_fd1(const char *s)
{
    syscall(0, 1, s, strlen(s));
}

/* Shell */

static void print_error_path(
    const char *command,
    const char *path,
    const char *message)
{
    puts_fd1(command);
    puts_fd1(path);
    puts_fd1(message);
}

int main(void)
{
    puts_fd1("Hello World C!\n");
    puts_fd1("BASIC SHELL!\n");

    dirent_t entries[64];

    for (;;) {
        char buffer[256] = {0};
        struct utsname uts = {0};

        syscall(3, &uts);

        puts_fd1("root@");
        puts_fd1(uts.nodename);
        puts_fd1(":~# ");

        long i = syscall(1, 0, buffer, sizeof(buffer) - 1);

        if (i == -EINTR)
            continue;

        if (i < 0) {
            puts_fd1("\nread: error\n");
            continue;
        }

        /*
         * Não sai do shell quando read retorna 0.
         */
        if (i == 0) {
            puts_fd1("\n");
            continue;
        }

        if ((size_t)i >= sizeof(buffer)) {
            puts_fd1("\nread: invalid length\n");
            continue;
        }

        buffer[i] = '\0';
        buffer[255] = '\0';

        puts_fd1("\n");

        while (i > 0 &&
               (buffer[i - 1] == '\n' ||
                buffer[i - 1] == '\r')) {
            buffer[--i] = '\0';
        }

        if (buffer[0] == '\0')
            continue;

        if (strcmp(buffer, "exit") == 0) {
            puts_fd1("bye\n");
            break;
        }

        /* uname */

        if (strcmp(buffer, "uname") == 0) {
            struct utsname uts = {0};

            long r = syscall(3, &uts);

            if (r < 0) {
                puts_fd1("uname: error\n");
                continue;
            }

            uts.sysname[64] = '\0';
            uts.nodename[64] = '\0';
            uts.release[64] = '\0';
            uts.version[64] = '\0';
            uts.machine[64] = '\0';

            puts_fd1(uts.sysname);
            puts_fd1(" ");
            puts_fd1(uts.nodename);
            puts_fd1(" ");
            puts_fd1(uts.release);
            puts_fd1(" ");
            puts_fd1(uts.version);
            puts_fd1(" ");
            puts_fd1(uts.machine);
            puts_fd1("\n");

            continue;
        }

        /* sethostname */

if (strcmp(buffer, "sethostname") == 0 ||
    strncmp(buffer, "sethostname ", 12) == 0) {

    if (strcmp(buffer, "sethostname") == 0) {
        puts_fd1("usage: sethostname <hostname>\n");
        continue;
    }

    char *hostname = buffer + 12;

    while (*hostname == ' ')
        hostname++;

    if (*hostname == '\0') {
        puts_fd1("sethostname: missing hostname\n");
        continue;
    }

    long r = syscall(8, hostname);

    if (r < 0) {
        puts_fd1("sethostname: failed\n");
        continue;
    }

    puts_fd1("hostname updated\n");
    continue;
}

        /* ls */

        if (strcmp(buffer, "ls") == 0 ||
            strncmp(buffer, "ls ", 3) == 0) {

            char *path;

            if (strcmp(buffer, "ls") == 0) {
                path = "/";
            } else {
                path = buffer + 3;

                while (*path == ' ')
                    path++;

                if (*path == '\0') {
                    puts_fd1("ls: missing operand\n");
                    continue;
                }
            }

            long r = syscall(4, path, entries, 64);

            if (r < 0) {
                puts_fd1("ls: ");
                puts_fd1(path);

                if (r == -EINVAL) {
                    puts_fd1(": invalid argument\n");
                } else if (r == -ENOTDIR) {
                    puts_fd1(": not a directory\n");
                } else if (r == -EIO) {
                    puts_fd1(": filesystem error\n");
                } else {
                    puts_fd1(": cannot open directory\n");
                }

                continue;
            }

            if (r > 64) {
                puts_fd1("ls: invalid result from kernel\n");
                continue;
            }

            for (long j = 0; j < r; j++) {
                entries[j].name[255] = '\0';

                if (entries[j].name[0] == '\0')
                    continue;

                puts_fd1(entries[j].name);

                if (j + 1 < r)
                    puts_fd1(" ");
            }

            puts_fd1("\n");
            continue;
        }

        /* mkdir */

        if (strcmp(buffer, "mkdir") == 0 ||
            strncmp(buffer, "mkdir ", 6) == 0) {

            if (strcmp(buffer, "mkdir") == 0) {
                puts_fd1("mkdir: missing operand\n");
                continue;
            }

            char *path = buffer + 6;

            while (*path == ' ')
                path++;

            if (*path == '\0') {
                puts_fd1("mkdir: missing operand\n");
                continue;
            }

            long r = syscall(5, path);

            if (r < 0) {
                if (r == -EEXIST) {
                    print_error_path("mkdir: ", path,
                                     ": file exists\n");
                } else if (r == -ENOENT) {
                    print_error_path("mkdir: ", path,
                                     ": no such file or directory\n");
                } else if (r == -ENOTDIR) {
                    print_error_path("mkdir: ", path,
                                     ": not a directory\n");
                } else if (r == -EACCES) {
                    print_error_path("mkdir: ", path,
                                     ": permission denied\n");
                } else {
                    print_error_path("mkdir: ", path,
                                     ": cannot create directory\n");
                }
            }

            continue;
        }

        /* create */

        if (strcmp(buffer, "create") == 0 ||
            strncmp(buffer, "create ", 7) == 0) {

            if (strcmp(buffer, "create") == 0) {
                puts_fd1("create: missing operand\n");
                continue;
            }

            char *path = buffer + 7;

            while (*path == ' ')
                path++;

            if (*path == '\0') {
                puts_fd1("create: missing operand\n");
                continue;
            }

            long r = syscall(6, path);

            if (r < 0) {
                if (r == -EEXIST) {
                    print_error_path("create: ", path,
                                     ": file exists\n");
                } else if (r == -ENOENT) {
                    print_error_path("create: ", path,
                                     ": no such file or directory\n");
                } else if (r == -ENOTDIR) {
                    print_error_path("create: ", path,
                                     ": not a directory\n");
                } else if (r == -EACCES) {
                    print_error_path("create: ", path,
                                     ": permission denied\n");
                } else {
                    print_error_path("create: ", path,
                                     ": cannot create file\n");
                }
            }

            continue;
        }

        /* remove */

        if (strcmp(buffer, "remove") == 0 ||
            strncmp(buffer, "remove ", 7) == 0) {

            if (strcmp(buffer, "remove") == 0) {
                puts_fd1("remove: missing operand\n");
                continue;
            }

            char *path = buffer + 7;

            while (*path == ' ')
                path++;

            if (*path == '\0') {
                puts_fd1("remove: missing operand\n");
                continue;
            }

            long r = syscall(7, path);

            if (r < 0) {
                if (r == -ENOENT) {
                    print_error_path("remove: ", path,
                                     ": no such file or directory\n");
                } else if (r == -ENOTDIR) {
                    print_error_path("remove: ", path,
                                     ": not a directory\n");
                } else if (r == -EACCES) {
                    print_error_path("remove: ", path,
                                     ": permission denied\n");
                } else {
                    print_error_path("remove: ", path,
                                     ": cannot remove\n");
                }
            }

            continue;
        }

        puts_fd1("nikinix: command not found: ");
        puts_fd1(buffer);
        puts_fd1("\n");
    }

    return 0;
}
