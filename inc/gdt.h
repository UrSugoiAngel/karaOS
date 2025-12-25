#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// Standard 8-byte GDT Entry
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// Special 16-byte System Segment Entry (For TSS)
struct gdt_system_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;
    uint32_t reserved;
} __attribute__((packed));

// The Task State Segment (x64)
struct tss_entry {
    uint32_t reserved0;
    uint64_t rsp0; // CRITICAL: Kernel Stack Pointer for Interrupts
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7]; // Interrupt Stack Table
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));

// GDTR Pointer
struct gdtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void gdt_init(void);
void tss_set_stack(uint64_t stack_ptr);

#endif