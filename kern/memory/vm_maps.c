#include "vm_maps.h"
#include "kprintf.h"
#include "lib.h"
#include "kmalloc.h"

#define MAX_VM_MAP_ENTRIES 64
static vm_map_entry_t vm_map_entries[MAX_VM_MAP_ENTRIES];

#define PAGE_SIZE 0x1000
#define ALLOC_ALIGN 0x100000     // 1mb alignment for vm allocation
#define BLOCK_SIZE 0x200000      // 2mb blocks for page table mapping
#define INITIAL_MAPPING_BASE 0x40000000

// kernel's level 1 page table allocated with 1kb alignment
static uint64_t kernel_l1_page_table[512] __attribute__((aligned(PAGE_SIZE)));
// a global level 2 page table for initial mapping (simple, static allocation)
static uint64_t kernel_l2_page_table[512] __attribute__((aligned(PAGE_SIZE)));

// memory type definitions for mair setting
#define MT_DEVICE_NGNRNE    0x00
#define MT_NORMAL_NC        0x01
#define MT_NORMAL           0x02

// major attribute register value setup
#define MAIR_VALUE ( (0x00 << (MT_DEVICE_NGNRNE * 8)) | \
                     (0x44 << (MT_NORMAL_NC * 8))     | \
                     (0xFF << (MT_NORMAL * 8)) ) 

// translation control register settings
#define TCR_T0SZ(x)         ((64 - (x)) << 0)
#define TCR_IRGN0_WBWA      (0x1 << 8)
#define TCR_ORGN0_WBWA      (0x1 << 10)
#define TCR_SH0_INNER       (0x3 << 12)
#define TCR_TG0_4KB         (0x0 << 14)
#define TCR_PS_48BIT        (0x5 << 16)
#define TCR_VALUE (TCR_T0SZ(48) | TCR_IRGN0_WBWA | TCR_ORGN0_WBWA | TCR_SH0_INNER | TCR_TG0_4KB | TCR_PS_48BIT)

// page table entry flags
#define PTE_VALID                (1ULL << 0)
#define PTE_TABLE                (1ULL << 1)
#define PTE_BLOCK                (0ULL << 1)
#define PTE_AF                   (1ULL << 10)
#define PTE_SH_INNER_SHAREABLE   (0x3ULL << 8)
#define PTE_AP_RW_EL1            (0x0ULL << 6)
#define PTE_PXN                  (1ULL << 53)
#define PTE_UXN                  (1ULL << 54)

// basic tlb invalidation function
static void tlb_invalidate(void) {
    asm volatile("tlbi vmalle1is");
    asm volatile("dsb sy");
    asm volatile("isb sy");
}

// find a free slot in the vm map entries array by scanning for an entry marked as free
static int find_free_vm_map_entry_slot() {
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        if (vm_map_entries[i].protection_flags == VM_PROT_FREE)
            return i;
    }
    return -1;
}

// check if a given mapping range overlaps with an existing mapping
static int check_range_overlap(uint64_t va, uint64_t size) {
    uint64_t new_end = va + size;
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        if (vm_map_entries[i].protection_flags != VM_PROT_FREE) {
            uint64_t existing_start = vm_map_entries[i].virtual_address;
            uint64_t existing_end = existing_start + vm_map_entries[i].size;
            if ((va < existing_end) && (new_end > existing_start))
                return 1;
        }
    }
    return 0;
}

// add a new vm mapping entry to the tracking array, checking for free slot and range overlap
void vm_map_add_entry(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags) {
    // check for overlap with existing mappings
    if (check_range_overlap(virtual_address, size)) {
        kprintf("vm_map_add_entry: mapping overlap detected at va: 0x%llx\n", virtual_address);
        return;
    }

    int index = find_free_vm_map_entry_slot();
    if (index == -1) {
        kprintf("vm_map_add_entry: no free vm map entries!\n");
        return;
    }
    vm_map_entries[index].virtual_address = virtual_address;
    vm_map_entries[index].physical_address = physical_address;
    vm_map_entries[index].size = size;
    vm_map_entries[index].protection_flags = protection_flags;
    // invalidate tlb after adding a new mapping
    tlb_invalidate();
}

// initialize all vm map entries to free and add the initial kernel mapping
void vm_init() {
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        vm_map_entries[i].protection_flags = VM_PROT_FREE;
    }
    // add kernel mapping: map first 1gb of physical address space with full permissions
    vm_map_add_entry(0x0, 0x0, 0x40000000, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXEC | VM_PROT_KERNEL);
}

// map a given virtual address range to a physical address range with specific protection flags
int vm_map(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags) {
    vm_map_add_entry(virtual_address, physical_address, size, protection_flags);
    // invalidate tlb after mapping change
    tlb_invalidate();
    return 0;
}

// unmap a given virtual address range; size parameter is currently ignored and unmaps the entire mapping
int vm_unmap(uint64_t virtual_address, uint64_t size) {
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        if (vm_map_entries[i].virtual_address == virtual_address && vm_map_entries[i].protection_flags != VM_PROT_FREE) {
            vm_map_entries[i].protection_flags = VM_PROT_FREE;
            // invalidate tlb after unmapping
            tlb_invalidate();
            return 0;
        }
    }
    return -1;
}

