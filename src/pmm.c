#include <pmm.h>
#include <limine.h>
#include <util.h>

static uint8_t *bitmap = NULL;      // Virtual pointer to the bitmap
static uint64_t highest_addr = 0;   // Highest physical page address
static size_t free_memory = 0;      // Tracked for get_free_page_count()
// static uint64_t hhdm_offset = 0;    // From Limine
static uint64_t bitmap_size = 0;    // Size of bitmap table
static uint64_t last_alloc_index = 0; // To optimize page lookup


void pmm_init(struct limine_memmap_response *map, uint64_t hhdm_offset){

    // find the highest memory address to determine bitmap size
    for(uint64_t i=0; i<map->entry_count; i++){
        struct limine_memmap_entry cur_pg = *(map->entries[i]);
        if(cur_pg.base + cur_pg.length > highest_addr){
            highest_addr = cur_pg.base + cur_pg.length;
            uint64_t highest_pfn = (highest_addr + PAGE_SIZE - 1) / PAGE_SIZE;
            bitmap_size = (highest_pfn + 7) / 8;
        }
    }

    // find a usable memory region large enough to hold the bitmap
    for(uint64_t i=0; i<map->entry_count; i++){
        struct limine_memmap_entry cur_pg = *(map->entries[i]);
        if(cur_pg.length >= bitmap_size && cur_pg.type == LIMINE_MEMMAP_USABLE){
            uint64_t bitmap_phys = cur_pg.base;
            bitmap = (uint8_t*)(bitmap_phys + hhdm_offset);
            break;
        }
    }

    if(bitmap == NULL){
        hcf();
    }

    // initialize all pages as used/reserved
    memset(bitmap, 0xFF, bitmap_size);
    
    for (uint64_t i = 0; i < map->entry_count; i++) {
        struct limine_memmap_entry *entry = map->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            //calculate which pages this entry covers; take floor w int div
            uint64_t start_pfn = entry->base / PAGE_SIZE;
            uint64_t page_count = entry->length / PAGE_SIZE;

            for (uint64_t j = 0; j < page_count; j++) {
                uint64_t pfn = start_pfn + j;
                bitmap[pfn / 8] &= ~(1 << (pfn % 8)); 
                free_memory += PAGE_SIZE;
            }
        }
    }

    //
    //mark bitmap back as used again
    //
    
    //convert the virtual bitmap pointer back to a physical address
    uint64_t bitmap_phys_start = (uint64_t)bitmap - hhdm_offset;
    uint64_t bitmap_phys_end = bitmap_phys_start + bitmap_size;
    
    uint64_t start_pfn = bitmap_phys_start / PAGE_SIZE;
    uint64_t end_pfn = (bitmap_phys_end + PAGE_SIZE - 1) / PAGE_SIZE; //round up

    for (uint64_t i = start_pfn; i < end_pfn; i++) {
        bitmap[i / 8] |= (1 << (i % 8));
        free_memory -= PAGE_SIZE;
    }
}

void *pmm_alloc_page(void){
    for(int i=last_alloc_index; i<bitmap_size; i++){
        if(bitmap[i] != 0xFF){
            uint8_t freebit;
            for(int bit=0; bit<8; bit++){
                if(!BITMAP_TEST((i * 8 + bit))){
                    freebit = bit;
                    break;
                }
            }
            uint64_t phys_addr = (i * 8 + freebit) * PAGE_SIZE;
            free_memory -= 4096;
            bitmap[i] = bitmap[i] | 1 << freebit;

            last_alloc_index = i;
            return (void*)phys_addr;
        }
    }

    for(uint64_t i = 0; i < last_alloc_index; i++){
        if(bitmap[i] != 0xFF){
            uint8_t freebit;
            for(int bit=0; bit<8; bit++){
                if(!BITMAP_TEST((i * 8 + bit))){
                    freebit = bit;
                    break;
                }
            }
            uint64_t phys_addr = (i * 8 + freebit) * PAGE_SIZE;
            free_memory -= 4096;
            bitmap[i] = bitmap[i] | 1 << freebit;

            last_alloc_index = i;
            return (void*)phys_addr;
        }
    }

    return NULL;
}

void pmm_free_page(void *page){
    uint64_t ipage = (uint64_t)page;

    uint64_t page_idx = ipage / PAGE_SIZE;
    BITMAP_UNSET(page_idx);
    free_memory += PAGE_SIZE;
}

size_t pmm_get_free_page_count(void){
    return free_memory / PAGE_SIZE;
}