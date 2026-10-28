#include "boot_fs.h"

#include "tfaf1.h"

static tfaf_volume volume;
static int volume_initialized;

static int tfaf_read(void *context, uint32_t block, uint8_t *output,
                     size_t capacity, int32_t *raw_status) {
  int result = boot_disk_read_block(context, block, output, capacity);
  if (raw_status) *raw_status = result == 0 ? 0 : -1;
  return result;
}

static boot_fs_result load_kernel(const boot_disk *disk, uint8_t *workspace,
                                  size_t workspace_size, boot_image *image) {
  static const uint8_t kernel_name[] = "kernel.x";
  tfaf_reader reader;
  tfaf_file file;
  size_t bytes_read = 0;
  tfaf_status status;

  if (!disk || !workspace || !image ||
      workspace_size < disk->block_size)
    return BOOT_FS_SKIP;
  if (!volume_initialized) {
    tfaf1_init(&volume);
    volume_initialized = 1;
  }

  reader.read = tfaf_read;
  reader.context = (void *)disk;
  reader.block_size = disk->block_size;
  reader.block_count = disk->block_count;
  status = tfaf1_mount(&volume, &reader, workspace, workspace_size, 0);
  if (status != TFAF_OK) return BOOT_FS_SKIP;

  status = tfaf1_lookup(&volume, kernel_name, sizeof(kernel_name) - 1,
                         &file);
  if (status != TFAF_OK || file.size < 8 ||
      file.size > BOOT_MAX_KERNEL_BYTES)
    goto skip;
  if (!boot_image_reserve(image, file.size)) goto skip;

  status = tfaf1_pread(&volume, &file, 0, image->data, file.size,
                        &bytes_read);
  if (status != TFAF_OK || bytes_read != file.size) goto skip;
  image->size = file.size;
  tfaf1_unmount(&volume);
  return BOOT_FS_IMAGE_READY;

skip:
  tfaf1_unmount(&volume);
  return BOOT_FS_SKIP;
}

const boot_fs_driver boot_fs_tfaf = {
    .name = "TFAF1",
    .load_kernel = load_kernel,
};
