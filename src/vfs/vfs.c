#include <stdint.h>
#include "vfs/vfs.h"
#include "print.h"
#include "pmm.h"

vnode_t *vfs_root;


/*
 * ============================================================
 * VFS LOOKUP PATH
 * ============================================================
 *
 * Resolve:
 *
 * /
 * /test.txt
 * /lorem/ipsum/dolor.txt
 * ./test.txt
 * lorem/ipsum
 *
 * Retorna o vnode encontrado em *result.
 */
int vfs_lookup_path(
    vnode_t *root,
    const char *path,
    vnode_t **result
) {
    if (!root || !path || !result)
        return -1;

    *result = NULL;

    vnode_t *current = root;

    /*
     * Caminho absoluto.
     */
    if (*path == '/')
        path++;

    while (*path) {

        /*
         * Ignora barras duplicadas.
         */
        while (*path == '/')
            path++;

        if (!*path)
            break;

        char name[256];
        size_t len = 0;

        /*
         * Extrai o próximo componente.
         */
        while (*path && *path != '/') {

            if (len >= sizeof(name) - 1)
                return -2;

            name[len++] = *path++;
        }

        name[len] = '\0';

        /*
         * .
         */
        if (strcmp(name, ".") == 0)
            continue;

        /*
         * ..
         */
        if (strcmp(name, "..") == 0) {

            if (current->parent)
                current = current->parent;

            continue;
        }

        /*
         * O componente atual precisa ser diretório.
         */
        if (!S_ISDIR(current->mode))
            return -3;

        /*
         * Verifica se o mount possui lookup.
         */
        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->lookup)
            return -4;

        vnode_t *next = NULL;

        int ret = current->mnt->ops->lookup(
            current,
            name,
            &next
        );

        if (ret < 0)
            return ret;

        if (!next)
            return -5;

        current = next;
    }

    *result = current;

    return 0;
}


/*
 * ============================================================
 * VFS CREATE PATH
 * ============================================================
 *
 * Cria ou encontra:
 *
 * /test.txt
 *
 * /lorem/ipsum/dolor.txt
 *
 * Se os diretórios intermediários não existirem,
 * eles serão criados automaticamente.
 *
 * Exemplo:
 *
 * /lorem/ipsum/dolor.txt
 *
 *       /
 *       |
 *     lorem
 *       |
 *     ipsum
 *       |
 *    dolor.txt
 */
int vfs_create_path(
    vnode_t *root,
    const char *path,
    mode_t mode,
    vnode_t **result
) {
    if (!root || !path || !result)
        return -1;

    *result = NULL;

    vnode_t *current = root;

    /*
     * Caminho absoluto.
     */
    if (*path == '/')
        path++;

    /*
     * Caminho vazio -> root.
     *
     * Isso também cobre "/".
     */
    if (!*path) {
        *result = root;
        return 0;
    }

    while (*path) {

        /*
         * Ignora barras.
         */
        while (*path == '/')
            path++;

        if (!*path)
            break;

        char name[256];
        size_t len = 0;

        /*
         * Pega um componente.
         */
        while (*path && *path != '/') {

            if (len >= sizeof(name) - 1)
                return -2;

            name[len++] = *path++;
        }

        name[len] = '\0';

        /*
         * .
         */
        if (strcmp(name, ".") == 0)
            continue;

        /*
         * ..
         */
        if (strcmp(name, "..") == 0) {

            if (current->parent)
                current = current->parent;

            continue;
        }

        /*
         * O vnode atual precisa ser diretório.
         */
        if (!S_ISDIR(current->mode))
            return -3;

        /*
         * O filesystem precisa suportar lookup.
         */
        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->lookup)
            return -4;

        vnode_t *next = NULL;

        /*
         * Primeiro tenta encontrar o objeto.
         */
        int ret = current->mnt->ops->lookup(
            current,
            name,
            &next
        );

        /*
         * Encontrou.
         */
        if (ret == 0 && next) {

            current = next;

            /*
             * Ainda existem componentes.
             * Portanto este objeto precisa ser diretório.
             */
            if (*path) {

                if (!S_ISDIR(current->mode))
                    return -5;
            }

            /*
             * Se este era o último componente,
             * ele já é o resultado.
             */
            if (!*path) {

                *result = current;

                return 0;
            }

            continue;
        }

        /*
         * Não encontrou.
         *
         * Agora precisamos criar.
         */

        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->create)
            return -6;

        /*
         * Ainda existem componentes depois deste.
         *
         * Portanto este componente precisa ser diretório.
         */
        if (*path) {

            ret = current->mnt->ops->create(
                current,
                name,
                S_IFDIR | 0755,
                &next
            );

            if (ret < 0)
                return -7;

            if (!next)
                return -8;

            current = next;

            continue;
        }

        /*
         * Último componente.
         *
         * Usa o mode solicitado pelo caller.
         */
        ret = current->mnt->ops->create(
            current,
            name,
            mode,
            &next
        );

        if (ret < 0)
            return -9;

        if (!next)
            return -10;

        /*
         * Muito importante:
         *
         * devolve o vnode criado.
         */
        *result = next;

        return 0;
    }

    /*
     * Caso o caminho tenha terminado apontando
     * para o diretório atual.
     */
    *result = current;

    return 0;
}


