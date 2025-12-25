#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <limine.h>
#include <util.h>
#include <pmm.h>
#include <vmm.h>
#include <gdt.h>
#include <idt.h>



uint64_t hhdm_offset;

// Set the base revision to 4, this is recommended as this is the latest
// base revision described by the Limine boot protocol specification.
// See specification for further info.

__attribute__((used, section(".limine_requests")))
static volatile uint64_t limine_base_revision[] = LIMINE_BASE_REVISION(4);

// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent, _and_ they should be accessed at least
// once or marked as used with the "used" attribute as done here.

__attribute__((used, section(".limine_requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID,
    .revision = 0
};

// Finally, define the start and end markers for the Limine requests.
// These can also be moved anywhere, to any .c file, as seen fit.

__attribute__((used, section(".limine_requests_start")))
static volatile uint64_t limine_requests_start_marker[] = LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end")))
static volatile uint64_t limine_requests_end_marker[] = LIMINE_REQUESTS_END_MARKER;


__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST_ID,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST_ID,
    .revision = 0
};


// --- Helper: Panic (Red Screen of Death) ---
void panic(void) {
    if (framebuffer_request.response != NULL && framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        volatile uint32_t *fb_ptr = fb->address;
        // Fill screen with RED
        for (size_t i = 0; i < fb->width * fb->height; i++) {
            fb_ptr[i] = 0xFF0000; 
        }
    }
    hcf(); // Halt
}

// --- Helper: Success (Green Screen) ---
void success(void) {
    if (framebuffer_request.response != NULL && framebuffer_request.response->framebuffer_count > 0) {
        struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
        volatile uint32_t *fb_ptr = fb->address;
        // Fill screen with GREEN
        for (size_t i = 0; i < fb->width * fb->height; i++) {
            fb_ptr[i] = 0x00FF00; 
        }
    }
    hcf();
}

uint64_t get_framebuffer_phys_addr(struct limine_memmap_response *map) {
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *entry = map->entries[i];
        if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER) {
            return entry->base; // Found the physical address!
        }
    }
    return 0; // Not found
}

// --- MAIN KERNEL ENTRY ---
void kmain(void) {
    // 1. Basic Revision Check
    if (LIMINE_BASE_REVISION_SUPPORTED(limine_base_revision) == false) {
        hcf();
    }

    // 2. Initialize the PMM
    // We must pass the responses from main.c to the PMM
    if (memmap_request.response == NULL || hhdm_request.response == NULL) {
        panic(); // Bootloader failed to give us memory info
    }

    hhdm_offset = hhdm_request.response->offset;

    serial_init();
    
    // Call your init function
    debug_print("---START DEBUG---\n");
    pmm_init(memmap_request.response, hhdm_offset);
    debug_print("PMM Initialized\n Initializing VMM...");
    vmm_init(memmap_request.response);
    debug_print("VMM Initialized\n");
    debug_print("---END DEBUG---\n");

    gdt_init();
    idt_init();

    

    // 1. Get the Physical Address of the framebuffer
    uint64_t fb_phys = get_framebuffer_phys_addr(memmap_request.response);
    
    // 2. Calculate the NEW Virtual Address (using your HHDM mapping)
    uint64_t hhdm_offset = hhdm_request.response->offset;
    volatile uint32_t *new_fb_ptr = (uint32_t*)(fb_phys + hhdm_offset);
    
    // 3. Update the global struct so panic() uses the new address
    // (Ideally, don't hack the struct like this, but for now it works)
    framebuffer_request.response->framebuffers[0]->address = (void*)new_fb_ptr;

    // success();


    // ============================================
    // TEST 1: Basic Allocation
    // ============================================
    void *phys_ptr_1 = pmm_alloc_page();
    
    if (phys_ptr_1 == NULL) panic(); // FAIL: Should not return NULL this early

    // Test Writing to it
    // CRITICAL: You cannot write to phys_ptr_1 directly! You must add HHDM offset.
    uint64_t *virt_ptr_1 = (uint64_t*)((uint64_t)phys_ptr_1 + hhdm_offset);
    *virt_ptr_1 = 0xDEADBEEFCAFEBABE; // Write magic number
    
    if (*virt_ptr_1 != 0xDEADBEEFCAFEBABE) panic(); // FAIL: Memory didn't retain data

    // ============================================
    // TEST 2: Multi-Page Allocation & Distinctness
    // ============================================
    void *phys_ptr_2 = pmm_alloc_page();
    void *phys_ptr_3 = pmm_alloc_page();

    // FAIL: Pointers should be distinct
    if (phys_ptr_1 == phys_ptr_2 || phys_ptr_2 == phys_ptr_3) panic(); 
    
    // FAIL: Addresses should be page aligned (end in 000)
    if (((uint64_t)phys_ptr_2 % 4096) != 0) panic(); 

    // ============================================
    // TEST 3: Freeing and Reclaiming
    // ============================================
    // We free ptr_2. The next alloc should ideally grab ptr_2's spot 
    // (since it's lower than ptr_3 and free).
    pmm_free_page(phys_ptr_2);

    void *phys_ptr_4 = pmm_alloc_page();
    
    // With a standard bitmap search from 0, this usually returns the same address.
    // If you implemented the "Next Fit" cursor optimization, this might fail, 
    // so this check depends on your specific implementation.
    // For now, just ensure we got a valid pointer back.
    if (phys_ptr_4 == NULL) panic();

    // ============================================
    // TEST 4: Stats Check (Optional)
    // ============================================
    size_t free_mem = pmm_get_free_page_count();
    if (free_mem == 0) panic(); // FAIL: We definitely have more memory than that.


    // If we made it here, everything works!
    success();
    // hcf();
}
