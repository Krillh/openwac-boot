#include "boot_memory.h"

#include <stdint.h>

static size_t wasm_pages(void) {
  return __builtin_wasm_memory_size(0);
}

static int ensure_memory_end(size_t end) {
  size_t required_pages = end / BOOT_WASM_PAGE_SIZE;
  if (end % BOOT_WASM_PAGE_SIZE != 0) ++required_pages;

  size_t current_pages = wasm_pages();
  if (required_pages <= current_pages) return 1;

  size_t delta = required_pages - current_pages;
  size_t previous = __builtin_wasm_memory_grow(0, delta);
  return previous != (size_t)-1 && previous == current_pages;
}

int boot_arena_init(boot_arena *arena) {
  size_t pages;
  if (!arena) return 0;
  pages = wasm_pages();
  if (pages > SIZE_MAX / BOOT_WASM_PAGE_SIZE) return 0;
  arena->cursor = pages * BOOT_WASM_PAGE_SIZE;
  return 1;
}

void *boot_arena_alloc(boot_arena *arena, size_t size, size_t alignment) {
  size_t aligned, end;
  if (!arena || size == 0 || alignment == 0 ||
      (alignment & (alignment - 1)) != 0)
    return 0;
  if (arena->cursor > SIZE_MAX - (alignment - 1)) return 0;
  aligned = (arena->cursor + alignment - 1) & ~(alignment - 1);
  if (size > SIZE_MAX - aligned) return 0;
  end = aligned + size;
  if (!ensure_memory_end(end)) return 0;
  arena->cursor = end;
  return (void *)(uintptr_t)aligned;
}

int boot_image_reserve(boot_image *image, size_t size) {
  size_t start, end;
  if (!image || !image->arena || size == 0 ||
      size > BOOT_MAX_KERNEL_BYTES)
    return 0;
  if (image->capacity >= size) return 1;

  if (!image->data) {
    image->data = (uint8_t *)boot_arena_alloc(
        image->arena, size, BOOT_WASM_PAGE_SIZE);
    if (!image->data) return 0;
    image->capacity = size;
    return 1;
  }

  start = (size_t)(uintptr_t)image->data;
  if (image->capacity > SIZE_MAX - start) return 0;
  if (start + image->capacity != image->arena->cursor) return 0;
  if (size > SIZE_MAX - start) return 0;
  end = start + size;
  if (!ensure_memory_end(end)) return 0;
  image->arena->cursor = end;
  image->capacity = size;
  return 1;
}

size_t boot_memory_pages(void) { return wasm_pages(); }