/*
 * ============================================================
 * VFS REMOVE PATH
 * ============================================================
 *
 * Remove:
 *
 * /test.txt
 * /lorem/test.txt
 */
int vfs_remove_path(
    vnode_t *root,
    const char *path
) {
    if (!root || !path)
        return -1;

    size_t len = strlen(path);

    /*
     * Remove barras finais.
     *
     * /test.txt////
     *
     * vira:
     *
     * /test.txt
     */
    while (len > 1 && path[len - 1] == '/')
        len--;

    /*
     * Não permite remover root.
     */
    if (len == 1 && path[0] == '/')
        return -2;

    /*
     * Encontra o último '/'.
     */
    size_t slash = len;

    while (slash > 0 && path[slash - 1] != '/')
        slash--;

    /*
     * Nome do objeto.
     */
    size_t name_len = len - slash;

    if (name_len == 0 ||
        name_len >= 256)
        return -3;

    char name[256];

    memcpy(
        name,
        path + slash,
        name_len
    );

    name[name_len] = '\0';

    /*
     * Caminho do pai.
     */
    char parent_path[256];

    if (slash <= 1) {

        strcpy(
            parent_path,
            "/"
        );

    } else {

        size_t parent_len = slash - 1;

        if (parent_len >= sizeof(parent_path))
            return -4;

        memcpy(
            parent_path,
            path,
            parent_len
        );

        parent_path[parent_len] = '\0';
    }

    /*
     * Encontra o diretório pai.
     */
    vnode_t *parent = NULL;

    int ret = vfs_lookup_path(
        root,
        parent_path,
        &parent
    );

    if (ret < 0)
        return ret;

    if (!parent)
        return -5;

    /*
     * Pai precisa ser diretório.
     */
    if (!S_ISDIR(parent->mode))
        return -6;

    /*
     * Verifica remove().
     */
    if (!parent->mnt ||
        !parent->mnt->ops ||
        !parent->mnt->ops->remove)
        return -7;

    return parent->mnt->ops->remove(
        parent,
        name
    );
}


/*
 * ============================================================
 * VFS INIT
 * ============================================================
 */
void vfs_init(void)
{
    vfs_root = NULL;
}


/*
 * ============================================================
 * VFS MOUNT
 * ============================================================
 */
int vfs_mount(
    const char *path,
    mount_t *mnt
) {
    if (!path ||
        !mnt ||
        !mnt->root)
        return -1;

    /*
     * Mount na raiz.
     */
    if (strcmp(path, "/") == 0) {

        if (vfs_root)
            return -2;

        vfs_root = mnt->root;

        mnt->root->mnt = mnt;

        /*
         * Root não possui pai.
         */
        mnt->root->parent = NULL;

        return 0;
    }

    /*
     * Mountpoints diferentes de "/" ainda
     * não são suportados.
     */
    return -3;
}


/*
 * ============================================================
 * VFS LOOKUP
 * ============================================================
 */
int vfs_lookup(
    vnode_t *dir,
    const char *name,
    vnode_t **result
) {
    if (!dir ||
        !name ||
        !result)
        return -1;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -2;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -3;

    if (!dir->mnt->ops->lookup)
        return -4;

    return dir->mnt->ops->lookup(
        dir,
        name,
        result
    );
}


/*
 * ============================================================
 * VFS CREATE
 * ============================================================
 */
int vfs_create(
    vnode_t *dir,
    const char *name,
    mode_t mode,
    vnode_t **result
) {
    if (!dir ||
        !name ||
        !result)
        return -1;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -2;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -3;

    if (!dir->mnt->ops->create)
        return -4;

    return dir->mnt->ops->create(
        dir,
        name,
        mode,
        result
    );
}


/*
 * ============================================================
 * VFS REMOVE
 * ============================================================
 */
int vfs_remove(
    vnode_t *dir,
    const char *name
) {
    if (!dir || !name)
        return -1;

    if (!S_ISDIR(dir->mode))
        return -2;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -3;

    if (!dir->mnt->ops->remove)
        return -4;

    return dir->mnt->ops->remove(
        dir,
        name
    );
}


/*
 * ============================================================
 * VFS READ
 * ============================================================
 */
ssize_t vfs_read(
    vnode_t *file,
    void *buffer,
    size_t count,
    uint64_t offset
) {
    if (!file || !buffer)
        return -1;

    if (!file->mnt ||
        !file->mnt->ops)
        return -2;

    if (!file->mnt->ops->read)
        return -3;

    return file->mnt->ops->read(
        file,
        buffer,
        count,
        offset
    );
}


/*
 * ============================================================
 * VFS WRITE
 * ============================================================
 */
ssize_t vfs_write(
    vnode_t *file,
    const void *buffer,
    size_t count,
    uint64_t offset
) {
    if (!file || !buffer)
        return -1;

    if (!file->mnt ||
        !file->mnt->ops)
        return -2;

    if (!file->mnt->ops->write)
        return -3;

    return file->mnt->ops->write(
        file,
        buffer,
        count,
        offset
    );
}