#include "security.h"
#include "vm_maps.h"
#include "kprintf.h"

void security_enforce_wx(uint64_t addr, uint64_t size, uint32_t current_prot_flags) {
    if ((current_prot_flags & VM_PROT_WRITE) && (current_prot_flags & VM_PROT_EXEC)) {
        vm_protect(addr, size, (current_prot_flags & ~VM_PROT_EXEC));
    }
}


