#ifndef OPENWAC_BOOT_HAL_H
#define OPENWAC_BOOT_HAL_H

#include <stdint.h>

#if defined(__wasm__) && defined(__clang__)
#define BOOT_IMPORT(name) \
  __attribute__((import_module("hal"), import_name(#name))) name
#define BOOT_EXPORT_START \
  __attribute__((export_name("_start"))) void _start(void)
#else
#define BOOT_IMPORT(name) name
#define BOOT_EXPORT_START void _start(void)
#endif

int32_t BOOT_IMPORT(hal_device_count)(void);
int32_t BOOT_IMPORT(hal_device_info)(int32_t index, void *output,
                                     int32_t output_capacity);
int32_t BOOT_IMPORT(hal_device_call)(const uint8_t uuid[16],
                                     int32_t operation,
                                     const void *arguments,
                                     int32_t arguments_len,
                                     void *response,
                                     int32_t response_capacity);
int32_t BOOT_IMPORT(hal_install_kernel)(const void *wasm, int32_t wasm_len);
void BOOT_IMPORT(hal_yield)(void);

static inline uint32_t boot_read_le32(const uint8_t *p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void boot_write_le32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

static inline int boot_response_ok(int32_t result, uint32_t response_size) {
  /* The production HAL returns status zero; the simulator may return the
     response byte count. */
  return result == 0 || result == (int32_t)response_size;
}

#endif
