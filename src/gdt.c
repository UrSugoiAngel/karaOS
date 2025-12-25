#include <gdt.h>
#include <vmm.h>
#include <util.h>

// Global Descriptor Table
// 0: Null
// 1: Kernel Code
// 2: Kernel Data
// 3: User Code
// 4: User Data
// 5: TSS (Takes up 2 slots)
static struct gdt_entry gdt[7]; 
static struct tss_entry tss;
static struct gdtr gdtr;

// Helper to fill entries
static void gdt_set_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

// Helper for the 16-byte TSS descriptor
static void gdt_set_system_gate(int num, uint64_t base, uint64_t limit, uint8_t access, uint8_t gran) {
    gdt_set_gate(num, base, limit, access, gran);
    
    // Cast to the larger struct to write the upper 32 bits
    struct gdt_system_entry *desc = (struct gdt_system_entry *)&gdt[num];
    desc->base_upper = (base >> 32) & 0xFFFFFFFF;
    desc->reserved = 0;
}

// External assembly helper (defined below)
extern void load_gdt(struct gdtr *gdtr);
extern void load_tss(void);

void gdt_init(void) {
    // 1. Setup the TSS (Task State Segment)
    // We can leave most of it zero for now, but rsp0 is critical later.
    memset(&tss, 0, sizeof(struct tss_entry));
    tss.iomap_base = sizeof(struct tss_entry); // Disable IO Map

    // 2. Setup GDT Entries
    // NULL Descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // Kernel Code (0x08) - Access: 0x9A, Gran: 0x20 (Long Mode)
    // Present | Priv 0 | Code | Ex | Readable
    gdt_set_gate(1, 0, 0, 0x9A, 0x20);

    // Kernel Data (0x10) - Access: 0x92
    // Present | Priv 0 | Data | RW
    gdt_set_gate(2, 0, 0, 0x92, 0);

    // User Code (0x18) - Access: 0xFA
    // Present | Priv 3 | Code | Ex | Readable
    gdt_set_gate(3, 0, 0, 0xFA, 0x20);

    // User Data (0x20) - Access: 0xF2
    // Present | Priv 3 | Data | RW
    gdt_set_gate(4, 0, 0, 0xF2, 0);

    // TSS (0x28) - System Segment
    // Base = &tss, Limit = sizeof(tss), Access = 0x89 (Present|Exec|Accessed)
    gdt_set_system_gate(5, (uint64_t)&tss, sizeof(struct tss_entry) - 1, 0x89, 0);

    // 3. Load GDT
    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint64_t)&gdt;
    
    load_gdt(&gdtr);
    load_tss(); // LTR instruction
}

// Helper to update the stack pointer used when an interrupt occurs
void tss_set_stack(uint64_t stack_ptr) {
    tss.rsp0 = stack_ptr;
}