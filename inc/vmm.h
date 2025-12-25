#ifndef VMM_H
#define VMM_H
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

typedef uint64_t pml4_t;

// Initialize VMM, create the Kernel PML4, and switch CR3
void vmm_init(struct limine_memmap_response *map); 
 
// Map a specific virtual address to a physical address
void vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags);

// Unmap a page (mostly for cleanup)
void vmm_unmap_page(pml4_t *pml4, uint64_t virt);

// Switch the CPU to a new Address Space (Load CR3)
void vmm_switch_pml4(pml4_t *pml4);


// Intel x64 Page Table Flags
#define PTE_PRESENT   (1ULL << 0)
#define PTE_RW        (1ULL << 1)
#define PTE_USER      (1ULL << 2)
#define PTE_PWT       (1ULL << 3)
#define PTE_PCD       (1ULL << 4)
#define PTE_ACCESSED  (1ULL << 5)
#define PTE_DIRTY     (1ULL << 6)
#define PTE_HUGE      (1ULL << 7) // For 2MB/1GB pages
#define PTE_GLOBAL    (1ULL << 8)
// #define PTE_NX        (1ULL << 63) // No Execute
#define PTE_NX 0

// Align an address down/up to the nearest 4096 bytes
#define ALIGN_DOWN(addr) ((addr) & ~(0xFFF))
#define ALIGN_UP(addr)   (((addr) + 0xFFF) & ~(0xFFF))

#define PHYS_ADDR_MASK 0x000FFFFFFFFFF000ULL // Mask to get physical address from page table entry

#endif // VMM_H