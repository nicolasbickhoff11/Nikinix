#ifndef _VFS_VFS_H
#define _VFS_VFS_H 1

#include <stdint.h>
#include <stddef.h>

#define S_IFMT   0170000  

#define S_IFSOCK 0140000 
#define S_IFLNK  0120000 
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000

#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)

typedef int64_t ssize_t;

typedef struct node_ops node_ops_t;
typedef struct mount mount_t;
typedef struct vnode vnode_t;

typedef struct dirent {
    uint64_t inode;
    uint8_t type;
    char name[256];
} dirent_t;

struct node_ops {
    int (*lookup)(vnode_t *, const char *, vnode_t **);
    int (*create)(vnode_t *, const char *, uint32_t, vnode_t **);
    int (*remove)(vnode_t *, const char *);
    ssize_t (*read)(vnode_t *, void *, size_t, uint64_t);
    ssize_t (*write)(vnode_t *, const void *, size_t, uint64_t);
    ssize_t (*readdir)(vnode_t *dir, dirent_t *entries, size_t max_entries);
};

struct mount {
    vnode_t *root;
    void *fs_private;
    node_ops_t *ops;
    uint64_t inode_count;
};


struct vnode {
    uint32_t mode;
    uint64_t inode;

    struct vnode *parent;
    mount_t *mnt;

    void *private;
};

typedef uint32_t mode_t;

extern vnode_t *vfs_root;

void vfs_init();
int vfs_mount(const char *path, mount_t *mnt);
int vfs_lookup(vnode_t *dir, const char *name, vnode_t **result);
int vfs_create(vnode_t *dir, const char *name, uint32_t mode, vnode_t **result);
int vfs_remove(vnode_t *dir, const char *name);
ssize_t vfs_read(vnode_t *file, void *buffer, size_t count, uint64_t offset);
ssize_t vfs_write(vnode_t *file, const void *buffer, size_t count, uint64_t offset);
int vfs_create_path(vnode_t *root, const char *path, mode_t mode, vnode_t **result);
int vfs_lookup_path(vnode_t *root, const char *path, vnode_t **result);
int vfs_remove_path(vnode_t *root, const char *path);
int vfs_open(const char *name, vnode_t **result);
#endif