#include <stdint.h>
#include "print.h"
#include "vfs/vfs.h"

typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
} tar_header_t;

uint64_t octal_to_u64(const char *str, size_t len)
{
    uint64_t value = 0;

    for (size_t i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '7')
            break;

        value = (value * 8) + (str[i] - '0');
    }

    return value;
}

void tar_list(void *initrd, uint64_t initrd_size)
{
    uint8_t *ptr = initrd;
    uint8_t *end = ptr + initrd_size;

    while (ptr + 512 <= end) {
        tar_header_t *hdr = (tar_header_t *)ptr;

        /* Header vazio = fim do TAR */
        if (hdr->name[0] == '\0')
            break;

        uint64_t size =
            octal_to_u64(hdr->size, sizeof(hdr->size));

        uint64_t mode =
            octal_to_u64(hdr->mode, sizeof(hdr->mode));

        kprintf(
            "TAR: %s size=%llu type=%c mode=%o\n",
            hdr->name,
            size,
            hdr->typeflag,
            mode
        );

        uint8_t *data = ptr + 512;

        vnode_t *curr = NULL;

        /*
         * "./" representa o diretório raiz.
         * Ele já existe no VFS, então não precisamos criá-lo.
         */
        if (strcmp(hdr->name, "./") != 0) {

            mode_t vfs_mode;

            /*
             * '0' ou '\0' = arquivo regular
             * '5'       = diretório
             */
            if (hdr->typeflag == '5') {
                vfs_mode = S_IFDIR | mode;
            }
            else if (hdr->typeflag == '0' ||
                     hdr->typeflag == '\0') {
                vfs_mode = S_IFREG | mode;
            }
            else {
                /*
                 * Outros tipos de TAR ainda não suportados.
                 */
                goto next_header;
            }

            int ret = vfs_create_path(
                vfs_root,
                hdr->name,
                vfs_mode,
                &curr
            );

            if (ret < 0 || !curr) {
                kprintf(
                    "TAR: failed to create %s\n",
                    hdr->name
                );

                goto next_header;
            }
        }

        /*
         * Somente arquivos regulares possuem dados.
         */
        if (hdr->typeflag == '0' ||
            hdr->typeflag == '\0') {

            if (!curr)
                goto next_header;

            int ret = vfs_write(
                curr,
                data,
                size,
                0
            );

            if (ret < 0) {
                kprintf(
                    "TAR: failed to write %s\n",
                    hdr->name
                );
            }
        }

next_header:

        /*
         * TAR sempre ocupa múltiplos de 512 bytes.
         */
        uint64_t data_blocks =
            (size + 511) / 512;

        ptr = data + data_blocks * 512;
    }
}