#include "block_device.h"
#include "kprintf.h"
#include "lib.h"

#define UFS_HCI_BASE 0xDEAD0000

#define UFS_HCI_CAPABILITIES_REG            (volatile uint32_t*)(UFS_HCI_BASE + 0x00)
#define UFS_HCI_VERSION_REG                 (volatile uint32_t*)(UFS_HCI_BASE + 0x04)
#define UFS_HCI_CONTROLLER_STATUS_REG       (volatile uint32_t*)(UFS_HCI_BASE + 0x08)
#define UFS_HCI_CONTROLLER_ENABLE_REG       (volatile uint32_t*)(UFS_HCI_BASE + 0x0C)
#define UFS_HCI_CONTROLLER_RESET_REG        (volatile uint32_t*)(UFS_HCI_BASE + 0x10)
#define UFS_HCI_INTERRUPT_STATUS_REG        (volatile uint32_t*)(UFS_HCI_BASE + 0x20)
#define UFS_HCI_INTERRUPT_ENABLE_REG        (volatile uint32_t*)(UFS_HCI_BASE + 0x24)
#define UFS_HCI_UTP_TRANSFER_REQ_INT_EN_REG (volatile uint32_t*)(UFS_HCI_BASE + 0x28)
#define UFS_HCI_UTP_TASK_REQ_INT_EN_REG     (volatile uint32_t*)(UFS_HCI_BASE + 0x2C)
#define UFS_HCI_UTP_TRANSFER_REQ_DOORBELL_REG (volatile uint32_t*)(UFS_HCI_BASE + 0x50)
#define UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_L_REG (volatile uint32_t*)(UFS_HCI_BASE + 0x60)
#define UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_H_REG (volatile uint32_t*)(UFS_HCI_BASE + 0x64)

#define UTP_TRD_COMMAND_TYPE_SCSI   0x00
#define UTP_TRD_COMMAND_TYPE_UFS    0x01
#define UTP_TRD_DD_READ             0x01
#define UTP_TRD_DD_WRITE            0x02
#define UTP_TRD_INT_CMD             0x01

#define PRDT_DATA_BASE_L_OFFSET 0
#define PRDT_DATA_BASE_H_OFFSET 4
#define PRDT_DATA_BYTE_COUNT_OFFSET 8
#define PRDT_RESERVED_OFFSET 12

#define UFS_TRL_BASE 0xDEAE0000
#define UFS_PRDT_BASE 0xDEAF0000

static utp_trd_t* ufs_trd_list = (utp_trd_t*)UFS_TRL_BASE;
static prdt_entry_t* ufs_prdt_list = (prdt_entry_t*)UFS_PRDT_BASE;

void ufs_init() {
    *UFS_HCI_CONTROLLER_RESET_REG = 1;
    while (*UFS_HCI_CONTROLLER_RESET_REG & 1);

    *UFS_HCI_CONTROLLER_ENABLE_REG = 1;
    while (!(*UFS_HCI_CONTROLLER_STATUS_REG & 1));

    *UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_L_REG = (uint32_t)UFS_TRL_BASE;
    *UFS_HCI_UTP_TRANSFER_REQ_LIST_BASE_H_REG = (uint32_t)((uint64_t)UFS_TRL_BASE >> 32);

    *UFS_HCI_UTP_TRANSFER_REQ_INT_EN_REG = 0xFFFFFFFF;
    *UFS_HCI_UTP_TASK_REQ_INT_EN_REG = 0xFFFFFFFF;

    kprintf("ufs initialized\n");
}

static int ufs_send_command(uint64_t lba, uint32_t num_blocks, uint8_t* buffer, uint32_t data_direction) {
    utp_trd_t* trd = &ufs_trd_list[0];
    prdt_entry_t* prdt = &ufs_prdt_list[0];

    memset(trd, 0, sizeof(utp_trd_t));
    memset(prdt, 0, sizeof(prdt_entry_t));

    prdt->dword0 = (uint32_t)(uint64_t)buffer;
    prdt->dword1 = (uint32_t)((uint64_t)buffer >> 32);
    prdt->dword2 = (num_blocks * UFS_BLOCK_SIZE) - 1;

    trd->dword0 = (UTP_TRD_COMMAND_TYPE_SCSI << 29) | (data_direction << 24) | (UTP_TRD_INT_CMD << 16);
    trd->dword1 = (uint32_t)UFS_PRDT_BASE;
    trd->dword2 = (uint32_t)((uint64_t)UFS_PRDT_BASE >> 32);

    uint8_t* cdb = (uint8_t*)&trd->dword4;
    if (data_direction == UTP_TRD_DD_READ) {
        cdb[0] = 0x28;
    } else {
        cdb[0] = 0x2A;
    }
    cdb[2] = (lba >> 24) & 0xFF;
    cdb[3] = (lba >> 16) & 0xFF;
    cdb[4] = (lba >> 8) & 0xFF;
    cdb[5] = lba & 0xFF;
    cdb[7] = (num_blocks >> 8) & 0xFF;
    cdb[8] = num_blocks & 0xFF;

    *UFS_HCI_UTP_TRANSFER_REQ_DOORBELL_REG = 1;

    while (*UFS_HCI_UTP_TRANSFER_REQ_DOORBELL_REG & 1);

    return 0;
}

int ufs_read_blocks(uint64_t lba, uint32_t num_blocks, uint8_t* buffer) {
    return ufs_send_command(lba, num_blocks, buffer, UTP_TRD_DD_READ);
}

int ufs_write_blocks(uint64_t lba, uint32_t num_blocks, const uint8_t* buffer) {
    return ufs_send_command(lba, num_blocks, (uint8_t*)buffer, UTP_TRD_DD_WRITE);
}

