#ifndef PMM_H
#define PMM_H
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

void pmm_init(struct limine_memmap_response *map, uint64_t hhdm_offset);
void *pmm_alloc_page(void);
void pmm_free_page(void *page);
size_t pmm_get_free_page_count(void);

#define PAGE_SIZE 4096
#define DIV_ROUND_UP(a, b) (((a) + (b) - 1) / (b))
#define BITMAP_SET(bit) (bitmap[(bit) / 8] |= (1 << ((bit) % 8)))
#define BITMAP_UNSET(bit) (bitmap[(bit) / 8] &= ~(1 << ((bit) % 8)))
#define BITMAP_TEST(bit) (bitmap[(bit) / 8] & (1 << ((bit) % 8)))
 
#endif // PMM_H