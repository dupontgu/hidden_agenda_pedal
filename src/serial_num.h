#include <stdio.h>
#include "pico/unique_id.h"

// 6 bytes for "HAXXX-"
#define SERIAL_NUMBER_OFFSET 6

const char *get_serial_number() {
  // 16 chars for 64 bit uid, 1 for terminating 0
  static char serial_number_buffer[SERIAL_NUMBER_OFFSET + 16 + 1] = {0};
  pico_unique_board_id_t board_id;
  pico_get_unique_board_id(&board_id);
  sprintf((char *)serial_number_buffer, "HA001-");
  for (size_t i = 0; i < 8; i++) {
    char* adj_i = (char *)serial_number_buffer + SERIAL_NUMBER_OFFSET + (i * 2);
    sprintf(adj_i, "%02x", board_id.id[i]);
  }
  return serial_number_buffer;
}