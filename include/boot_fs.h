#ifndef OPENWAC_BOOT_FS_H
#define OPENWAC_BOOT_FS_H

#include "boot_disk.h"
#include "boot_memory.h"

#include <stddef.h>

typedef enum boot_fs_result {
  BOOT_FS_SKIP = 0,
  BOOT_FS_IMAGE_READY = 1
} boot_fs_result;

typedef struct boot_fs_driver {
  const char *name;
  boot_fs_result (*load_kernel)(const boot_disk *disk, uint8_t *workspace,
                                size_t workspace_size, boot_image *image);
} boot_fs_driver;

extern const boot_fs_driver boot_fs_tbdf;
extern const boot_fs_driver boot_fs_tfaf;

int boot_fs_tbdf_init(boot_arena *arena);

#endif
