#include <stdint.h>

#include "hid_output.hpp"
#include "util.h"
class TestHIDOutput : public IHIDOutput {
 public:
  TestHIDOutput() {}
  void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                         int8_t pan, bool process = false) {
    log_line("m report %d %d %d", x, y, buttons);
  }
  void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                            const uint8_t keycode[6]) {
    (void)reserved;
    log_line("k report %u %u %u %u %u %u", keycode[0], keycode[1], keycode[2],
             keycode[3], keycode[4], keycode[5]);
  }
};