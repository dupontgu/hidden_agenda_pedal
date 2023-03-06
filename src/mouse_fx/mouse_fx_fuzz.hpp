#include "custom_hid.hpp"
#include "tusb.h"

class MouseFuzz : public IMouseFx {
 public:
  MouseFuzz() {}

  void initialize() { log_line("mouse fuzz init"); }

  uint32_t get_indicator_color() { return urgb_u32(200, 0, 0); }

  uint32_t get_current_pixel_value(uint32_t frame) {
    return get_indicator_color();
  }

  void update_parameter(float percentage) {}

  void process_mouse_report(hid_mouse_report_t const *report) {
    int8_t pos_vals[] = {0,   -2, -17, 4,  20,  6,  35, 50,
                         -10, 1,  66,  11, -20, 22, 10, 120};
    int8_t neg_vals[] = {0,  2,  17,  -4,  -20, -6,  -35, -50,
                         10, -1, -66, -11, 20,  -22, -10, -120};
    int8_t x = 0;
    uint8_t x_index = (get_random_byte() & 0b00001111);
    int8_t scalar = 2;
    if (report->x > 0) {
      x = pos_vals[x_index] / scalar;
    } else if (report->x < 0) {
      x = neg_vals[x_index] / scalar;
    }
    uint8_t y = 0;
    uint8_t y_index = (get_random_byte() & 0b00001111);
    if (report->y > 0) {
      y = pos_vals[y_index] / scalar;
    } else if (report->y < 0) {
      y = neg_vals[y_index] / scalar;
    }
    log_line("x %i", x);
    send_mouse_report(report->buttons, x, y, report->wheel, report->pan);
  }
};
