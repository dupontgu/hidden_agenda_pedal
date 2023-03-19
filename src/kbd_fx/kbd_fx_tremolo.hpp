#include "custom_hid.hpp"
#include "tusb.h"

class KeyboardTremolo : public IKeyboardFx {
 public:
  KeyboardTremolo() {}

  void initialize(uint32_t time_ms) { log_line("keyboard trem init %u", 1); }

  uint32_t get_indicator_color() { return urgb_u32(200, 0, 0); }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {}

  void tick(uint32_t time_ms) {}

  void process_keyboard_report(hid_keyboard_report_t const *report) {
    log_line("oops");
    send_keyboard_report(report->modifier | 0b00100000, report->reserved, report->keycode);
  }
};
