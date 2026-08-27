#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

long syscall(long number, ...)
{
    va_list ap;
    va_start(ap, number);

    register long rax asm("rax") = number;
    register long rdi asm("rdi") = va_arg(ap, long);
    register long rsi asm("rsi") = va_arg(ap, long);
    register long rdx asm("rdx") = va_arg(ap, long);
    register long r10 asm("r10") = va_arg(ap, long);
    register long r8  asm("r8")  = va_arg(ap, long);
    register long r9  asm("r9")  = va_arg(ap, long);

    va_end(ap);

    asm volatile (
        "int $0x80"
        : "+a"(rax)
        : "D"(rdi),
          "S"(rsi),
          "d"(rdx),
          "r"(r10),
          "r"(r8),
          "r"(r9)
        : "rcx", "r11", "memory"
    );

    return rax;
}

int strlen(const char *p)
{
    int i = 0;

    while (p[i])
        i++;

    return i;
}

int strcmp(const char *s1, const char *s2)
{
    int i = 0;

    while (s1[i] && s1[i] == s2[i])
        i++;

    return (unsigned char)s1[i] -
           (unsigned char)s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;

    while (i < n &&
           s1[i] &&
           s1[i] == s2[i])
        i++;

    if (i == n)
        return 0;

    return (unsigned char)s1[i] -
           (unsigned char)s2[i];
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

typedef struct dirent {
    uint64_t inode;
    uint8_t type;
    char name[256];
} dirent_t;

#define EPERM 1

#define ENOENT 2

#define ESRCH 3

#define EINTR 4

#define EIO 5

#define ENXIO 6

#define E2BIG 7

#define ENOEXEC 8

#define EBADF 9

#define ECHILD 10

#define EAGAIN 11

#define EWOULDBLOCK EAGAIN

#define ENOMEM 12

#define EACCES 13

#define EFAULT 14

#define ENOTBLK 15

#define EBUSY 16

#define EEXIST 17

#define EXDEV 18

#define ENODEV 19

#define ENOTDIR 20

#define EISDIR 21

#define EINVAL 22

#define ENFILE 23

#define EMFILE 24

#define ENOTTY 25

#define ETXTBSY 26

#define EFBIG 27

#define ENOSPC 28

#define ESPIPE 29

#define EROFS 30

#define EMLINK 31

#define EPIPE 32

void main()
{
    syscall(0, 1, "Hello World C!\n", strlen("Hello World C!\n"));
    syscall(0, 1, "BASIC SHELL!\n", strlen("BASIC SHELL!\n"));

    while (1) {
        syscall(0, 1,
                "root@nikinix:~# ",
                strlen("root@nikinix:~# "));

        char buffer[256];

        long i = syscall(1, 0, buffer, sizeof(buffer) - 1);

        if (i < 0) {
            syscall(0, 1, "\nread: error\n", 13);
            continue;
        }

        syscall(0, 1, "\n", 1);

        buffer[i] = '\0';

        /*
         * Remove '\n' caso o sys_read() tenha retornado a
         * quebra de linha.
         */
        if (i > 0 && buffer[i - 1] == '\n')
            buffer[i - 1] = '\0';

        /*
         * uname
         */
        if (strcmp(buffer, "uname") == 0) {
            struct utsname uts = {0};

            long r = syscall(3, &uts);

            if (r < 0) {
                syscall(0, 1, "uname: error\n", 13);
                continue;
            }

            syscall(0, 1, uts.sysname, strlen(uts.sysname));
            syscall(0, 1, " ", 1);

            syscall(0, 1, uts.nodename, strlen(uts.nodename));
            syscall(0, 1, " ", 1);

            syscall(0, 1, uts.release, strlen(uts.release));
            syscall(0, 1, " ", 1);

            syscall(0, 1, uts.version, strlen(uts.version));
            syscall(0, 1, " ", 1);

            syscall(0, 1, uts.machine, strlen(uts.machine));
            syscall(0, 1, "\n", 1);

            continue;
        }

        /*
         * ls
         */
        if (strcmp(buffer, "ls") == 0 ||
            strncmp(buffer, "ls ", 3) == 0) {

            char *path;

            if (strcmp(buffer, "ls") == 0) {
                path = "/";
            } else {
                path = buffer + 3;

                /*
                 * "ls " sem caminho
                 */
                if (*path == '\0') {
                    syscall(0, 1,
                            "ls: missing operand\n",
                            strlen("ls: missing operand\n"));
                    continue;
                }
            }

            dirent_t entries[64];

            long r = syscall(4, path, entries, 64);

            if (r < 0) {
                syscall(0, 1, "ls: ", 4);
                syscall(0, 1, path, strlen(path));

                if (r == -EINVAL) {
                    syscall(0, 1,
                            ": invalid argument\n",
                            strlen(": invalid argument\n"));
                } else if (r == -ENOTDIR) {
                    syscall(0, 1,
                            ": not a directory\n",
                            strlen(": not a directory\n"));
                } else if (r == -EIO) {
                    syscall(0, 1,
                            ": filesystem error\n",
                            strlen(": filesystem error\n"));
                } else {
                    syscall(0, 1,
                            ": cannot open directory\n",
                            strlen(": cannot open directory\n"));
                }

                continue;
            }

            for (long j = 0; j < r; j++) {
                syscall(0, 1,
                        entries[j].name,
                        strlen(entries[j].name));

                if (j + 1 < r)
                    syscall(0, 1, " ", 1);
            }

            syscall(0, 1, "\n", 1);

            continue;
        }

        /*
         * Comando vazio
         */
        if (buffer[0] == '\0')
            continue;

        /*
         * Comando desconhecido
         */
        syscall(0, 1,
                "nikinix: command not found: ",
                strlen("nikinix: command not found: "));

        syscall(0, 1, buffer, strlen(buffer));
        syscall(0, 1, "\n", 1);
    }
}
