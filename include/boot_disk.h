#ifndef OPENWAC_BOOT_DISK_H
#define OPENWAC_BOOT_DISK_H

#include <stddef.h>
#include <stdint.h>

typedef struct boot_disk {
  uint8_t uuid[16];
  uint32_t block_size;
  uint32_t block_count;
} boot_disk;

/* Finds the largest block size advertised by a currently attached disk that
   can be called through the HAL's 256 KiB request/response work bound. */
int boot_disk_max_block_size(uint32_t *block_size);
int boot_disk_open(const uint8_t uuid[16], boot_disk *disk);
int boot_disk_read_block(void *context, uint32_t block, uint8_t *output,
                         size_t length);

#endif
