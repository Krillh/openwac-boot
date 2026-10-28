#include "boot_fs.h"

#include "tbdfv1.h"

static void *volume_storage;
static size_t volume_storage_size;

static int tbdf_read(void *context, uint32_t block, uint8_t *output,
                     size_t length) {
  return boot_disk_read_block(context, block, output, length);
}

int boot_fs_tbdf_init(boot_arena *arena) {
  size_t alignment;
  if (!arena) return 0;
  volume_storage_size = tbdf_volume_storage_size();
  alignment = tbdf_volume_storage_align();
  if (volume_storage_size == 0 || alignment == 0) return 0;
  volume_storage = boot_arena_alloc(arena, volume_storage_size, alignment);
  return volume_storage != 0;
}

static boot_fs_result load_kernel(const boot_disk *disk, uint8_t *workspace,
                                  size_t workspace_size, boot_image *image) {
  static const uint8_t kernel_name[] = "kernel.x";
  tbdf_device device;
  tbdf_file_info file;
  size_t bytes_read = 0;
  tbdf_status status;
  int opened = 0;

  if (!disk || !workspace || !image || !volume_storage ||
      workspace_size < (size_t)disk->block_size + 64u)
    return BOOT_FS_SKIP;

  device.context = (void *)disk;
  device.block_size = disk->block_size;
  device.block_count = disk->block_count;
  device.read_block = tbdf_read;
  device.write_block = 0;
  status = tbdf_volume_open(volume_storage, volume_storage_size, &device,
                            workspace, workspace_size);
  if (status != TBDF_OK) return BOOT_FS_SKIP;
  opened = 1;

  status = tbdf_lookup(volume_storage, 0, kernel_name,
                       sizeof(kernel_name) - 1, &file);
  if (status != TBDF_OK || file.kind != TBDF_FILE_REGULAR ||
      file.regular_size < 8 || file.regular_size > BOOT_MAX_KERNEL_BYTES)
    goto skip;
  if (!boot_image_reserve(image, file.regular_size)) goto skip;

  status = tbdf_read_at(volume_storage, file.id, 0, image->data,
                        file.regular_size, &bytes_read);
  if (status != TBDF_OK || bytes_read != file.regular_size) goto skip;
  image->size = file.regular_size;
  (void)tbdf_volume_close(volume_storage, volume_storage_size);
  return BOOT_FS_IMAGE_READY;

skip:
  if (opened) (void)tbdf_volume_close(volume_storage, volume_storage_size);
  return BOOT_FS_SKIP;
}

const boot_fs_driver boot_fs_tbdf = {
    .name = "TBDF",
    .load_kernel = load_kernel,
};
