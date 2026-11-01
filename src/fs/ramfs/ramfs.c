#include "fs/ramfs/ramfs.h"
#include "user/sched/sched.h"
#include "vfs/vfs.h"
#include "malloc.h"
#include "string.h"
#include "print.h"
#include "errno.h"


int ramfs_lookup(
    vnode_t *dir,
    const char *name,
    vnode_t **result
) {
    if (!dir || !name || !result)
        return -EFAULT;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    ramfs_node_t *node = dir->private;

    if (!node)
        return -EIO;

    for (size_t i = 0;
         i < node->children_count;
         i++) {

        ramfs_node_t *entry = node->children[i];

        if (!entry || !entry->name)
            continue;

        if (strcmp(name, entry->name) == 0) {

            vnode_t *res = malloc(sizeof(vnode_t));

            if (!res)
                return -ENOMEM;

            res->mode = entry->mode;
            res->name = entry->name;
            res->inode = entry->inode;
            res->mnt = dir->mnt;
            res->private = entry;

            /*
             * O vnode encontrado pertence ao diretório
             * que realizou o lookup.
             */
            res->parent = dir;

            *result = res;

            return 0;
        }
    }

    return -ENOENT;
}


ssize_t ramfs_readdir(
    vnode_t *dir,
    dirent_t *entries,
    size_t max_entries
) {
    if (!dir || !entries)
        return -EFAULT;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    ramfs_node_t *node = dir->private;

    if (!node)
        return -EIO;

    size_t count = 0;

    for (size_t i = 0;
         i < node->children_count &&
         count < max_entries;
         i++) {

        ramfs_node_t *entry = node->children[i];

        if (!entry || !entry->name)
            continue;

        entries[count].inode = entry->inode;
        entries[count].type = entry->mode;

        memcpy(
            entries[count].name,
            entry->name,
            sizeof(entries[count].name) - 1
        );

        entries[count].name[
            sizeof(entries[count].name) - 1
        ] = '\0';

        count++;
    }

    return (ssize_t)count;
}


int ramfs_create(
    vnode_t *dir,
    const char *name,
    uint32_t mode,
    vnode_t **result
) {
    if (!dir || !name || !result)
        return -EFAULT;

    *result = NULL;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    ramfs_node_t *node = dir->private;

    if (!node)
        return -EIO;

    /*
     * Evita criar dois objetos com o mesmo nome.
     */
    for (size_t i = 0;
         i < node->children_count;
         i++) {

        ramfs_node_t *entry = node->children[i];

        if (!entry || !entry->name)
            continue;

        if (strcmp(name, entry->name) == 0)
            return -EEXIST;
    }

    ramfs_node_t *new_entry =
        malloc(sizeof(ramfs_node_t));

    if (!new_entry)
        return -ENOMEM;

    new_entry->name = strdup(name);

    if (!new_entry->name) {
        free(new_entry);
        return -ENOMEM;
    }

    new_entry->mode = mode;
    new_entry->inode = ++dir->mnt->inode_count;
    new_entry->data = NULL;
    new_entry->current_data_size = 0;
    new_entry->children = NULL;
    new_entry->children_count = 0;
    new_entry->parent = node;
    new_entry->next = NULL;

    ramfs_node_t **tmp = realloc(
        node->children,
        (node->children_count + 1) *
        sizeof(*node->children)
    );

    if (!tmp) {
        free(new_entry->name);
        free(new_entry);
        return -ENOMEM;
    }

    node->children = tmp;

    node->children[
        node->children_count
    ] = new_entry;

    node->children_count++;

    vnode_t *res = malloc(sizeof(vnode_t));

    if (!res) {
        /*
         * O objeto já foi inserido no diretório.
         * Por enquanto mantemos a entrada.
         */
        return -ENOMEM;
    }

    res->mode = new_entry->mode;
    res->inode = new_entry->inode;
    res->mnt = dir->mnt;
    res->private = new_entry;
    res->name = new_entry->name;

    /*
     * O pai é o diretório onde o objeto foi criado.
     */
    res->parent = dir;

    *result = res;

    return 0;
}


int ramfs_remove(
    vnode_t *dir,
    const char *name
) {
    if (!dir || !name)
        return -EFAULT;

    if (!S_ISDIR(dir->mode))
        return -ENOTDIR;

    ramfs_node_t *node = dir->private;

    if (!node)
        return -EIO;

    for (size_t i = 0;
         i < node->children_count;
         i++) {

        ramfs_node_t *entry = node->children[i];

        if (!entry)
            continue;

        if (strcmp(entry->name, name) == 0) {

            /*
             * Não permite remover diretório
             * que ainda possui filhos.
             */
            if (S_ISDIR(entry->mode) &&
                entry->children_count > 0)
                return -EBUSY;

            free(entry->data);
            free(entry->name);
            free(entry);

            node->children[i] = NULL;

            return 0;
        }
    }

    return -ENOENT;
}


