#include "custom_hid.hpp"

class MousePassthrough : public IMouseFx {
 private:
  float brightness;

 public:
  MousePassthrough() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) { brightness = percentage; }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_mouse_report(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    send_mouse_report(report->buttons, report->x, report->y, report->wheel, 0);
  }
};
