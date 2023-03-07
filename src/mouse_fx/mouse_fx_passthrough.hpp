#include "custom_hid.hpp"
#include "tusb.h"

class MousePassthrough : public IMouseFx {
 public:
  MousePassthrough() {}

  void initialize() { log_line("Mouse Passthrough Initialized..."); }

  uint32_t get_indicator_color() { return urgb_u32(100, 50, 100); }

  uint32_t get_current_pixel_value(uint32_t frame) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {}

  void process_mouse_report(hid_mouse_report_t const *report) {
    log_line("mse btns: %u, x: %i, y: %i, whl: %i, pan: %i", report->buttons, report->x, report->y, report->wheel, report->pan);
    // send_mouse_report(report->buttons, report->x, report->y, report->wheel, report->pan);
  }
};