ssize_t ramfs_read(
    vnode_t *file,
    void *buffer,
    size_t count,
    uint64_t offset
) {
    if (!file || !buffer)
        return -EFAULT;

    ramfs_node_t *node = file->private;

    if (!node)
        return -EIO;

    if (!S_ISREG(file->mode))
        return -EINVAL;

    if (count == 0)
        return 0;

    if (offset >= node->current_data_size)
        return 0;

    uint64_t size =
        node->current_data_size - offset;

    if (size > count)
        size = count;

    memcpy(
        buffer,
        node->data + offset,
        size
    );

    return (ssize_t)size;
}


ssize_t ramfs_write(
    vnode_t *file,
    const void *buffer,
    size_t count,
    uint64_t offset
) {
    if (!file || !buffer)
        return -EFAULT;

    ramfs_node_t *node = file->private;

    if (!node)
        return -EIO;

    if (!S_ISREG(file->mode))
        return -EINVAL;

    if (count == 0)
        return 0;

    /*
     * Verifica overflow de offset + count.
     */
    if (offset > UINT64_MAX - count)
        return -EOVERFLOW;

    uint64_t new_size = offset + count;

    if (new_size > node->current_data_size) {

        uint64_t old_size =
            node->current_data_size;

        void *new_data =
            realloc(node->data, new_size);

        if (!new_data)
            return -ENOMEM;

        node->data = new_data;

        if (new_size > old_size) {

            memset(
                (uint8_t *)node->data + old_size,
                0,
                new_size - old_size
            );
        }

        node->current_data_size = new_size;
    }

    memcpy(
        (uint8_t *)node->data + offset,
        buffer,
        count
    );

    return (ssize_t)count;
}

vnode_t *ramfs_resolve(const char *path) {
    if (!path || !*path || !vfs_root)
        return NULL;

    /*
     * Caminhos absolutos começam em vfs_root.
     * Caminhos relativos começam no CWD do processo;
     * se ainda não houver CWD, começam em vfs_root.
     */
    vnode_t *walk;

    if (path[0] == '/') {
        walk = vfs_root;
    } else {
        walk = (current && current->current_cwd.dir)
             ? current->current_cwd.dir
             : vfs_root;
    }

    while (*path) {
        while (*path == '/')
            path++;

        if (!*path)
            break;

        char component[256];
        size_t len = 0;

        while (*path && *path != '/') {
            if (len >= sizeof(component) - 1)
                return NULL;

            component[len++] = *path++;
        }

        component[len] = '\0';

        if (!strcmp(component, "."))
            continue;

        if (!strcmp(component, "..")) {
            if (walk->parent)
                walk = walk->parent;
            else
                walk = vfs_root;

            continue;
        }

        if (!S_ISDIR(walk->mode))
            return NULL;

        vnode_t *next = NULL;
        int ret = ramfs_lookup(walk, component, &next);

        if (ret < 0 || !next)
            return NULL;

        walk = next;
    }

    return walk;
}

node_ops_t ramfs_ops = {
    .create  = ramfs_create,
    .lookup  = ramfs_lookup,
    .remove  = ramfs_remove,
    .read    = ramfs_read,
    .write   = ramfs_write,
    .readdir = ramfs_readdir,
    .resolve = ramfs_resolve
};


mount_t *create_ramfs(void)
{
    ramfs_node_t *root_node =
        malloc(sizeof(ramfs_node_t));

    if (!root_node)
        return NULL;

    root_node->name = strdup("/");

    if (!root_node->name) {
        free(root_node);
        return NULL;
    }

    root_node->mode = S_IFDIR | 0777;
    root_node->inode = 1;
    root_node->data = NULL;
    root_node->current_data_size = 0;
    root_node->children = NULL;
    root_node->children_count = 0;
    root_node->parent = NULL;
    root_node->next = NULL;

    vnode_t *root_vfs =
        malloc(sizeof(vnode_t));

    if (!root_vfs) {
        free(root_node->name);
        free(root_node);
        return NULL;
    }

    root_vfs->mode = root_node->mode;
    root_vfs->inode = root_node->inode;
    root_vfs->mnt = NULL;
    root_vfs->private = root_node;
    root_vfs->parent = NULL;
    root_vfs->name = root_node->name;

    mount_t *mount =
        malloc(sizeof(mount_t));

    if (!mount) {
        free(root_vfs);
        free(root_node->name);
        free(root_node);
        return NULL;
    }

    mount->root = root_vfs;
    mount->fs_private = NULL;
    mount->ops = &ramfs_ops;
    mount->inode_count = 1;

    root_vfs->mnt = mount;

    return mount;
}