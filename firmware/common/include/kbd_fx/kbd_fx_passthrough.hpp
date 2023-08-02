#include "custom_hid.hpp"

class KeyboardPassthrough : public IKeyboardFx {
 public:
  KeyboardPassthrough() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return indicator_color;
  }

  void update_parameter(float percentage) { (void)percentage; }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_keyboard_report(ha_keyboard_report_t const *report,
                               uint32_t time_ms) {
    (void)time_ms;
    send_keyboard_report(report->modifier, report->reserved, report->keycode);
  }
};
