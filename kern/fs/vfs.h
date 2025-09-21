#ifndef VFS_H
#define VFS_H

// define node types for vfs
#define VFS_NODE_DIR  1
#define VFS_NODE_FILE 2

// vfs_node_t structure represents a file or directory node in the virtual filesystem
// this implementation uses a linked list for storing children of a directory
typedef struct vfs_node {
    char *name;                // node name
    int type;                  // node type (directory or file)
    struct vfs_node *parent;   // pointer to parent node, null for root
    struct vfs_node *child;    // pointer to first child node
    struct vfs_node *sibling;  // pointer to next sibling in the child list
    void *fs_data;             // pointer to filesystem-specific data
} vfs_node_t;

// initialize the vfs and create the root directory
int vfs_init(void);

// get the root node of the vfs
vfs_node_t* vfs_get_root(void);

// create a new directory under the specified parent
vfs_node_t* vfs_create_dir(vfs_node_t *parent, const char *name);

// create a new file under the specified parent
vfs_node_t* vfs_create_file(vfs_node_t *parent, const char *name, void *fs_data);

// lookup a child node within a directory by name
vfs_node_t* vfs_lookup(vfs_node_t *node, const char *name);

// mount a filesystem root to the given mount point
int vfs_mount(vfs_node_t *mount_point, void *fs_root);

// read from a file node; this delegates to the fs layer
int vfs_read(vfs_node_t *node, void *buffer, int size);

// write to a file node; this delegates to the fs layer
int vfs_write(vfs_node_t *node, const void *buffer, int size);

#endif
