#include "custom_hid.hpp"
#include "tusb.h"

class KeyboardPassthrough : public IKeyboardFx {
 public:
  KeyboardPassthrough() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    log_line("keyboard passthrough init %u", 1);
  }

  uint32_t get_indicator_color() { return urgb_u32(0, 200, 0); }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {}

  void tick(uint32_t time_ms) {}

  void process_keyboard_report(hid_keyboard_report_t const *report) {
    send_keyboard_report(report->modifier, report->reserved, report->keycode);
  }
};