#define EMMC_HCI_BASE 0xBEEF0000

#define EMMC_HCI_ARGUMENT_REG       (volatile uint32_t*)(EMMC_HCI_BASE + 0x00)
#define EMMC_HCI_COMMAND_REG        (volatile uint32_t*)(EMMC_HCI_BASE + 0x04)
#define EMMC_HCI_RESPONSE_REG       (volatile uint32_t*)(EMMC_HCI_BASE + 0x08)
#define EMMC_HCI_DATA_REG           (volatile uint32_t*)(EMMC_HCI_BASE + 0x0C)
#define EMMC_HCI_STATUS_REG         (volatile uint32_t*)(EMMC_HCI_BASE + 0x10)
#define EMMC_HCI_INTERRUPT_STATUS_REG (volatile uint32_t*)(EMMC_HCI_BASE + 0x14)
#define EMMC_HCI_INTERRUPT_ENABLE_REG (volatile uint32_t*)(EMMC_HCI_BASE + 0x18)

#define EMMC_CMD_GO_IDLE_STATE      0x00
#define EMMC_CMD_SEND_OP_COND       0x01
#define EMMC_CMD_ALL_SEND_CID       0x02
#define EMMC_CMD_SET_RELATIVE_ADDR  0x03
#define EMMC_CMD_READ_SINGLE_BLOCK  0x11
#define EMMC_CMD_WRITE_SINGLE_BLOCK 0x18
#define EMMC_CMD_READ_MULTIPLE_BLOCK 0x12
#define EMMC_CMD_WRITE_MULTIPLE_BLOCK 0x19
#define EMMC_CMD_STOP_TRANSMISSION  0x0C

#define EMMC_STATUS_CMD_INHIBIT     (1 << 0)
#define EMMC_STATUS_DATA_INHIBIT    (1 << 1)
#define EMMC_STATUS_BUFFER_READ_READY (1 << 11)
#define EMMC_STATUS_BUFFER_WRITE_READY (1 << 12)
#define EMMC_STATUS_TRANSFER_COMPLETE (1 << 1)

void emmc_init() {
    kprintf("emmc initialized\n");
}

static int emmc_send_command(uint32_t cmd, uint32_t arg, uint32_t response_type) {
    while (*EMMC_HCI_STATUS_REG & EMMC_STATUS_CMD_INHIBIT);

    *EMMC_HCI_ARGUMENT_REG = arg;
    *EMMC_HCI_COMMAND_REG = cmd | response_type;

    while (!(*EMMC_HCI_INTERRUPT_STATUS_REG & EMMC_STATUS_TRANSFER_COMPLETE));
    *EMMC_HCI_INTERRUPT_STATUS_REG = EMMC_STATUS_TRANSFER_COMPLETE;

    return *EMMC_HCI_RESPONSE_REG;
}

int emmc_read_blocks(uint32_t lba, uint32_t num_blocks, uint8_t* buffer) {
    if (num_blocks == 0) return 0;

    emmc_send_command(EMMC_CMD_READ_MULTIPLE_BLOCK, lba, 0);

    for (uint32_t i = 0; i < num_blocks; i++) {
        while (!(*EMMC_HCI_STATUS_REG & EMMC_STATUS_BUFFER_READ_READY));
        for (uint32_t j = 0; j < EMMC_BLOCK_SIZE / sizeof(uint32_t); j++) {
            ((uint32_t*)buffer)[i * (EMMC_BLOCK_SIZE / sizeof(uint32_t)) + j] = *EMMC_HCI_DATA_REG;
        }
    }

    emmc_send_command(EMMC_CMD_STOP_TRANSMISSION, 0, 0);

    return 0;
}

int emmc_write_blocks(uint32_t lba, uint32_t num_blocks, const uint8_t* buffer) {
    if (num_blocks == 0) return 0;

    emmc_send_command(EMMC_CMD_WRITE_MULTIPLE_BLOCK, lba, 0);

    for (uint32_t i = 0; i < num_blocks; i++) {
        while (!(*EMMC_HCI_STATUS_REG & EMMC_STATUS_BUFFER_WRITE_READY));
        for (uint32_t j = 0; j < EMMC_BLOCK_SIZE / sizeof(uint32_t); j++) {
            *EMMC_HCI_DATA_REG = ((uint32_t*)buffer)[i * (EMMC_BLOCK_SIZE / sizeof(uint32_t)) + j];
        }
    }

    emmc_send_command(EMMC_CMD_STOP_TRANSMISSION, 0, 0);

    return 0;
}

static block_device_type_t active_block_device_type = BLOCK_DEVICE_TYPE_NONE;

block_device_type_t get_active_block_device_type() {
    return active_block_device_type;
}

void set_active_block_device(block_device_type_t type) {
    active_block_device_type = type;
}

int block_device_read(uint64_t lba, uint32_t num_blocks, uint8_t* buffer) {
    if (active_block_device_type == BLOCK_DEVICE_TYPE_UFS) {
        return ufs_read_blocks(lba, num_blocks, buffer);
    } else if (active_block_device_type == BLOCK_DEVICE_TYPE_EMMC) {
        return emmc_read_blocks(lba, num_blocks, buffer);
    }
    return -1;
}

int block_device_write(uint64_t lba, uint32_t num_blocks, const uint8_t* buffer) {
    if (active_block_device_type == BLOCK_DEVICE_TYPE_UFS) {
        return ufs_write_blocks(lba, num_blocks, buffer);
    } else if (active_block_device_type == BLOCK_DEVICE_TYPE_EMMC) {
        return emmc_write_blocks(lba, num_blocks, buffer);
    }
    return -1;
}


