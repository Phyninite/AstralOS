#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

#define UFS_BLOCK_SIZE 4096

#define UFS_HCI_BASE_ADDR 0xDEAD0000

#define UFS_HCI_CAPABILITIES            (UFS_HCI_BASE_ADDR + 0x00)
#define UFS_HCI_VERSION                 (UFS_HCI_BASE_ADDR + 0x04)
#define UFS_HCI_CONTROLLER_STATUS       (UFS_HCI_BASE_ADDR + 0x08)
#define UFS_HCI_CONTROLLER_ENABLE       (UFS_HCI_BASE_ADDR + 0x0C)
#define UFS_HCI_CONTROLLER_RESET        (UFS_HCI_BASE_ADDR + 0x10)
#define UFS_HCI_INTERRUPT_STATUS        (UFS_HCI_BASE_ADDR + 0x20)
#define UFS_HCI_INTERRUPT_ENABLE        (UFS_HCI_BASE_ADDR + 0x24)
#define UFS_HCI_UTP_TRANSFER_REQ_INT_EN (UFS_HCI_BASE_ADDR + 0x28)
#define UFS_HCI_UTP_TASK_REQ_INT_EN     (UFS_HCI_BASE_ADDR + 0x2C)
#define UFS_HCI_UTP_TRANSFER_REQ_DOORBELL (UFS_HCI_BASE_ADDR + 0x50)
#define UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_L (UFS_HCI_BASE_ADDR + 0x60)
#define UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_H (UFS_HCI_BASE_ADDR + 0x64)
#define UFS_HCI_UTP_TASK_REQ_LIST_BASE_L (UFS_HCI_BASE_ADDR + 0x68)
#define UFS_HCI_UTP_TASK_REQ_LIST_BASE_H (UFS_HCI_BASE_ADDR + 0x6C)

#define UFS_HCI_UTP_TRANSFER_REQ_COMPL_STATUS (UFS_HCI_BASE_ADDR + 0x30)
#define UFS_HCI_UTP_TASK_REQ_COMPL_STATUS (UFS_HCI_BASE_ADDR + 0x34)

typedef struct {
    uint32_t dword0;
    uint32_t dword1;
    uint32_t dword2;
    uint32_t dword3;
    uint32_t dword4;
    uint32_t dword5;
    uint32_t dword6;
    uint32_t dword7;
} utp_trd_t;

typedef struct {
    uint32_t dword0;
    uint32_t dword1;
    uint32_t dword2;
    uint32_t dword3;
} prdt_entry_t;

void ufs_init();
int ufs_read_blocks(uint64_t lba, uint32_t num_blocks, uint8_t* buffer);
int ufs_write_blocks(uint64_t lba, uint32_t num_blocks, const uint8_t* buffer);

#define EMMC_BLOCK_SIZE 512

#define EMMC_HCI_BASE_ADDR 0xBEEF0000

#define EMMC_HCI_ARGUMENT       (EMMC_HCI_BASE_ADDR + 0x00)
#define EMMC_HCI_COMMAND        (EMMC_HCI_BASE_ADDR + 0x04)
#define EMMC_HCI_RESPONSE       (EMMC_HCI_BASE_ADDR + 0x08)
#define EMMC_HCI_DATA           (EMMC_HCI_BASE_ADDR + 0x0C)
#define EMMC_HCI_STATUS         (EMMC_HCI_BASE_ADDR + 0x10)
#define EMMC_HCI_INTERRUPT_STATUS (EMMC_HCI_BASE_ADDR + 0x14)
#define EMMC_HCI_INTERRUPT_ENABLE (EMMC_HCI_BASE_ADDR + 0x18)
#define EMMC_HCI_CONTROL_REG    (EMMC_HCI_BASE_ADDR + 0x20)
#define EMMC_HCI_CAPABILITIES_REG (EMMC_HCI_BASE_ADDR + 0x40)

#define EMMC_CMD_GO_IDLE_STATE  0x00
#define EMMC_CMD_SEND_OP_COND   0x01
#define EMMC_CMD_ALL_SEND_CID   0x02
#define EMMC_CMD_SET_RELATIVE_ADDR 0x03
#define EMMC_CMD_READ_SINGLE_BLOCK 0x11
#define EMMC_CMD_WRITE_SINGLE_BLOCK 0x18
#define EMMC_CMD_READ_MULTIPLE_BLOCK 0x12
#define EMMC_CMD_WRITE_MULTIPLE_BLOCK 0x19
#define EMMC_CMD_STOP_TRANSMISSION  0x0C
#define EMMC_CMD_APP_CMD        0x37
#define EMMC_ACMD_SET_BUS_WIDTH 0x06

#define EMMC_STATUS_CMD_INHIBIT     (1 << 0)
#define EMMC_STATUS_DATA_INHIBIT    (1 << 1)
#define EMMC_STATUS_BUFFER_READ_READY (1 << 11)
#define EMMC_STATUS_BUFFER_WRITE_READY (1 << 12)
#define EMMC_STATUS_TRANSFER_COMPLETE (1 << 1)
#define EMMC_STATUS_COMMAND_COMPLETE (1 << 0)

void emmc_init();
int emmc_read_blocks(uint32_t lba, uint32_t num_blocks, uint8_t* buffer);
int emmc_write_blocks(uint32_t lba, uint32_t num_blocks, const uint8_t* buffer);

typedef enum {
    BLOCK_DEVICE_TYPE_NONE,
    BLOCK_DEVICE_TYPE_UFS,
    BLOCK_DEVICE_TYPE_EMMC
} block_device_type_t;

block_device_type_t get_active_block_device_type();
void set_active_block_device(block_device_type_t type);
int block_device_read(uint64_t lba, uint32_t num_blocks, uint8_t* buffer);
int block_device_write(uint64_t lba, uint32_t num_blocks, const uint8_t* buffer);

#endif


