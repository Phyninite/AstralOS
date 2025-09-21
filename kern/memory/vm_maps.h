#ifndef VM_MAPS_H
#define VM_MAPS_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#define VM_PROT_NONE  0x00
#define VM_PROT_READ  0x01
#define VM_PROT_WRITE 0x02
#define VM_PROT_EXEC  0x04
#define VM_PROT_KERNEL 0x08
#define VM_PROT_FREE   0x10

typedef struct {
    uint64_t virtual_address;
    uint64_t physical_address;
    uint64_t size;
    uint32_t protection_flags;
} vm_map_entry_t;

void vm_init();
int vm_map(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags);
int vm_unmap(uint64_t virtual_address, uint64_t size);
int vm_protect(uint64_t virtual_address, uint64_t size, uint32_t new_protection_flags);
uint64_t vm_map_allocate(uint64_t size, uint32_t protection_flags);
int vm_map_deallocate(uint64_t virtual_address);
void vm_map_add_entry(uint64_t virtual_address, uint64_t physical_address, uint64_t size, uint32_t protection_flags);
void cpu_enable_mmu();
uint64_t vm_create_task_pagetable();

#endif


