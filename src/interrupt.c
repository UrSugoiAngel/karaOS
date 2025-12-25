#include <idt.h>
#include <util.h> // debug_print

// This is called from Assembly
void exception_handler(struct interrupt_frame *frame) {
    // 1. CPU Exceptions (0-31)
    if (frame->int_no < 32) {
        debug_print("CPU EXCEPTION! Halting.\n");
        // Print CR2 if page fault, dump regs, etc.
        hcf(); 
    }

    // 2. Hardware Interrupts (32+)
    if (frame->int_no >= 32) {
        int irq = frame->int_no - 32;

        if (irq == 0) {
            // Timer Tick - Call Scheduler!
            // schedule();
        }
        else if (irq == 1) {
            // Keyboard - In Microkernel, we DO NOT read the scancode here.
            // We verify the interrupt happened, send an IPC message to Driver(PID 4),
            // and unblock it.
            // msg_send(DRIVER_KEYBOARD, MSG_IRQ, 1);
        }

        // Acknowledge PIC (EOI)
        if (irq >= 8) outb(0xA0, 0x20); // Slave
        outb(0x20, 0x20);               // Master
    }
}