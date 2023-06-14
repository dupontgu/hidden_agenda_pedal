#include "custom_hid.hpp"

#define MOUSE_XOVER_THROTTLE_MAX 200

// Should match https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
#ifndef HID_KEY_A
#define HID_KEY_A 0x04
#define HID_KEY_0 0x27
#define HID_KEY_BACKSPACE 0x2A
#endif

class MouseXOver : public IMouseFx {
 private:
  enum State {
    idle,
    backspace_press,
    backspace_release,
    key_press,
    key_release
  };
  State state = idle;
  float brightness;
  uint16_t throttle;
  uint8_t current_key = HID_KEY_A;
  bool skip_backspace = true;

 public:
  MouseXOver() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    update_parameter(param_percentage);
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) {
    throttle = (uint16_t)(percentage * (float)MOUSE_XOVER_THROTTLE_MAX);
    if (throttle < 3) {
        throttle = 3;
    }
  }

  void tick(uint32_t time_ms) {
    static uint16_t out_count = 0;
    static uint8_t key_buf[6] = {0, 0, 0, 0, 0, 0};
    if (out_count++ < 10000) {
        return;
    }
    out_count = 0;
    if (state == idle) return;
    skip_backspace = false;
    if (state == backspace_press) {
      key_buf[0] = HID_KEY_BACKSPACE;
      state = backspace_release;
    } else if (state == backspace_release) {
      key_buf[0] = 0;
      state = key_press;
    } else if (state == key_press) {
      key_buf[0] = current_key;
      state = key_release;
    } else if (state == key_release) {
      key_buf[0] = 0;
      state = idle;
    }
    send_keyboard_report(0, 0, key_buf);
  }

  void deinit() {}

  void process_mouse_report(ha_mouse_report_t const *report,
                            uint32_t time_ms) {
    (void)time_ms;
    if (report->buttons & 0b10) {
      skip_backspace = true;
    }
    if (state != idle) return;
    if (report->wheel < 0 || report->y < 0) {
      current_key--;
      state = skip_backspace ? key_press : backspace_press;
    } else if (report->wheel > 0 || report->y > 0) {
      current_key++;
      state = skip_backspace ? key_press : backspace_press;
    }

    if (current_key < HID_KEY_A) {
      current_key = HID_KEY_0;
    } else if (current_key > HID_KEY_0) {
      current_key = HID_KEY_A;
    }
  }
};
