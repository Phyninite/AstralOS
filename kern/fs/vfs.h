#ifndef VFS_H
#define VFS_H

struct vfs_node;

int vfs_init(void);
struct vfs_node* vfs_lookup(struct vfs_node *node, const char *name);
int vfs_mount(struct vfs_node *mount_point, void *fs_root);
struct vfs_node* vfs_create_node(struct vfs_node *parent, const char *name, int type);
int vfs_read(struct vfs_node *node, void *buffer, int size);
int vfs_write(struct vfs_node *node, const void *buffer, int size);

#endif
