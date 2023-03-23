#include <math.h>

#include "custom_hid.hpp"
#include "tusb.h"

#define SHIFT_FLAG 0b00100000
#define DUTY_CYCLE_MAX 0xFFFF

class KeyboardTremolo : public IKeyboardFx {
  uint32_t off_color = 0;
  uint32_t indicator_color = 0;
  uint16_t duty_cycle_ms = 0;
  bool sarcastic_mode = false;
  bool timer_engaged = false;

 public:
  KeyboardTremolo() {
    indicator_color = urgb_u32(150, 0, 40);
    off_color = color_at_brightness(get_indicator_color(), 0.3);
  }

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
    log_line("keyboard trem init %u", 1);
  }

  uint32_t get_indicator_color() { return urgb_u32(150, 0, 40); }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return timer_engaged ? get_indicator_color() : off_color;
  }

  void update_parameter(float percentage) {
    uint8_t int_p = round(percentage * 100);
    if (int_p < 5) {
      sarcastic_mode = true;
      return;
    }
    sarcastic_mode = false;
    switch (int_p) {
      case 5 ... 20:
        duty_cycle_ms = 50;
        break;
      case 21 ... 35:
        duty_cycle_ms = 250;
        break;
      case 36 ... 50:
        duty_cycle_ms = 500;
        break;
      case 51 ... 70:
        duty_cycle_ms = 1000;
        break;
      case 71 ... 85:
        duty_cycle_ms = 2000;
        break;
      default:
        duty_cycle_ms = DUTY_CYCLE_MAX;
        break;
    }
  }

  void tick(uint32_t time_ms) {
    if (!sarcastic_mode) {
      if (duty_cycle_ms == DUTY_CYCLE_MAX) {
        timer_engaged = true;
      } else {
        timer_engaged = (time_ms % (2 * duty_cycle_ms)) > duty_cycle_ms;
      }
    }
  }

  void deinit() {}

  void process_keyboard_report(hid_keyboard_report_t const *report,
                               uint32_t time_ms) {
    (void)time_ms;
    uint8_t modifier_flag = 0;
    if (sarcastic_mode) {
      timer_engaged = (get_random_byte() & 0b01);
    }
    modifier_flag |= timer_engaged ? SHIFT_FLAG : 0;
    send_keyboard_report(report->modifier | modifier_flag, report->reserved,
                         report->keycode);
  }
};
