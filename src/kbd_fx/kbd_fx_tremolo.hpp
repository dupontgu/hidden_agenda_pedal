#include "custom_hid.hpp"
#include "tusb.h"

#define SHIFT_FLAG 0b00100000

class KeyboardTremolo : public IKeyboardFx {
  bool sarcastic_mode = false;
 public:
  KeyboardTremolo() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    log_line("keyboard trem init %u", 1);
  }

  uint32_t get_indicator_color() { return urgb_u32(200, 0, 0); }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {
    if (percentage < 0.05) {
      log_line("sarcastic mode on");
      sarcastic_mode = true;
      return;
    }
    sarcastic_mode = false;
  }

  void tick(uint32_t time_ms) {}

  void process_keyboard_report(hid_keyboard_report_t const *report) {
    uint8_t modifier_flag = 0;
    if (sarcastic_mode) {
      modifier_flag |= (get_random_byte() & 0b01) ? 0 : SHIFT_FLAG;
    }
    send_keyboard_report(report->modifier | modifier_flag, report->reserved,
                         report->keycode);
  }
};
