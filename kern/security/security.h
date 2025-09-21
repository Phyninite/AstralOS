#ifndef SECURITY_H
#define SECURITY_H

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

// Protection flags for W^X
#define VM_PROT_READ  0x01
#define VM_PROT_WRITE 0x02
#define VM_PROT_EXEC  0x04

void security_enforce_wx(uint64_t addr, uint64_t size, uint32_t current_prot_flags);

#endif // SECURITY_H


