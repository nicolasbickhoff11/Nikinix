#include "fs/ramfs/ramfs.h"
#include "vfs/vfs.h"
#include "malloc.h"
#include "string.h"
#include "print.h"

int ramfs_lookup(vnode_t *dir, const char *name, vnode_t **result) {
    if (!dir || !name || !result) return -1;
    *result = NULL;

    if (!S_ISDIR(dir->mode)) return -2;

    ramfs_node_t *node = dir->private;
    if (!node) return -3;

    for (size_t i = 0; i < node->children_count; i++) {
        ramfs_node_t *entry = node->children[i];

        if (!entry || !entry->name)
            continue;

        if (strcmp(name, entry->name) == 0) {
            vnode_t *res = malloc(sizeof(vnode_t));
            if (!res)
                return -4;

            res->mode = entry->mode;
            res->inode = entry->inode;
            res->mnt = dir->mnt;
            res->private = entry;

            *result = res;
            return 0;
        }
    }

    return -5;
}

int ramfs_create(vnode_t *dir, const char *name, uint32_t mode, vnode_t **result) {
    if (!dir || !name || !result) return -1;
    *result = NULL;

    if (!S_ISDIR(dir->mode)) return -2;

    ramfs_node_t *node = dir->private;
    if (!node) return -3;

    ramfs_node_t *new_entry = malloc(sizeof(ramfs_node_t));
    if (!new_entry) return -4;

    new_entry->name = strdup(name);
    if (!new_entry->name) {
        free(new_entry);
        return -5;
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
        (node->children_count + 1) * sizeof(*node->children)
    );

    if (!tmp) {
        free(new_entry->name);
        free(new_entry);
        return -6;
    }

    node->children = tmp;
    node->children[node->children_count] = new_entry;
    node->children_count++;

    vnode_t *res = malloc(sizeof(vnode_t));
    if (!res) {
        return -7;
    }

    res->mode = new_entry->mode;
    res->inode = new_entry->inode;
    res->mnt = dir->mnt;
    res->private = new_entry;
    res->parent = dir->parent;

    *result = res;

    return 0;
}

int ramfs_remove(vnode_t *dir, const char *name) {
    if (!dir || !name) return -1;

    if (!S_ISDIR(dir->mode)) return -2;

    ramfs_node_t *node = dir->private;
    if (!node) return -3;

    for (size_t i = 0; i < node->children_count; i++) {
        ramfs_node_t *entry = node->children[i];

        if (!entry)
            continue;

        if (strcmp(entry->name, name) == 0) {
            free(entry->data);
            free(entry->name);
            free(entry);

            node->children[i] = NULL;

            return 0;
        }
    }

    return -4;
}

ssize_t ramfs_read(vnode_t *file, void *buffer, size_t count, uint64_t offset) {
    if (!file || !buffer) return -1;

    ramfs_node_t *node = file->private;

    if (!node) return -2;
    if (!S_ISREG(file->mode)) return -3;

    if (count == 0)
        return 0;

    if (offset >= node->current_data_size)
        return 0;

    uint64_t size = node->current_data_size - offset;

    if (size > count)
        size = count;

    memcpy(buffer, node->data + offset, size);

    return size;
}

ssize_t ramfs_write(vnode_t *file, const void *buffer, size_t count, uint64_t offset) {
    if (!file || !buffer) return -1;

    ramfs_node_t *node = file->private;

    if (!node) return -2;
    if (!S_ISREG(file->mode)) return -3;

    if (count == 0)
        return 0;

    uint64_t new_size = offset + count;

    if (new_size < offset)
        return -4;

    if (new_size > node->current_data_size) {
        uint64_t old_size = node->current_data_size;

        void *new_data = realloc(node->data, new_size);
        if (!new_data)
            return -5;

        node->data = new_data;

        if (new_size > old_size) {
            memset(
                node->data + old_size,
                0,
                new_size - old_size
            );
        }

        node->current_data_size = new_size;
    }

    memcpy(node->data + offset, buffer, count);

    return count;
}

node_ops_t ramfs_ops = {
    .create = ramfs_create,
    .lookup = ramfs_lookup,
    .remove = ramfs_remove,
    .read = ramfs_read,
    .write = ramfs_write
};

mount_t *create_ramfs(void) {
    ramfs_node_t *root = malloc(sizeof(ramfs_node_t));
    if (!root)
        return NULL;

    root->name = strdup("/");
    if (!root->name) {
        free(root);
        return NULL;
    }

    root->mode = S_IFDIR | 0777;
    root->inode = 1;
    root->data = NULL;
    root->current_data_size = 0;
    root->children = NULL;
    root->children_count = 0;
    root->parent = NULL;
    root->next = NULL;

    vnode_t *root_vfs = malloc(sizeof(vnode_t));
    if (!root_vfs) {
        free(root->name);
        free(root);
        return NULL;
    }

    root_vfs->mode = root->mode;
    root_vfs->inode = root->inode;
    root_vfs->mnt = NULL;
    root_vfs->private = root;
    root_vfs->parent = NULL;

    mount_t *mount = malloc(sizeof(mount_t));
    if (!mount) {
        free(root_vfs);
        free(root->name);
        free(root);
        return NULL;
    }

    mount->root = root_vfs;
    mount->fs_private = NULL;
    mount->ops = &ramfs_ops;
    mount->inode_count = 1;

    root_vfs->mnt = mount;

    return mount;
}