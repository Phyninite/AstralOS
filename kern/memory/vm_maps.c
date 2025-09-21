#include "vm_maps.h"
#include "kprintf.h"
#include "lib.h"
#include "kmalloc.h"

#define MAX_VM_MAP_ENTRIES 64
static vm_map_entry_t vm_map_entries[MAX_VM_MAP_ENTRIES];
static int num_vm_map_entries = 0;

#define PAGE_SIZE 0x1000
#define PAGE_TABLE_L1_SIZE 0x1000
#define PAGE_TABLE_L2_SIZE 0x1000

static uint64_t kernel_l1_page_table[512] __attribute__((aligned(PAGE_TABLE_L1_SIZE)));
static uint64_t kernel_l2_page_table[512] __attribute__((aligned(PAGE_TABLE_L2_SIZE)));

#define MT_DEVICE_NGNRNE    0x00
#define MT_NORMAL_NC        0x01
#define MT_NORMAL           0x02

#define MAIR_VALUE ( (0x00 << (MT_DEVICE_NGNRNE * 8)) | \
                     (0x44 << (MT_NORMAL_NC * 8))     | \
                     (0xFF << (MT_NORMAL * 8)) ) 

#define TCR_T0SZ(x)         ((64 - (x)) << 0)
#define TCR_IRGN0_WBWA      (0x1 << 8)
#define TCR_ORGN0_WBWA      (0x1 << 10)
#define TCR_SH0_INNER       (0x3 << 12)
#define TCR_TG0_4KB         (0x0 << 14)
#define TCR_PS_48BIT        (0x5 << 16)

#define TCR_VALUE (TCR_T0SZ(48) | TCR_IRGN0_WBWA | TCR_ORGN0_WBWA | TCR_SH0_INNER | TCR_TG0_4KB | TCR_PS_48BIT)

#define PTE_VALID           (1ULL << 0)
#define PTE_TABLE           (1ULL << 1)
#define PTE_BLOCK           (0ULL << 1)
#define PTE_AF              (1ULL << 10)
#define PTE_SH_INNER_SHAREABLE (0x3ULL << 8)
#define PTE_AP_RW_EL1       (0x0ULL << 6)
#define PTE_AP_RW_EL0       (0x1ULL << 6)
#define PTE_AP_RO_EL1       (0x2ULL << 6)
#define PTE_AP_RO_EL0       (0x3ULL << 6)
#define PTE_NS              (1ULL << 5)
#define PTE_PXN             (1ULL << 53)
#define PTE_UXN             (1ULL << 54)

static int get_vm_map_entry_index(uint64_t virtual_address) {
    for (int i = 0; i < num_vm_map_entries; i++) {
        if (vm_map_entries[i].virtual_address == virtual_address) {
            return i;
        }
    }
    return -1;
}

static int find_free_vm_map_entry_slot() {
    if (num_vm_map_entries < MAX_VM_MAP_ENTRIES) {
        return num_vm_map_entries;
    }
    return -1;
}

void vm_map_add_entry(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags) {
    int index = find_free_vm_map_entry_slot();
    if (index == -1) {
        kprintf("vm_map_add_entry: no free vm map entries!\n");
        return;
    }

    vm_map_entries[index].virtual_address = virtual_address;
    vm_map_entries[index].physical_address = physical_address;
    vm_map_entries[index].size = size;
    vm_map_entries[index].protection_flags = protection_flags;
    num_vm_map_entries++;
}

void vm_init() {
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        vm_map_entries[i].protection_flags = VM_PROT_FREE;
    }
    num_vm_map_entries = 0;

    vm_map_add_entry(0x0, 0x0, 0x40000000, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC | VM_PROT_KERNEL);
}

int vm_map(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags) {
    vm_map_add_entry(virtual_address, physical_address, size, protection_flags);
    return 0;
}

int vm_unmap(uint64_t virtual_address, uint64_t size) {
    int index = get_vm_map_entry_index(virtual_address);
    if (index != -1) {
        vm_map_entries[index].protection_flags = VM_PROT_FREE;
        return 0;
    }
    return -1;
}

int vm_protect(uint64_t virtual_address, uint64_t size, uint32_t new_protection_flags) {
    int index = get_vm_map_entry_index(virtual_address);
    if (index != -1) {
        vm_map_entries[index].protection_flags = new_protection_flags;
        return 0;
    }
    return -1;
}

