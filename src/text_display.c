#include "text_display.h"

#include "boot_hal.h"

#define HAL_INTERFACE_TTD 0x0400u

extern const text_display_ops text_display_ttd_ops;

static const text_display_ops *const drivers[] = {&text_display_ttd_ops};

int text_display_discover(text_display *display) {
  int32_t count;
  if (!display) return 0;
  display->ops = 0;
  display->columns = 0;
  display->rows = 0;
  display->next_row = 0;

  count = hal_device_count();
  if (count <= 0) return 0;
  for (int32_t i = 0; i < count; ++i) {
    uint8_t info[20];
    if (hal_device_info(i, info, 20) != 0) continue;
    uint32_t interface_id = boot_read_le32(info);
    for (size_t d = 0; d < sizeof(drivers) / sizeof(drivers[0]); ++d) {
      if (drivers[d]->interface_id != interface_id) continue;
      if (drivers[d]->open(display, info + 4)) {
        display->ops = drivers[d];
        for (size_t j = 0; j < sizeof(display->uuid); ++j)
          display->uuid[j] = info[4 + j];
        return 1;
      }
    }
  }
  return 0;
}

int text_display_append_line(text_display *display, const char *text,
                             size_t length) {
  if (!display || !display->ops || !display->ops->append_line || !text)
    return 0;
  return display->ops->append_line(display, text, length);
}
