#include <idt.h>
#include <util.h>

static struct idt_entry idt[IDT_ENTRIES];
static struct idtr idtr;

extern void *isr_stub_table[]; // Defined in Assembly

static void idt_set_gate(uint8_t vector, void *isr, uint8_t flags) {
    uint64_t addr = (uint64_t)isr;

    idt[vector].isr_low    = addr & 0xFFFF;
    idt[vector].kernel_cs  = 0x08; // Offset of Kernel Code in GDT
    idt[vector].ist        = 0;    // Use Interrupt Stack Table 0 (from TSS)
    idt[vector].attributes = flags;
    idt[vector].isr_mid    = (addr >> 16) & 0xFFFF;
    idt[vector].isr_high   = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].reserved   = 0;
}

// Helper to remap the legacy 8259 PIC
// Master -> 0x20 to 0x27
// Slave  -> 0x28 to 0x2F
static void pic_remap(void) {
    uint8_t a1, a2;
    
    a1 = inb(0x21); // Save masks
    a2 = inb(0xA1);
    
    outb(0x20, 0x11); outb(0xA0, 0x11); // Start initialization
    outb(0x21, 0x20); outb(0xA1, 0x28); // Offset vectors
    outb(0x21, 0x04); outb(0xA1, 0x02); // Tell Master about Slave
    outb(0x21, 0x01); outb(0xA1, 0x01); // 8086 mode
    
    outb(0x21, a1);   outb(0xA1, a2);   // Restore masks
}

void idt_init(void) {
    pic_remap();

    // Fill IDT with our assembly stubs
    // 0x8E = Present | Ring0 | Interrupt Gate
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate(i, isr_stub_table[i], 0x8E);
    }

    idtr.base = (uint64_t)&idt;
    idtr.limit = sizeof(idt) - 1;

    __asm__ volatile ("lidt %0" : : "m"(idtr));
    __asm__ volatile ("sti"); // Re-enable interrupts
}