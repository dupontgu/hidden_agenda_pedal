#include "custom_hid.hpp"

#define MOUSE_XOVER_THROTTLE_MAX 13000

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
    log_line("Mouse crossover initialized");
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
    (void)time_ms;
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
    static bool right_button_last_pressed = false;
    bool right_button_pressed = report->buttons & 0b10;

    // if user presses right key, move to the next character
    if (!right_button_pressed && right_button_last_pressed) {
      skip_backspace = true;
    }

    right_button_last_pressed = right_button_pressed;

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

    // dont move unless holding button
    if (!right_button_pressed && report->wheel == 0) return;

    uint8_t last_key = current_key;
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
    } else if (current_key > HID_KEY_0 && current_key < HID_KEY_MINUS) {
      current_key = last_key < current_key ? HID_KEY_MINUS : HID_KEY_0;
    } else if (current_key > HID_KEY_SLASH) {
      current_key = HID_KEY_A;
    }
  }
};
