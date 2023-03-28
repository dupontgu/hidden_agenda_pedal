#include "custom_hid.hpp"
#include "tusb.h"

class KeyboardPassthrough : public IKeyboardFx {
  uint32_t indicator_color;

 public:
  KeyboardPassthrough() { indicator_color = urgb_u32(0, 200, 0); }

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
    log_line("k pass init");
  }

  uint32_t get_indicator_color() { return indicator_color; }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return indicator_color;
  }

  void update_parameter(float percentage) { (void)percentage; }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_keyboard_report(hid_keyboard_report_t const *report,
                               uint32_t time_ms) {
    (void)time_ms;
    send_keyboard_report(report->modifier, report->reserved, report->keycode);
  }
};
