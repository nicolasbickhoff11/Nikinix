#ifndef _FS_RAMFS_H
#define _FS_RAMFS_H 1

#include <stdint.h>
#include <stddef.h>

typedef struct ramfs_node ramfs_node_t;

struct ramfs_node {
    char *name;
    uint32_t mode;
    uint64_t inode;
    uint8_t *data;
    size_t current_data_size;
    ramfs_node_t **children;
    size_t children_count;
    ramfs_node_t *parent;
    ramfs_node_t *next;

    void *private;
};

#endif