// change protection flags for a given vm mapping range; size parameter is not used for now
int vm_protect(uint64_t virtual_address, uint64_t size, uint32_t new_protection_flags) {
    for (int i = 0; i < MAX_VM_MAP_ENTRIES; i++) {
        if (vm_map_entries[i].virtual_address == virtual_address && vm_map_entries[i].protection_flags != VM_PROT_FREE) {
            vm_map_entries[i].protection_flags = new_protection_flags;
            // invalidate tlb after permission change
            tlb_invalidate();
            return 0;
        }
    }
    return -1;
}

// allocate a new vm mapping using a simple linear allocation scheme with fixed alignment
uint64_t vm_map_allocate(uint64_t size, uint32_t protection_flags) {
    int index = find_free_vm_map_entry_slot();
    if (index == -1) {
        kprintf("vm_map_allocate: no free mapping slots available\n");
        return 0;
    }
    // calculate allocated virtual address based on index and fixed allocation alignment
    uint64_t allocated_va = INITIAL_MAPPING_BASE + (index * ALLOC_ALIGN);
    // assume pa = va mapping for now; in production, a real physical allocator should be used
    uint64_t allocated_pa = allocated_va;
    vm_map_add_entry(allocated_va, allocated_pa, size, protection_flags);
    return allocated_va;
}

// deallocate vm mapping by unmapping at the given virtual address
int vm_map_deallocate(uint64_t virtual_address) {
    return vm_unmap(virtual_address, 0);
}

// enable the mmu with basic page table setup
void cpu_enable_mmu() {
    // clear kernel level 1 and level 2 page tables
    memset(kernel_l1_page_table, 0, sizeof(kernel_l1_page_table));
    memset(kernel_l2_page_table, 0, sizeof(kernel_l2_page_table));

    // map first 1gb of physical memory as 2mb blocks using the global level 2 table
    kernel_l1_page_table[0] = (uint64_t)kernel_l2_page_table | PTE_TABLE | PTE_VALID;

    for (int i = 0; i < 512; i++) {
        uint64_t pa = i * BLOCK_SIZE;
        uint64_t attributes = PTE_VALID | PTE_BLOCK | PTE_AF | PTE_SH_INNER_SHAREABLE | PTE_AP_RW_EL1;
        attributes |= (MT_NORMAL << 2);
        kernel_l2_page_table[i] = pa | attributes;
    }

    // map the framebuffer as device memory, assuming framebuffer is 16mb at 0x40000000
    uint64_t fb_base_pa = INITIAL_MAPPING_BASE;
    uint64_t fb_size = 0x1000000; // 16mb
    uint64_t l1_idx = (fb_base_pa >> 30) & 0x1FF;

    // if the l1 entry for fb region does not have a valid l2 table, allocate one
    if (!(kernel_l1_page_table[l1_idx] & PTE_VALID)) {
        uint64_t* new_l2_table = (uint64_t*)kmalloc(PAGE_SIZE);
        memset(new_l2_table, 0, PAGE_SIZE);
        kernel_l1_page_table[l1_idx] = (uint64_t)new_l2_table | PTE_TABLE | PTE_VALID;
    }
    // use the existing l2 table for framebuffer mapping
    uint64_t* fb_l2_table = (uint64_t*)(kernel_l1_page_table[l1_idx] & ~0xFFFULL);
    uint64_t fb_attributes = PTE_VALID | PTE_BLOCK | PTE_AF | PTE_SH_INNER_SHAREABLE | PTE_AP_RW_EL1 | PTE_PXN | PTE_UXN;
    fb_attributes |= (MT_DEVICE_NGNRNE << 2);
    // map framebuffer over the required 2mb blocks
    for (uint64_t current_pa = fb_base_pa; current_pa < fb_base_pa + fb_size; current_pa += BLOCK_SIZE) {
        uint64_t l2_idx = (current_pa >> 21) & 0x1FF;
        fb_l2_table[l2_idx] = current_pa | fb_attributes;
    }

    // set mmu attributes and translation control registers
    asm volatile("msr mair_el1, %0" : : "r"(MAIR_VALUE));
    asm volatile("msr tcr_el1, %0" : : "r"(TCR_VALUE));
    asm volatile("msr ttbr0_el1, %0" : : "r"((uint64_t)kernel_l1_page_table));

    // invalidate tlb and perform barrier operations
    tlb_invalidate();

    uint64_t sctlr;
    asm volatile("mrs %0, sctlr_el1" : "=r"(sctlr));
    sctlr |= (1 << 0);
    asm volatile("msr sctlr_el1, %0" : : "r"(sctlr));
    asm volatile("isb sy");
}

// create a new task-specific pagetable by copying kernel mappings
uint64_t vm_create_task_pagetable() {
    uint64_t* task_l1_table = (uint64_t*)kmalloc(PAGE_SIZE);
    if (!task_l1_table) {
        kprintf("vm_create_task_pagetable: failed to allocate task l1 table\n");
        return 0;
    }
    memset(task_l1_table, 0, PAGE_SIZE);
    // copy kernel mappings into task's level 1 table
    for (int i = 0; i < 512; i++) {
        task_l1_table[i] = kernel_l1_page_table[i];
    }
    return (uint64_t)task_l1_table;
}
