#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include <stddef.h>

void *memcpy(void *__restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

void hcf(void);

void serial_init();
void debug_putc(char c);
void debug_print(const char* str);
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);

#endif // UTIL_H