#include "vfs.h"
#include "fs.h"
#include "kmalloc.h"
#include "kprintf.h"
#include "lib.h"
#include <string.h>

struct vfs_node {
    char *name;
    int type;
    struct vfs_node *parent;
    struct vfs_node *children;
    int children_count;
    void *fs_data;
};

static struct vfs_node *root;

int vfs_init(void) {
    root = (struct vfs_node*)kmalloc(sizeof(struct vfs_node));
    if (!root)
        return -1;
    root->name = "root";
    root->type = 0;
    root->parent = 0;
    root->children = 0;
    root->children_count = 0;
    root->fs_data = 0;
    return 0;
}

struct vfs_node* vfs_lookup(struct vfs_node *node, const char *name) {
    if (!node || !node->children)
        return 0;
    for (int i = 0; i < node->children_count; i++) {
        if (strcmp((node->children + i)->name, name) == 0)
            return (node->children + i);
    }
    return 0;
}

int vfs_mount(struct vfs_node *mount_point, void *fs_root) {
    if (!mount_point)
        return -1;
    mount_point->fs_data = fs_root;
    return 0;
}

struct vfs_node* vfs_create_node(struct vfs_node *parent, const char *name, int type) {
    if (!parent)
        return 0;
    struct vfs_node *node = (struct vfs_node*)kmalloc(sizeof(struct vfs_node));
    if (!node)
        return 0;
    node->name = (char*)name;
    node->type = type;
    node->parent = parent;
    node->children = 0;
    node->children_count = 0;
    node->fs_data = 0;
    struct vfs_node *new_children = (struct vfs_node*)kmalloc(sizeof(struct vfs_node) * (parent->children_count + 1));
    if (!new_children)
        return 0;
    if (parent->children_count > 0 && parent->children) {
        memcpy(new_children, parent->children, sizeof(struct vfs_node) * parent->children_count);
    }
    new_children[parent->children_count] = *node;
    parent->children = new_children;
    parent->children_count++;
    return node;
}

int vfs_read(struct vfs_node *node, void *buffer, int size) {
    if (!node || !node->fs_data)
        return -1;
    return fs_read(node->fs_data, buffer, size);
}

int vfs_write(struct vfs_node *node, const void *buffer, int size) {
    if (!node || !node->fs_data)
        return -1;
    return fs_write(node->fs_data, buffer, size);
}
