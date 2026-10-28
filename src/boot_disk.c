#include "boot_disk.h"

#include "boot_hal.h"
#include "boot_memory.h"

#define HAL_INTERFACE_DISK 0x0100u
#define DISK_BLOCK_SIZE 0x10
#define DISK_BLOCK_COUNT 0x11
#define DISK_READ_BLOCK 0x20
#define HAL_MAX_DEVICE_CALL_BYTES (256u * 1024u)

static int device_info(int32_t index, uint8_t interface_and_uuid[20]) {
  return hal_device_info(index, interface_and_uuid, 20) == 0;
}

static int read_geometry_value(const uint8_t uuid[16], int32_t operation,
                               uint32_t *value) {
  uint8_t response[4];
  int32_t result;
  static const uint8_t empty = 0;
  result = hal_device_call(uuid, operation, &empty, 0, response, 4);
  if (!boot_response_ok(result, 4)) return 0;
  *value = boot_read_le32(response);
  return 1;
}

int boot_disk_max_block_size(uint32_t *block_size) {
  int32_t count;
  uint32_t maximum = 0;
  if (!block_size) return 0;
  count = hal_device_count();
  if (count < 0) return 0;

  for (int32_t i = 0; i < count; ++i) {
    uint8_t info[20];
    uint32_t size;
    if (!device_info(i, info) || boot_read_le32(info) != HAL_INTERFACE_DISK)
      continue;
    if (!read_geometry_value(info + 4, DISK_BLOCK_SIZE, &size)) continue;
    if (size < 16 || size > BOOT_MAX_BLOCK_SIZE ||
        size > HAL_MAX_DEVICE_CALL_BYTES - 4u)
      continue;
    if (size > maximum) maximum = size;
  }
  *block_size = maximum;
  return 1;
}

int boot_disk_open(const uint8_t uuid[16], boot_disk *disk) {
  uint32_t block_size, block_count;
  if (!uuid || !disk ||
      !read_geometry_value(uuid, DISK_BLOCK_SIZE, &block_size) ||
      !read_geometry_value(uuid, DISK_BLOCK_COUNT, &block_count) ||
      block_size < 16 || block_size > BOOT_MAX_BLOCK_SIZE ||
      block_size > HAL_MAX_DEVICE_CALL_BYTES - 4u ||
      block_count == 0)
    return 0;
  for (size_t i = 0; i < sizeof(disk->uuid); ++i) disk->uuid[i] = uuid[i];
  disk->block_size = block_size;
  disk->block_count = block_count;
  return 1;
}

int boot_disk_read_block(void *context, uint32_t block, uint8_t *output,
                         size_t length) {
  boot_disk *disk = (boot_disk *)context;
  uint8_t argument[4];
  int32_t result;
  if (!disk || !output || block >= disk->block_count ||
      length != disk->block_size || length > INT32_MAX ||
      length + sizeof(argument) > HAL_MAX_DEVICE_CALL_BYTES)
    return -1;
  boot_write_le32(argument, block);
  result = hal_device_call(disk->uuid, DISK_READ_BLOCK, argument,
                           (int32_t)sizeof(argument), output,
                           (int32_t)length);
  return boot_response_ok(result, (uint32_t)length) ? 0 : -1;
}
