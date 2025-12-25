#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IDT_ENTRIES 256

// Interrupt Gate (16 bytes)
struct idt_entry {
    uint16_t isr_low;       // Lower 16 bits of handler address
    uint16_t kernel_cs;     // Kernel Code Segment Selector (0x08)
    uint8_t  ist;           // Interrupt Stack Table (offset in TSS)
    uint8_t  attributes;    // Type and attributes (Present, DPL, Gate Type)
    uint16_t isr_mid;       // Middle 16 bits of handler address
    uint32_t isr_high;      // Upper 32 bits of handler address
    uint32_t reserved;
} __attribute__((packed));

struct idtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

// The struct usually passed to your C-level handler to see CPU state
struct interrupt_frame {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss; // Pushed by CPU automatically
};

void idt_init(void);

#endif