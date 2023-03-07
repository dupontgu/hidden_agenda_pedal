#include "custom_hid.hpp"
#include "tusb.h"

class KeyboardPassthrough : public IKeyboardFx {
 public:
  KeyboardPassthrough() {}

  void initialize() { log_line("keyboard trem init"); }

  uint32_t get_indicator_color() { return urgb_u32(0, 200, 0); }

  uint32_t get_current_pixel_value(uint32_t frame) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {}

  void process_keyboard_report(hid_keyboard_report_t const *report) {
    send_keyboard_report(report->modifier, report->reserved, report->keycode);
  }
};
