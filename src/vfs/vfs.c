#include <stdint.h>
#include <string.h>
#include "vfs/vfs.h"
#include "print.h"
#include "pmm.h"
#include "errno.h"

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
    vnode_t *start,
    const char *path,
    vnode_t **result
) {
    if (!start || !path || !result)
        return -EFAULT;

    *result = NULL;

    vnode_t *current = start;

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
                return -ENAMETOOLONG;

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
            return -ENOTDIR;

        /*
         * Verifica se o mount possui lookup.
         */
        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->lookup)
            return -ENOSYS;

        vnode_t *next = NULL;

        int ret = current->mnt->ops->lookup(
            current,
            name,
            &next
        );

        if (ret < 0)
            return ret;

        if (!next)
            return -ENOENT;

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
 */
int vfs_create_path(
    vnode_t *start,
    const char *path,
    mode_t mode,
    vnode_t **result
) {
    if (!start || !path || !result)
        return -EFAULT;

    *result = NULL;

    vnode_t *current = start;

    /*
     * Caminho absoluto.
     */
    if (*path == '/')
        path++;

    /*
     * Caminho vazio -> vnode inicial.
     *
     * Isso também cobre "/".
     */
    if (!*path) {
        *result = start;
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
                return -ENAMETOOLONG;

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
            return -ENOTDIR;

        /*
         * O filesystem precisa suportar lookup.
         */
        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->lookup)
            return -ENOSYS;

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
                    return -ENOTDIR;
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
         * Se o lookup retornou um erro diferente de
         * "não encontrado", propaga o erro.
         */
        if (ret < 0 && ret != -ENOENT)
            return ret;

        /*
         * Não encontrou.
         *
         * Agora precisamos criar.
         */
        if (!current->mnt ||
            !current->mnt->ops ||
            !current->mnt->ops->create)
            return -ENOSYS;

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
                return ret;

            if (!next)
                return -EIO;

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
            return ret;

        if (!next)
            return -EIO;

        /*
         * Devolve o vnode criado.
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

#include "user/sched/sched.h"

/*
 * ============================================================
 * VFS CREATE USER PATH
 * ============================================================
 *
 * Cria um objeto usando o CWD do processo atual.
 *
 * Caminhos relativos:
 *   teste.txt
 *   pasta/teste.txt
 *
 * Caminhos absolutos:
 *   /teste.txt
 *   /tmp/teste.txt
 */
int vfs_create_user_path(
    const char *path,
    mode_t mode,
    vnode_t **result
) {
    if (!path || !result)
        return -EFAULT;

    *result = NULL;

    if (!vfs_root)
        return -ENODEV;

    vnode_t *start = vfs_root;

    /*
     * Caminhos relativos começam no CWD.
     */
    if (path[0] != '/') {

        if (current && current->current_cwd.dir)
            start = current->current_cwd.dir;
    }

    return vfs_create_path(
        start,
        path,
        mode,
        result
    );
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
    vnode_t *start,
    const char *path
) {
    if (!start || !path)
        return -EFAULT;

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
     * Não permite remover a raiz.
     */
    if (len == 1 && path[0] == '/')
        return -EBUSY;

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
        return -ENAMETOOLONG;

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
            return -ENAMETOOLONG;

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
        start,
        parent_path,
        &parent
    );

    if (ret < 0)
        return ret;

    if (!parent)
        return -ENOENT;

    /*
     * Pai precisa ser diretório.
     */
    if (!S_ISDIR(parent->mode))
        return -ENOTDIR;

    /*
     * Verifica remove().
     */
    if (!parent->mnt ||
        !parent->mnt->ops ||
        !parent->mnt->ops->remove)
        return -ENOSYS;

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
        return -EFAULT;

    /*
     * Mount na raiz.
     */
    if (strcmp(path, "/") == 0) {

        if (vfs_root)
            return -EBUSY;

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
    return -ENOSYS;
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
        return -EFAULT;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -EFAULT;

    if (!dir->mnt->ops->lookup)
        return -ENOSYS;

    return dir->mnt->ops->lookup(
        dir,
        name,
        result
    );
}


/*
 * ============================================================
 * VFS OPEN
 * ============================================================
 */
int vfs_open(
    const char *name,
    vnode_t **result
) {
    return vfs_lookup_path(
        vfs_root,
        name,
        result
    );
}


/*
 * ============================================================
 * VFS READDIR
 * ============================================================
 */
ssize_t vfs_readdir(
    vnode_t *dir,
    dirent_t *entries,
    size_t max_entries
) {
    if (!dir || !entries)
        return -EFAULT;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -EFAULT;

    if (!dir->mnt->ops->readdir)
        return -ENOSYS;

    return dir->mnt->ops->readdir(
        dir,
        entries,
        max_entries
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
        return -EFAULT;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -EFAULT;

    if (!dir->mnt->ops->create)
        return -ENOSYS;

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
        return -EFAULT;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    if (!dir->mnt ||
        !dir->mnt->ops)
        return -EFAULT;

    if (!dir->mnt->ops->remove)
        return -ENOSYS;

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
        return -EFAULT;

    if (!file->mnt ||
        !file->mnt->ops)
        return -EFAULT;

    if (!file->mnt->ops->read)
        return -ENOSYS;

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
        return -EFAULT;

    if (!file->mnt ||
        !file->mnt->ops)
        return -EFAULT;

    if (!file->mnt->ops->write)
        return -ENOSYS;

    return file->mnt->ops->write(
        file,
        buffer,
        count,
        offset
    );
}

/*
 * Resolve um caminho usando o filesystem montado na raiz.
 * O callback do filesystem deve tratar caminhos absolutos e relativos;
 * no RAMFS, os relativos começam em current->current_cwd.dir.
 *
 * Erros são retornados como ponteiros codificados (ERR_PTR).
 */
#ifndef ERR_PTR
#define ERR_PTR(error) ((vnode_t *)(intptr_t)(error))
#endif

#ifndef IS_ERR
#define IS_ERR(pointer) ((uintptr_t)(pointer) >= (uintptr_t)-4095)
#endif

#ifndef PTR_ERR
#define PTR_ERR(pointer) ((int)(intptr_t)(pointer))
#endif

vnode_t *vfs_resolve(const char *path) {
    if (!path || !vfs_root)
        return ERR_PTR(-EFAULT);

    if (!vfs_root->mnt ||
        !vfs_root->mnt->ops ||
        !vfs_root->mnt->ops->resolve)
        return ERR_PTR(-ENOSYS);

    return vfs_root->mnt->ops->resolve(path);
}

int vfs_truncate(vnode_t *node, uint64_t length)
{
    if (!node)
        return -EINVAL;

    if (!S_ISREG(node->mode))
        return -EINVAL;

    if (!node->mnt || !node->mnt->ops)
        return -ENODEV;

    if (!node->mnt->ops->truncate)
        return -ENOSYS;

    return node->mnt->ops->truncate(node, length);
}

/*
 * Reconstrói o caminho absoluto percorrendo os vnodes pais.
 * Implementação genérica: usa vnode->name, sem conhecer o filesystem.
 */
int vnode_get_path(vnode_t *node, char *buffer, size_t size) {
    if (!node || !buffer || !vfs_root)
        return -EINVAL;

    if (size < 2)
        return -ENAMETOOLONG;

    vnode_t *parents[64];
    size_t count = 0;

    while (node && count < (sizeof(parents) / sizeof(parents[0]))) {
        parents[count++] = node;

        if (node == vfs_root)
            break;

        node = node->parent;
    }

    /* A cadeia de pais precisa chegar à raiz do VFS. */
    if (count == 0 || parents[count - 1] != vfs_root)
        return -EINVAL;

    size_t pos = 0;
    buffer[pos++] = '/';

    /* Percorre da raiz até o vnode solicitado. */
    for (size_t i = count - 1; i > 0; i--) {
        const char *name = parents[i - 1]->name;

        if (!name)
            return -EINVAL;

        size_t len = strlen(name);
        size_t separator = (i > 1) ? 1 : 0;

        /* Reserva espaço para nome, possível '/', e '\0'. */
        if (len > size - pos ||
            separator > size - pos - len ||
            pos + len + separator >= size)
            return -ENAMETOOLONG;

        memcpy(buffer + pos, name, len);
        pos += len;

        if (separator)
            buffer[pos++] = '/';
    }

    buffer[pos] = '\0';
    return 0;
}
