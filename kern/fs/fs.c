#include "fs.h"
#include "block_device.h"
#include "kmalloc.h"
#include "kprintf.h"
#include "lib.h"

#define FS_MAGIC 0xDEADBEEF

static superblock_t current_superblock;
static uint8_t* inode_bitmap = 0;
static uint8_t* block_bitmap = 0;

static int read_block(uint32_t block_num, uint8_t* buffer) {
    return block_device_read(block_num, 1, buffer);
}

static int write_block(uint32_t block_num, const uint8_t* buffer) {
    return block_device_write(block_num, 1, buffer);
}

static inode_t* get_inode(uint32_t inode_id) {
    if (inode_id == 0 || inode_id > current_superblock.total_inodes) return 0;

    uint32_t block_offset = (inode_id - 1) / (FS_BLOCK_SIZE / sizeof(inode_t));
    uint32_t inode_offset_in_block = (inode_id - 1) % (FS_BLOCK_SIZE / sizeof(inode_t));
    uint32_t inode_block_num = current_superblock.inode_table_start_block + block_offset;

    uint8_t block_buffer[FS_BLOCK_SIZE];
    if (read_block(inode_block_num, block_buffer) != 0) {
        return 0;
    }

    inode_t* inode = kmalloc(sizeof(inode_t));
    if (inode) {
        memcpy(inode, &((inode_t*)block_buffer)[inode_offset_in_block], sizeof(inode_t));
    }
    return inode;
}

static int write_inode(uint32_t inode_id, const inode_t* inode) {
    if (inode_id == 0 || inode_id > current_superblock.total_inodes) return -1;

    uint32_t block_offset = (inode_id - 1) / (FS_BLOCK_SIZE / sizeof(inode_t));
    uint32_t inode_offset_in_block = (inode_id - 1) % (FS_BLOCK_SIZE / sizeof(inode_t));
    uint32_t inode_block_num = current_superblock.inode_table_start_block + block_offset;

    uint8_t block_buffer[FS_BLOCK_SIZE];
    if (read_block(inode_block_num, block_buffer) != 0) {
        return -1;
    }

    memcpy(&((inode_t*)block_buffer)[inode_offset_in_block], inode, sizeof(inode_t));

    return write_block(inode_block_num, block_buffer);
}

static uint32_t allocate_inode() {
    for (uint32_t i = 0; i < current_superblock.total_inodes; i++) {
        if (!((inode_bitmap[i / 8] >> (i % 8)) & 1)) {
            inode_bitmap[i / 8] |= (1 << (i % 8));
            current_superblock.free_inodes--;
            write_block(current_superblock.inode_bitmap_block, inode_bitmap);
            return i + 1;
        }
    }
    return 0;
}

static void free_inode(uint32_t inode_id) {
    if (inode_id == 0 || inode_id > current_superblock.total_inodes) return;
    inode_bitmap[(inode_id - 1) / 8] &= ~(1 << ((inode_id - 1) % 8));
    current_superblock.free_inodes++;
    write_block(current_superblock.inode_bitmap_block, inode_bitmap);
}

static uint32_t allocate_block() {
    for (uint32_t i = 0; i < current_superblock.total_blocks; i++) {
        if (!((block_bitmap[i / 8] >> (i % 8)) & 1)) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            current_superblock.free_blocks--;
            write_block(current_superblock.block_bitmap_block, block_bitmap);
            return i + 1;
        }
    }
    return 0;
}

static void free_block(uint32_t block_num) {
    if (block_num == 0 || block_num > current_superblock.total_blocks) return;
    block_bitmap[(block_num - 1) / 8] &= ~(1 << ((block_num - 1) % 8));
    current_superblock.free_blocks++;
    write_block(current_superblock.block_bitmap_block, block_bitmap);
}

