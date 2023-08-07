#include <algorithm>

#include "custom_hid.hpp"
#include "hid_fx.hpp"

// random velocities to send the cursor around the screen when keys are pressed
static const int8_t skate_values[] = {-10, 12,  -18, 15,  -29, 35, -40,
                                      66,  -74, 80,  -90, 40,  -50};

class KeyboardXOver : public IKeyboardFx {
  using IKeyboardFx::IKeyboardFx;
  bool mouse_override = false;
  ha_mouse_report_t mouse_report;
  ha_mouse_report_t last_report;
  float acceleration = 1.0;

 public:
  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    update_parameter(param_percentage);
    log_line("Keyboard crossover initialized");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    float brightness = 0.2f;
    if (mouse_override || mouse_report.buttons) {
      brightness = 0.9f;
    } else {
      float d = std::max(abs(mouse_report.x), abs(mouse_report.y));
      // ensure that divisor is > skate_values.max { abs(it) }
      brightness = std::max(0.1f, d / 95.0f);
    }
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) { acceleration = percentage; }

  void tick(uint32_t time_ms) {
    static uint32_t last_time = 0;
    if (time_ms - last_time < 24) return;
    last_time = time_ms;
    if (mouse_override || mouse_report.x != last_report.x ||
        mouse_report.y != last_report.y ||
        mouse_report.buttons != last_report.buttons) {
      hid_output->send_mouse_report(mouse_report.buttons, mouse_report.x,
                                    mouse_report.y, mouse_report.wheel,
                                    mouse_report.pan);
      last_report = mouse_report;
    }

    if (!mouse_override) {
      // "gravity" - always be slowing down the cursor
      if (mouse_report.y != 0) mouse_report.y *= 0.9;
      if (mouse_report.x != 0) mouse_report.x *= 0.9;
    }
  }

  void deinit() {}

  void process_keyboard_report(ha_keyboard_report_t const *report,
                               uint32_t time_ms) {
    (void)time_ms;
    static int8_t last_x = 0;
    static int8_t last_y = 0;
    static bool sent_modifier_keys;
    int8_t x = 0, y = 0;
    uint8_t buttons = 0;
    uint8_t override_buttons_count = REPORT_KEYCODE_COUNT;
    // speed for when we're holding arrow keys
    int8_t override_velocity = (int8_t)(25.0 * acceleration) + 1;
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      uint8_t key = report->keycode[i];
      if (key == HID_KEY_ARROW_DOWN) {
        y += override_velocity;
      } else if (key == HID_KEY_ARROW_UP) {
        y -= override_velocity;
      } else if (key == HID_KEY_ARROW_LEFT) {
        x -= override_velocity;
      } else if (key == HID_KEY_ARROW_RIGHT) {
        x += override_velocity;
      } else if (key == HID_KEY_ENTER) {
        buttons = 1;
      } else {
        override_buttons_count--;
        if (key > 0) {
          // pseudo random x and y vals, scaled by acceleration
          x = (int8_t)((float)skate_values[key % 10] * acceleration) + 1;
          y = (int8_t)((float)skate_values[key % 13] * acceleration) + 1;
        }
      }
    }

    if ((x != 0 && x != last_x) || (y != 0 && y != last_y) ||
        buttons != mouse_report.buttons) {
      mouse_report.x = x;
      mouse_report.y = y;
      mouse_report.buttons = buttons;
    }

    last_x = x;
    last_y = y;

    // if we're holding an arrow key, we want to keep sending the same message
    // as long as its held
    mouse_override = override_buttons_count > 0;

    if (report->modifier || sent_modifier_keys) {
      const uint8_t dummy_keys[6] = {0, 0, 0, 0, 0, 0};
      hid_output->send_keyboard_report(report->modifier, report->reserved,
                                       dummy_keys);
      sent_modifier_keys = !sent_modifier_keys;
    }
  }
};