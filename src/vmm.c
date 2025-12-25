#include <vmm.h>
#include <pmm.h>
#include <util.h>
#include <limine.h>
#include <stdbool.h>

//from linker script
extern uint8_t __text_start[], __text_end[];
extern uint8_t __rodata_start[], __rodata_end[];
extern uint8_t __data_start[], __data_end[]; // Data covers .bss too usually

uint64_t *kernel_pml4 = NULL; //global kernel pml4
extern uint64_t hhdm_offset;


static uint64_t *get_next_page(uint64_t *table, uint64_t index, bool allocate){
    if(table[index] & PTE_PRESENT){
        uint64_t phys = table[index] & PHYS_ADDR_MASK;

        return (uint64_t*)(phys + hhdm_offset);
    }

    if(!allocate){
        return NULL;
    }

    //if neither statement has returned, the page table needs to be created still
    void *new_table_phys = pmm_alloc_page();
    if (!new_table_phys) hcf(); // OOM Panic

    //make a virtual mapping and zero out the new table
    uint64_t *new_table_virt = (uint64_t*)((uint64_t)new_table_phys + hhdm_offset);
    memset(new_table_virt, 0, PAGE_SIZE);

    table[index] = (uint64_t)new_table_phys | PTE_PRESENT | PTE_RW;
    
    return new_table_virt;
}

void vmm_map_page(pml4_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags){
    uint64_t pml4_idx = (virt >> 39) & 0x1FF;
    uint64_t pdpt_idx = (virt >> 30) & 0x1FF;
    uint64_t pd_idx   = (virt >> 21) & 0x1FF;
    uint64_t pt_idx   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt = get_next_page(pml4, pml4_idx, true);
    uint64_t *pd   = get_next_page(pdpt, pdpt_idx, true);
    uint64_t *pt   = get_next_page(pd,   pd_idx,   true);

    // Set the entry
    pt[pt_idx] = phys | flags;
    
    // Flush TLB (invalidate cache for this page)
    __asm__ volatile("invlpg (%0)" :: "r" (virt) : "memory");
}

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request exec_addr_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST_ID,
    .revision = 0
};


void vmm_init(struct limine_memmap_response *map){

    if ((uint64_t)__text_end <= (uint64_t)__text_start) {
        // PANIC: Linker script is broken, size is 0!
        // Fill screen with BLUE to indicate this specific error
        // if (framebuffer_request.response) {
        //     uint32_t *fb = framebuffer_request.response->framebuffers[0]->address;
        //     for(int i=0; i<10000; i++) fb[i] = 0x0000FF; // BLUE
        // }
        hcf();
    }

    if (exec_addr_request.response == NULL) {
        hcf(); // Panic: Bootloader didn't provide address info
    }

    uint64_t phys_base = exec_addr_request.response->physical_base;
    uint64_t virt_base = exec_addr_request.response->virtual_base;

    uint64_t kernel_pml4_phys = (uint64_t)pmm_alloc_page();
    if (!kernel_pml4_phys) hcf(); // Panic: OOM

    kernel_pml4 = (uint64_t *)(kernel_pml4_phys + hhdm_offset);
    memset(kernel_pml4, 0, PAGE_SIZE);


    //HHDM mapping
    // Flags: Present | ReadWrite | NX (Data shouldn't be executed)
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *entry = map->entries[i];
        
        // Map Usable and Bootloader Reclaimable (Kernel lives here)
        if (entry->type == LIMINE_MEMMAP_USABLE || 
            entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE ||
            entry->type == LIMINE_MEMMAP_EXECUTABLE_AND_MODULES ||
            entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            
            uint64_t start = ALIGN_DOWN(entry->base);
            uint64_t end   = ALIGN_UP(entry->base + entry->length);
            
            for (uint64_t phys = start; phys < end; phys += 4096) {
                uint64_t virt = phys + hhdm_offset;
                vmm_map_page(kernel_pml4, virt, phys, PTE_PRESENT | PTE_RW | PTE_NX);
            }
        }
    }

    // Kernel mapping
    // Flags: Present | ReadWrite | NX
    uint64_t text_start = ALIGN_DOWN((uint64_t)__text_start) - PAGE_SIZE;
    // uint64_t text_start = ALIGN_DOWN((uint64_t));
    uint64_t text_end   = ALIGN_UP((uint64_t)__text_end);

    for (uint64_t virt = text_start; virt < text_end; virt += PAGE_SIZE) {
        uint64_t phys = virt - virt_base + phys_base;
        vmm_map_page(kernel_pml4, virt, phys, PTE_PRESENT); // RW bit cleared = RO
    }
    
    // kernel rodata map
    uint64_t rodata_start = ALIGN_DOWN((uint64_t)__rodata_start);
    uint64_t rodata_end   = ALIGN_UP((uint64_t)__rodata_end);

    for(uint64_t virt = rodata_start; virt < rodata_end; virt += PAGE_SIZE){
        uint64_t phys = virt - virt_base + phys_base;
        vmm_map_page(kernel_pml4, virt, phys, PTE_PRESENT | PTE_RW | PTE_NX);
    }


    // kernel data
    uint64_t data_start = ALIGN_DOWN((uint64_t)__data_start);
    uint64_t data_end   = ALIGN_UP((uint64_t)__data_end);

    for (uint64_t virt = data_start; virt < data_end; virt += PAGE_SIZE) {
        uint64_t phys = virt - virt_base + phys_base;
        vmm_map_page(kernel_pml4, virt, phys, PTE_PRESENT | PTE_RW | PTE_NX);
    }

    //page switch (kowtow to the cpu overlords)
    __asm__ volatile("mov %0, %%cr3" :: "r" (kernel_pml4_phys) : "memory");
}