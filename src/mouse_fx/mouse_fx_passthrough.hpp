#include "custom_hid.hpp"
#include "tusb.h"

class MousePassthrough : public IMouseFx {
 private:
  uint32_t indicator_color;
  float brightness;

 public:
  MousePassthrough(uint32_t color) {
    indicator_color = color;
    brightness = 0.0f;
  }

  void initialize(uint32_t time_ms, float param_percentage) {
    log_line("Mouse Passthrough Initialized, color: %u", indicator_color);
  }

  uint32_t get_indicator_color() { return indicator_color; }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) { brightness = percentage; }

  void tick(uint32_t time_ms) {}

  void process_mouse_report(hid_mouse_report_t const *report) {
    log_line("mse btns: %u, x: %i, y: %i, whl: %i, pan: %i", report->buttons,
             report->x, report->y, report->wheel, report->pan);
    // send_mouse_report(report->buttons, report->x, report->y, report->wheel,
    // report->pan);
  }
};
