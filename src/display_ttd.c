#include "text_display.h"

#include "boot_hal.h"

#define TTD_GEOMETRY 0x00
#define TTD_READ_CELLS 0x10
#define TTD_WRITE_CELLS 0x20
#define TTD_MAX_COLUMNS 60u
#define TTD_ROWS 30u

static int call_no_response(text_display *display, int32_t operation,
                            const void *arguments, int32_t arguments_len) {
  static const uint8_t empty = 0;
  return hal_device_call(display->uuid, operation, arguments, arguments_len,
                         (void *)&empty, 0) == 0;
}

static int ttd_read_row(const uint8_t uuid[16], uint32_t columns,
                        uint32_t row, uint8_t cells[TTD_MAX_COLUMNS]) {
  uint8_t arguments[16];
  int32_t result;
  boot_write_le32(arguments, 0);
  boot_write_le32(arguments + 4, row);
  boot_write_le32(arguments + 8, columns);
  boot_write_le32(arguments + 12, 1);
  result = hal_device_call(uuid, TTD_READ_CELLS, arguments,
                           (int32_t)sizeof(arguments), cells,
                           (int32_t)columns);
  return boot_response_ok(result, columns);
}

static int ttd_open(text_display *display, const uint8_t uuid[16]) {
  uint8_t geometry[8];
  uint8_t row_cells[TTD_MAX_COLUMNS];
  uint32_t columns, rows, next_row = 0;
  static const uint8_t empty = 0;
  int32_t result;

  for (size_t i = 0; i < 16; ++i) display->uuid[i] = uuid[i];
  result = hal_device_call(uuid, TTD_GEOMETRY, &empty, 0, geometry, 8);
  if (!boot_response_ok(result, 8)) return 0;
  columns = boot_read_le32(geometry);
  rows = boot_read_le32(geometry + 4);
  if (columns != TTD_MAX_COLUMNS || rows != TTD_ROWS) return 0;

  for (uint32_t y = rows; y != 0; --y) {
    uint32_t row_index = y - 1;
    int occupied = 0;
    if (!ttd_read_row(uuid, columns, row_index, row_cells)) return 0;
    for (uint32_t x = 0; x < columns; ++x)
      if (row_cells[x] != ' ') occupied = 1;
    if (occupied) {
      next_row = row_index + 1;
      break;
    }
  }

  display->columns = columns;
  display->rows = rows;
  display->next_row = next_row;
  if (rows - next_row < TEXT_DISPLAY_MIN_BOOT_LINES) return 0;
  return 1;
}

static int ttd_append_line(text_display *display, const char *text,
                           size_t length) {
  uint8_t arguments[16 + TTD_MAX_COLUMNS];
  size_t copied = length;
  if (display->next_row >= display->rows) return 0;
  if (copied > display->columns) copied = display->columns;

  boot_write_le32(arguments, 0);
  boot_write_le32(arguments + 4, display->next_row);
  boot_write_le32(arguments + 8, (uint32_t)copied);
  boot_write_le32(arguments + 12, 1);
  for (size_t i = 0; i < copied; ++i)
    arguments[16 + i] = (uint8_t)text[i];
  if (!call_no_response(display, TTD_WRITE_CELLS, arguments,
                        (int32_t)(16 + copied)))
    return 0;
  ++display->next_row;
  return 1;
}

const text_display_ops text_display_ttd_ops = {
    .interface_id = 0x0400u,
    .open = ttd_open,
    .append_line = ttd_append_line,
};
