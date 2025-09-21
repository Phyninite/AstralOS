#include "vfs.h"
#include "fs.h"
#include "kmalloc.h"
#include "kprintf.h"
#include "lib.h"
#include <string.h>

// global pointer to the root node of the virtual filesystem
static vfs_node_t *root = 0;

// initialize the vfs by creating the root directory
// this function allocates memory for the root node and sets its initial values
int vfs_init(void) {
    root = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!root) {
        kprintf("vfs_init: failed to allocate memory for root\n");
        return -1;
    }
    root->name = "root";
    root->type = VFS_NODE_DIR;    // mark as directory
    root->parent = 0;             // root has no parent
    root->child = 0;              // no children at initialization
    root->sibling = 0;            // no siblings for root
    root->fs_data = 0;            // no filesystem-specific data by default
    return 0;
}

// return the root node of the vfs
vfs_node_t* vfs_get_root(void) {
    return root;
}

// create a new directory node under the specified parent directory
// this function allocates memory for the new directory, initializes its fields,
// and adds it to the parent's children linked list
vfs_node_t* vfs_create_dir(vfs_node_t *parent, const char *name) {
    if (!parent || parent->type != VFS_NODE_DIR) {
        kprintf("vfs_create_dir: invalid parent\n");
        return 0;
    }
    vfs_node_t *node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!node) {
        kprintf("vfs_create_dir: memory allocation failed\n");
        return 0;
    }
    node->name = (char*)name;
    node->type = VFS_NODE_DIR;
    node->parent = parent;
    node->child = 0;
    node->sibling = 0;
    node->fs_data = 0;
    
    // add the new directory as the last child of the parent
    if (!parent->child) {
        parent->child = node;
    } else {
        vfs_node_t *cur = parent->child;
        while (cur->sibling)
            cur = cur->sibling;
        cur->sibling = node;
    }
    return node;
}

// create a new file node under the specified parent directory
// the fs_data field can be used by the filesystem layer to store file-specific data
vfs_node_t* vfs_create_file(vfs_node_t *parent, const char *name, void *fs_data) {
    if (!parent || parent->type != VFS_NODE_DIR) {
        kprintf("vfs_create_file: invalid parent\n");
        return 0;
    }
    vfs_node_t *node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!node) {
        kprintf("vfs_create_file: memory allocation failed\n");
        return 0;
    }
    node->name = (char*)name;
    node->type = VFS_NODE_FILE;
    node->parent = parent;
    node->child = 0;    // files do not have children
    node->sibling = 0;
    node->fs_data = fs_data;
    
    // add the new file as the last child of the parent directory
    if (!parent->child) {
        parent->child = node;
    } else {
        vfs_node_t *cur = parent->child;
        while (cur->sibling)
            cur = cur->sibling;
        cur->sibling = node;
    }
    return node;
}

// lookup a child node by name under the given directory node
// this function iterates through the children linked list until a match is found
vfs_node_t* vfs_lookup(vfs_node_t *node, const char *name) {
    if (!node || node->type != VFS_NODE_DIR)
        return 0;
    vfs_node_t *cur = node->child;
    while (cur) {
        if (strcmp(cur->name, name) == 0)
            return cur;
        cur = cur->sibling;
    }
    return 0;
}

// mount a filesystem root to the given mount point node
// this function sets the fs_data pointer so that subsequent read/write operations
// can be delegated to the specific filesystem implementation
int vfs_mount(vfs_node_t *mount_point, void *fs_root) {
    if (!mount_point) {
        kprintf("vfs_mount: invalid mount point\n");
        return -1;
    }
    mount_point->fs_data = fs_root;
    return 0;
}

// read from a file node by delegating to the underlying filesystem's read function
int vfs_read(vfs_node_t *node, void *buffer, int size) {
    if (!node || !node->fs_data) {
        kprintf("vfs_read: invalid node or missing filesystem data\n");
        return -1;
    }
    return fs_read(node->fs_data, buffer, size);
}

// write to a file node by delegating to the underlying filesystem's write function
int vfs_write(vfs_node_t *node, const void *buffer, int size) {
    if (!node || !node->fs_data) {
        kprintf("vfs_write: invalid node or missing filesystem data\n");
        return -1;
    }
    return fs_write(node->fs_data, buffer, size);
}
