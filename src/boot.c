#include "boot_disk.h"
#include "boot_fs.h"
#include "boot_hal.h"
#include "boot_memory.h"
#include "text_display.h"

#define HAL_INTERFACE_DISK 0x0100u

static text_display primary_display;
static int has_primary_display;

_Noreturn void abort(void) { __builtin_trap(); }

_Noreturn static void panic(void) {
  for (;;) hal_yield();
}

static void progress(const char *text) {
  if (has_primary_display) {
    size_t length = 0;
    while (text[length] != '\0') ++length;
    (void)text_display_append_line(&primary_display, text, length);
  }
}

_Noreturn static void fail(const char *message) {
  if (has_primary_display) {
    progress(message);
    panic();
  }
  abort();
}

static int try_driver_on_disks(const boot_fs_driver *driver,
                               uint8_t *workspace, size_t workspace_size,
                               boot_image *image) {
  int32_t count = hal_device_count();
  if (count < 0) return -1;

  for (int32_t i = 0; i < count; ++i) {
    uint8_t info[20];
    boot_disk disk;
    boot_fs_result result;
    int32_t install_status;
    if (hal_device_info(i, info, (int32_t)sizeof(info)) != 0) continue;
    if (boot_read_le32(info) != HAL_INTERFACE_DISK) continue;
    if (!boot_disk_open(info + 4, &disk)) continue;
    if (!workspace || workspace_size < (size_t)disk.block_size) continue;

    result = driver->load_kernel(&disk, workspace, workspace_size, image);
    if (result != BOOT_FS_IMAGE_READY) continue;
    if (image->size < 8 || image->size > BOOT_MAX_KERNEL_BYTES ||
        image->size > INT32_MAX)
      continue;

    install_status = hal_install_kernel(image->data, (int32_t)image->size);
    if (install_status == 0) return 1;
    /* Invalid WASM belongs to this candidate; other statuses describe a
       global install/caller failure and cannot improve on another disk. */
    if (install_status != -16) return -1;
  }
  return 0;
}

BOOT_EXPORT_START {
  boot_arena arena;
  boot_image image;
  uint32_t max_block_size = 0;
  size_t workspace_size = 0;
  uint8_t *workspace = 0;
  int result;

  image.arena = &arena;
  image.data = 0;
  image.capacity = 0;
  image.size = 0;
  has_primary_display = text_display_discover(&primary_display);
  progress("OpenWAC bootloader");
  if (!boot_arena_init(&arena)) fail("ERROR: memory unavailable");
  progress("Checking disk geometry");
  if (!boot_disk_max_block_size(&max_block_size))
    fail("ERROR: device enumeration failed");
  if (max_block_size != 0) {
    if ((size_t)max_block_size > SIZE_MAX - 64u)
      fail("ERROR: disk geometry too large");
    workspace_size = (size_t)max_block_size + 64u;
    workspace = (uint8_t *)boot_arena_alloc(&arena, workspace_size, 16);
    if (!workspace) fail("ERROR: disk workspace unavailable");
    (void)boot_fs_tbdf_init(&arena);
  }

  progress("Searching TBDF disks");
  result = try_driver_on_disks(&boot_fs_tbdf, workspace, workspace_size,
                               &image);
  if (result < 0) fail("ERROR: kernel installation failed");
  if (result > 0) {
    progress("Kernel installed");
    return;
  }

  progress("Searching TFAF disks");
  result = try_driver_on_disks(&boot_fs_tfaf, workspace, workspace_size,
                               &image);
  if (result < 0) fail("ERROR: kernel installation failed");
  if (result > 0) {
    progress("Kernel installed");
    return;
  }

  fail("ERROR: no installable root kernel.x found");
}
