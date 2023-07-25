#include "custom_hid.hpp"

#define MOUSE_XOVER_THROTTLE_MAX 13000

// Should match
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
#ifndef HID_KEY_A
#define HID_KEY_A 0x04
#define HID_KEY_0 0x27
#define HID_KEY_MINUS 0x2D
#define HID_KEY_SLASH 0x38
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_ARROW_RIGHT 0x4F
#define HID_KEY_ARROW_LEFT 0x50
#define HID_KEY_SPACE 0x2C
#define HID_KEY_ENTER 0x28
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
    float brightness =
        (state != idle && state != backspace_press) ? 0.8f : 0.3f;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) {
    throttle = (uint16_t)(percentage * (float)MOUSE_XOVER_THROTTLE_MAX);
  }

  void tick(uint32_t time_ms) {
    (void) time_ms;
    static uint16_t out_count = 0;
    static uint8_t key_buf[6] = {0, 0, 0, 0, 0, 0};
    if (out_count++ < 16000 - throttle) {
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

  void process_mouse_report(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    static bool left_button_last_pressed = false;

    // if user presses right key, move to the next character
    if (report->buttons & 0b10) {
      skip_backspace = true;
    }

    if (state != idle) return;

    // if user is holding left button, let them "navigate"/edit
    if (report->buttons & 0b01) {
      bool consumed = true;
      if (report->wheel > 0) {
        current_key = HID_KEY_ARROW_LEFT;
      } else if (report->wheel < 0) {
        current_key = HID_KEY_ARROW_RIGHT;
      } else if (report->x > 5) {
        current_key = HID_KEY_SPACE;
      } else if (report->x < -5) {
        current_key = HID_KEY_BACKSPACE;
      } else if (report->y > 6) {
        current_key = HID_KEY_ENTER;
      } else {
        consumed = false;
      }

      if (consumed) {
        state = key_press;
        left_button_last_pressed = true;
      }
      return;
    }

    // if user was just navigating/editing, don't prepend a backspace before the
    // next character
    if (left_button_last_pressed) {
      skip_backspace = true;
      left_button_last_pressed = false;
    }

    if (report->wheel < 0 || report->y < -1) {
      current_key--;
      state = skip_backspace ? key_press : backspace_press;
    } else if (report->wheel > 0 || report->y > 1) {
      current_key++;
      state = skip_backspace ? key_press : backspace_press;
    }

    // this is stupid, just hardcode an array of valid values
    if (current_key < HID_KEY_A) {
      current_key = HID_KEY_SLASH;
    } else if (current_key == HID_KEY_0 + 1) {
      current_key = HID_KEY_MINUS;
    } else if (current_key == HID_KEY_MINUS - 1) {
      current_key = HID_KEY_0;
    } else if (current_key > HID_KEY_SLASH) {
      current_key = HID_KEY_A;
    }
  }
};
