#ifndef OPENWAC_TEXT_DISPLAY_H
#define OPENWAC_TEXT_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

typedef struct text_display_ops text_display_ops;

typedef struct text_display {
  const text_display_ops *ops;
  uint8_t uuid[16];
  uint32_t columns;
  uint32_t rows;
  uint32_t next_row;
} text_display;

#define TEXT_DISPLAY_MIN_BOOT_LINES 5u

struct text_display_ops {
  uint32_t interface_id;
  int (*open)(text_display *display, const uint8_t uuid[16]);
  int (*append_line)(text_display *display, const char *text, size_t length);
};

int text_display_discover(text_display *display);
int text_display_append_line(text_display *display, const char *text,
                             size_t length);

#endif