uint64_t vm_map_allocate(uint64_t size, uint32_t protection_flags) {
    int index = find_free_vm_map_entry_slot();
    if (index == -1) {
        return 0;
    }

    uint64_t allocated_va = 0x40000000 + (index * 0x100000);
    uint64_t allocated_pa = 0x40000000 + (index * 0x100000);

    vm_map_add_entry(allocated_va, allocated_pa, size, protection_flags);
    return allocated_va;
}

int vm_map_deallocate(uint64_t virtual_address) {
    return vm_unmap(virtual_address, 0);
}

void cpu_enable_mmu() {
    memset(kernel_l1_page_table, 0, sizeof(kernel_l1_page_table));
    memset(kernel_l2_page_table, 0, sizeof(kernel_l2_page_table));

    // map first 1gb of physical memory (0x0 - 0x40000000) as 2mb blocks
    // and also map the framebuffer at 0x40000000
    kernel_l1_page_table[0] = (uint64_t)kernel_l2_page_table | PTE_TABLE | PTE_VALID;

    for (int i = 0; i < 512; i++) {
        uint64_t pa = (uint64_t)i * 0x200000;
        uint64_t attributes = PTE_VALID | PTE_BLOCK | PTE_AF | PTE_SH_INNER_SHAREABLE | PTE_AP_RW_EL1;

        attributes |= (MT_NORMAL << 2);

        kernel_l2_page_table[i] = pa | attributes;
    }

    // explicitly map the framebuffer at 0x40000000 as device memory
    // this assumes the framebuffer is 16MB (0x1000000) in size, adjust if needed
    uint64_t fb_base_pa = 0x40000000;
    uint64_t fb_size = 0x1000000; // 16MB for framebuffer
    
    // calculate l1 and l2 indices for framebuffer base
    uint64_t l1_idx = (fb_base_pa >> 30) & 0x1FF; // 1GB per L1 entry
    uint64_t l2_idx = (fb_base_pa >> 21) & 0x1FF; // 2MB per L2 entry

    // ensure l1 entry points to a valid l2 table
    if (!(kernel_l1_page_table[l1_idx] & PTE_VALID)) {
        uint64_t* new_l2_table = (uint64_t*)kmalloc(PAGE_TABLE_L2_SIZE);
        memset(new_l2_table, 0, PAGE_TABLE_L2_SIZE);
        kernel_l1_page_table[l1_idx] = (uint64_t)new_l2_table | PTE_TABLE | PTE_VALID;
    }

    // map framebuffer as device memory
    uint64_t fb_attributes = PTE_VALID | PTE_BLOCK | PTE_AF | PTE_SH_INNER_SHAREABLE | PTE_AP_RW_EL1 | PTE_PXN | PTE_UXN; // no execute
    fb_attributes |= (MT_DEVICE_NGNRNE << 2);
    
    // map all 2MB blocks covered by the framebuffer
    for (uint64_t current_pa = fb_base_pa; current_pa < fb_base_pa + fb_size; current_pa += 0x200000) {
        l2_idx = (current_pa >> 21) & 0x1FF;
        kernel_l2_page_table[l2_idx] = current_pa | fb_attributes;
    }

    asm volatile("msr mair_el1, %0" : : "r"(MAIR_VALUE));

    asm volatile("msr tcr_el1, %0" : : "r"(TCR_VALUE));

    asm volatile("msr ttbr0_el1, %0" : : "r"((uint64_t)kernel_l1_page_table));

    asm volatile("tlbi vmalle1is");
    asm volatile("dsb sy");
    asm volatile("isb sy");

    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1 << 0);
    asm volatile("msr sctlr_el1, %0" : : "r"(sctlr));

    asm volatile("isb sy");
}

uint64_t vm_create_task_pagetable() {
    uint64_t* task_l1_table = (uint64_t*)kmalloc(PAGE_TABLE_L1_SIZE);
    if (!task_l1_table) {
        return 0;
    }
    memset(task_l1_table, 0, PAGE_TABLE_L1_SIZE);

    for (int i = 0; i < 512; i++) {
        task_l1_table[i] = kernel_l1_page_table[i];
    }

    return (uint64_t)task_l1_table;
}


