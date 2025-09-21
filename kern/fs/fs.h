#ifndef FS_H
#define FS_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

// filesystem constants
#define FS_MAX_FILENAME_LEN 28
#define FS_MAX_FILES_PER_DIR 64
#define FS_BLOCK_SIZE 4096 // same as UFS block size for simplicity

// inode types
#define FS_INODE_TYPE_FILE 0x8000
#define FS_INODE_TYPE_DIR  0x4000

// inode structure
typedef struct {
    uint32_t id;
    uint16_t type;
    uint16_t permissions;
    uint32_t owner_id;
    uint32_t group_id;
    uint64_t size;
    uint64_t creation_time;
    uint64_t modification_time;
    uint32_t direct_blocks[10]; // direct block pointers
    uint32_t indirect_block;
    uint32_t double_indirect_block;
} inode_t;

// directory entry structure
typedef struct {
    uint32_t inode_id;
    char name[FS_MAX_FILENAME_LEN + 1];
} dir_entry_t;

// superblock structure
typedef struct {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t free_blocks;
    uint32_t total_inodes;
    uint32_t free_inodes;
    uint32_t root_inode;
    uint32_t inode_bitmap_block;
    uint32_t block_bitmap_block;
    uint32_t inode_table_start_block;
    uint32_t data_start_block;
} superblock_t;

// function prototypes
void fs_init();
uint32_t fs_create(uint32_t parent_inode_id, const char* name, uint16_t type);
int fs_delete(uint32_t parent_inode_id, const char* name);
uint32_t fs_lookup(uint32_t parent_inode_id, const char* name);
int fs_read(uint32_t inode_id, uint64_t offset, uint8_t* buffer, uint64_t count);
int fs_write(uint32_t inode_id, uint64_t offset, const uint8_t* buffer, uint64_t count);

#endif // FS_H


