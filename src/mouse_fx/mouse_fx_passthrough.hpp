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
    (void)time_ms;
    (void)param_percentage;
    log_line("Mouse Passthrough init");
  }

  uint32_t get_indicator_color() { return indicator_color; }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) { brightness = percentage; }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_mouse_report(hid_mouse_report_t const *report,
                            uint32_t time_ms) {
    (void)time_ms;
    send_mouse_report(report->buttons, report->x, report->y, report->wheel, 0);
  }
};
