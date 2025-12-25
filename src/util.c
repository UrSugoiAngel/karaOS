#include <util.h>

void *memcpy(void *__restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

// Halt and catch fire function.
void hcf(void) {
    for (;;) {
        asm ("hlt");
    }
}

// Port I/O helpers (usually in standard headers, but putting here for speed)
void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
}

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ( "inb %1, %0" : "=a"(ret) : "Nd"(port) );
    return ret;
}

// Minimal Serial Driver
void serial_init() {
    outb(0x3f8 + 1, 0x00);    // Disable all interrupts
    outb(0x3f8 + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(0x3f8 + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(0x3f8 + 1, 0x00);    //                  (hi byte)
    outb(0x3f8 + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(0x3f8 + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold
    outb(0x3f8 + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void debug_putc(char c) {
    while ((inb(0x3f8 + 5) & 0x20) == 0); // Wait for transmit buffer to be empty
    outb(0x3f8, c);
}

void debug_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        outb(0x3F8, str[i]); // Simple outb usually works for QEMU stdio
    }
}