void fs_init() {
    uint8_t superblock_buffer[FS_BLOCK_SIZE];
    if (read_block(0, superblock_buffer) != 0) {
        memset(&current_superblock, 0, sizeof(superblock_t));
        current_superblock.magic = FS_MAGIC;
        current_superblock.block_size = FS_BLOCK_SIZE;
        current_superblock.total_blocks = 1024;
        current_superblock.total_inodes = 128;

        uint32_t inode_bitmap_blocks = (current_superblock.total_inodes / 8 + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        uint32_t block_bitmap_blocks = (current_superblock.total_blocks / 8 + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        uint32_t inode_table_blocks = (current_superblock.total_inodes * sizeof(inode_t) + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;

        current_superblock.inode_bitmap_block = 1;
        current_superblock.block_bitmap_block = current_superblock.inode_bitmap_block + inode_bitmap_blocks;
        current_superblock.inode_table_start_block = current_superblock.block_bitmap_block + block_bitmap_blocks;
        current_superblock.data_start_block = current_superblock.inode_table_start_block + inode_table_blocks;

        current_superblock.free_inodes = current_superblock.total_inodes;
        current_superblock.free_blocks = current_superblock.total_blocks - current_superblock.data_start_block;

        memcpy(superblock_buffer, &current_superblock, sizeof(superblock_t));
        write_block(0, superblock_buffer);

        inode_bitmap = kmalloc(inode_bitmap_blocks * FS_BLOCK_SIZE);
        block_bitmap = kmalloc(block_bitmap_blocks * FS_BLOCK_SIZE);
        memset(inode_bitmap, 0, inode_bitmap_blocks * FS_BLOCK_SIZE);
        memset(block_bitmap, 0, block_bitmap_blocks * FS_BLOCK_SIZE);

        for (uint32_t i = 0; i <= current_superblock.data_start_block; i++) {
            block_bitmap[i / 8] |= (1 << (i % 8));
            current_superblock.free_blocks--;
        }
        write_block(current_superblock.inode_bitmap_block, inode_bitmap);
        write_block(current_superblock.block_bitmap_block, block_bitmap);

        uint32_t root_inode_id = allocate_inode();
        inode_t root_inode;
        memset(&root_inode, 0, sizeof(inode_t));
        root_inode.id = root_inode_id;
        root_inode.type = FS_INODE_TYPE_DIR;
        root_inode.permissions = 0755;
        root_inode.size = 0;
        root_inode.creation_time = 0;
        root_inode.modification_time = 0;
        write_inode(root_inode_id, &root_inode);
        current_superblock.root_inode = root_inode_id;

        memcpy(superblock_buffer, &current_superblock, sizeof(superblock_t));
        write_block(0, superblock_buffer);
    } else {
        memcpy(&current_superblock, superblock_buffer, sizeof(superblock_t));
        if (current_superblock.magic != FS_MAGIC) {
            return;
        }

        uint32_t inode_bitmap_blocks = (current_superblock.total_inodes / 8 + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        uint32_t block_bitmap_blocks = (current_superblock.total_blocks / 8 + FS_BLOCK_SIZE - 1) / FS_BLOCK_SIZE;
        inode_bitmap = kmalloc(inode_bitmap_blocks * FS_BLOCK_SIZE);
        block_bitmap = kmalloc(block_bitmap_blocks * FS_BLOCK_SIZE);
        read_block(current_superblock.inode_bitmap_block, inode_bitmap);
        read_block(current_superblock.block_bitmap_block, block_bitmap);
    }
}

uint32_t fs_create(uint32_t parent_inode_id, const char* name, uint16_t type) {
    if (strlen(name) > FS_MAX_FILENAME_LEN) return 0;

    inode_t* parent_inode = get_inode(parent_inode_id);
    if (!parent_inode || !(parent_inode->type & FS_INODE_TYPE_DIR)) {
        kfree(parent_inode);
        return 0;
    }

    if (fs_lookup(parent_inode_id, name) != 0) {
        kfree(parent_inode);
        return 0;
    }

    uint32_t new_inode_id = allocate_inode();
    if (new_inode_id == 0) {
        kfree(parent_inode);
        return 0;
    }

    inode_t new_inode;
    memset(&new_inode, 0, sizeof(inode_t));
    new_inode.id = new_inode_id;
    new_inode.type = type;
    new_inode.permissions = 0644;
    new_inode.creation_time = 0;
    new_inode.modification_time = 0;
    write_inode(new_inode_id, &new_inode);

    dir_entry_t new_entry;
    new_entry.inode_id = new_inode_id;
    strncpy(new_entry.name, name, FS_MAX_FILENAME_LEN);
    new_entry.name[FS_MAX_FILENAME_LEN] = '\0';

    uint8_t dir_block_buffer[FS_BLOCK_SIZE];
    int found_spot = 0;
    for (int i = 0; i < 10; i++) {
        if (parent_inode->direct_blocks[i] == 0) {
            parent_inode->direct_blocks[i] = allocate_block();
            if (parent_inode->direct_blocks[i] == 0) break;
            memset(dir_block_buffer, 0, FS_BLOCK_SIZE);
        } else {
            read_block(parent_inode->direct_blocks[i], dir_block_buffer);
        }

        for (uint32_t j = 0; j < FS_BLOCK_SIZE / sizeof(dir_entry_t); j++) {
            dir_entry_t* entry = (dir_entry_t*)&dir_block_buffer[j * sizeof(dir_entry_t)];
            if (entry->inode_id == 0) {
                memcpy(entry, &new_entry, sizeof(dir_entry_t));
                write_block(parent_inode->direct_blocks[i], dir_block_buffer);
                parent_inode->size += sizeof(dir_entry_t);
                write_inode(parent_inode_id, parent_inode);
                found_spot = 1;
                break;
            }
        }
        if (found_spot) break;
    }

    kfree(parent_inode);
    return new_inode_id;
}

int fs_delete(uint32_t parent_inode_id, const char* name) {
    if (strlen(name) > FS_MAX_FILENAME_LEN) return -1;

    inode_t* parent_inode = get_inode(parent_inode_id);
    if (!parent_inode || !(parent_inode->type & FS_INODE_TYPE_DIR)) {
        kfree(parent_inode);
        return -1;
    }

    uint8_t dir_block_buffer[FS_BLOCK_SIZE];
    uint32_t target_inode_id = 0;
    int found_entry = 0;

    for (int i = 0; i < 10; i++) {
        if (parent_inode->direct_blocks[i] == 0) continue;
        read_block(parent_inode->direct_blocks[i], dir_block_buffer);
        for (uint32_t j = 0; j < FS_BLOCK_SIZE / sizeof(dir_entry_t); j++) {
            dir_entry_t* entry = (dir_entry_t*)&dir_block_buffer[j * sizeof(dir_entry_t)];
            if (entry->inode_id != 0 && strcmp(entry->name, name) == 0) {
                target_inode_id = entry->inode_id;
                memset(entry, 0, sizeof(dir_entry_t));
                write_block(parent_inode->direct_blocks[i], dir_block_buffer);
                parent_inode->size -= sizeof(dir_entry_t);
                write_inode(parent_inode_id, parent_inode);
                found_entry = 1;
                break;
            }
        }
        if (found_entry) break;
    }

    kfree(parent_inode);

    if (target_inode_id == 0) return -1;

    inode_t* target_inode = get_inode(target_inode_id);
    if (!target_inode) return -1;

    for (int i = 0; i < 10; i++) {
        if (target_inode->direct_blocks[i] != 0) {
            free_block(target_inode->direct_blocks[i]);
        }
    }

    free_inode(target_inode_id);
    kfree(target_inode);

    return 0;
}

uint32_t fs_lookup(uint32_t parent_inode_id, const char* name) {
    if (strlen(name) > FS_MAX_FILENAME_LEN) return 0;

    inode_t* parent_inode = get_inode(parent_inode_id);
    if (!parent_inode || !(parent_inode->type & FS_INODE_TYPE_DIR)) {
        kfree(parent_inode);
        return 0;
    }

    uint8_t dir_block_buffer[FS_BLOCK_SIZE];
    for (int i = 0; i < 10; i++) {
        if (parent_inode->direct_blocks[i] == 0) continue;
        read_block(parent_inode->direct_blocks[i], dir_block_buffer);
        for (uint32_t j = 0; j < FS_BLOCK_SIZE / sizeof(dir_entry_t); j++) {
            dir_entry_t* entry = (dir_entry_t*)&dir_block_buffer[j * sizeof(dir_entry_t)];
            if (entry->inode_id != 0 && strcmp(entry->name, name) == 0) {
                uint32_t found_id = entry->inode_id;
                kfree(parent_inode);
                return found_id;
            }
        }
    }

    kfree(parent_inode);
    return 0;
}

int fs_read(uint32_t inode_id, uint64_t offset, uint8_t* buffer, uint64_t count) {
    inode_t* inode = get_inode(inode_id);
    if (!inode || !(inode->type & FS_INODE_TYPE_FILE)) {
        kfree(inode);
        return -1;
    }
    if (offset + count > inode->size) {
        count = inode->size - offset;
    }
    if (count == 0) {
        kfree(inode);
        return 0;
    }

    uint64_t bytes_read = 0;
    uint8_t data_block_buffer[FS_BLOCK_SIZE];

    while (bytes_read < count) {
        uint64_t current_file_offset = offset + bytes_read;
        uint32_t block_idx = current_file_offset / FS_BLOCK_SIZE;
        uint32_t block_offset = current_file_offset % FS_BLOCK_SIZE;

        uint32_t data_block_num = 0;
        if (block_idx < 10) {
            data_block_num = inode->direct_blocks[block_idx];
        } else {
            break;
        }

        if (data_block_num == 0) {
            break;
        }

        if (read_block(data_block_num, data_block_buffer) != 0) {
            kfree(inode);
            return -1;
        }

        uint64_t bytes_to_copy = FS_BLOCK_SIZE - block_offset;
        if (bytes_to_copy > count - bytes_read) {
            bytes_to_copy = count - bytes_read;
        }
        memcpy(buffer + bytes_read, data_block_buffer + block_offset, bytes_to_copy);
        bytes_read += bytes_to_copy;
    }

    kfree(inode);
    return bytes_read;
}

int fs_write(uint32_t inode_id, uint64_t offset, const uint8_t* buffer, uint64_t count) {
    inode_t* inode = get_inode(inode_id);
    if (!inode || !(inode->type & FS_INODE_TYPE_FILE)) {
        kfree(inode);
        return -1;
    }

    uint64_t bytes_written = 0;
    uint8_t data_block_buffer[FS_BLOCK_SIZE];

    while (bytes_written < count) {
        uint64_t current_file_offset = offset + bytes_written;
        uint32_t block_idx = current_file_offset / FS_BLOCK_SIZE;
        uint32_t block_offset = current_file_offset % FS_BLOCK_SIZE;

        uint32_t data_block_num = 0;
        if (block_idx < 10) {
            data_block_num = inode->direct_blocks[block_idx];
            if (data_block_num == 0) {
                data_block_num = allocate_block();
                if (data_block_num == 0) {
                    break;
                }
                inode->direct_blocks[block_idx] = data_block_num;
                write_inode(inode_id, inode);
            }
        } else {
            break;
        }

        if (read_block(data_block_num, data_block_buffer) != 0) {
            kfree(inode);
            return -1;
        }

        uint64_t bytes_to_copy = FS_BLOCK_SIZE - block_offset;
        if (bytes_to_copy > count - bytes_written) {
            bytes_to_copy = count - bytes_written;
        }
        memcpy(data_block_buffer + block_offset, buffer + bytes_written, bytes_to_copy);
        write_block(data_block_num, data_block_buffer);
        bytes_written += bytes_to_copy;
    }

    if (offset + bytes_written > inode->size) {
        inode->size = offset + bytes_written;
        write_inode(inode_id, inode);
    }

    kfree(inode);
    return bytes_written;
}


