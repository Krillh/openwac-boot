#ifndef OPENWAC_BOOT_MEMORY_H
#define OPENWAC_BOOT_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#define BOOT_WASM_PAGE_SIZE 65536u
#define BOOT_MAX_KERNEL_BYTES (4u * BOOT_WASM_PAGE_SIZE)
/* TBDF needs 64 bytes beyond one full block for its validation scratch. */
#define BOOT_MAX_BLOCK_SIZE (BOOT_WASM_PAGE_SIZE - 64u)

typedef struct boot_arena {
  size_t cursor;
} boot_arena;

typedef struct boot_image {
  boot_arena *arena;
  uint8_t *data;
  size_t capacity;
  size_t size;
} boot_image;

int boot_arena_init(boot_arena *arena);
void *boot_arena_alloc(boot_arena *arena, size_t size, size_t alignment);
int boot_image_reserve(boot_image *image, size_t size);
size_t boot_memory_pages(void);

#endif
