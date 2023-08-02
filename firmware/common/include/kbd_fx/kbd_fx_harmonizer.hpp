#include "custom_hid.hpp"

#define PRESSED_KEYS_COUNT REPORT_KEYCODE_COUNT / 2

class KeyboardHarmonizer : public IKeyboardFx {
  uint8_t pressed_keys[PRESSED_KEYS_COUNT] = {0};
  uint8_t report_code_buffer[REPORT_KEYCODE_COUNT] = {0};
  uint8_t harmony_offset = 1;
  uint8_t harmonics = 0;
  uint8_t led_flash_count = 0;
  uint32_t last_flash_time = 0;
  float led_brightness = 0.3;

  int8_t index_of(uint8_t keycode, const uint8_t* array) {
    uint8_t n = sizeof(array);
    for (size_t i = 0; i < n; i++) {
      if (array[i] == keycode) {
        return i;
      }
    }
    return -1;
  }

  inline int8_t free_slot_index() { return index_of(0, pressed_keys); }

 public:
  KeyboardHarmonizer() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
    update_parameter(param_percentage);
    log_line("Keyboard harmonizer initialized");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    if (led_flash_count == 0 || time_ms - last_flash_time < 32) {
      return color_at_brightness(indicator_color, led_brightness);
    }
    last_flash_time = time_ms;
    led_flash_count--;
    if (led_flash_count & 1) {
      return color_at_brightness(indicator_color, 0.1f);
    } else {
      return color_at_brightness(indicator_color, 0.8f);
    }
  }

  void update_parameter(float percentage) {
    uint8_t raw_percentage = (uint8_t)(percentage * 98.0);
    harmony_offset = raw_percentage % 33;
    uint8_t last_harmonic_value = harmonics;
    harmonics = raw_percentage / 33;
    // if we cross into a new zone, defined by the number of harmonics, flash to
    // let user know
    if (last_harmonic_value != harmonics) {
      led_flash_count = (2 * harmonics) + 2;
      log_line("Keyboard harmonics: %u", harmonics);
    }
    led_brightness = (((float)harmony_offset / 33.0) * 0.8) + 0.05;
  }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_keyboard_report(ha_keyboard_report_t const* report,
                               uint32_t time_ms) {
    (void)time_ms;
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      uint8_t keycode = report->keycode[i];
      if (keycode == 0) break;
      int8_t free_slot = free_slot_index();
      if (free_slot < 0) break;
      int8_t pressed_key_index = index_of(keycode, pressed_keys);
      if (pressed_key_index < 0) {
        pressed_keys[free_slot] = keycode;
      }
    }

    for (size_t i = 0; i < PRESSED_KEYS_COUNT; i++) {
      int8_t currently_pressed_index =
          index_of(pressed_keys[i], report->keycode);
      if (currently_pressed_index == -1) {
        pressed_keys[i] = 0;
      }
    }

    size_t j = 0;
    for (size_t i = 0; i < PRESSED_KEYS_COUNT && j < REPORT_KEYCODE_COUNT;
         i++) {
      uint8_t pressed_key = pressed_keys[i];
      if (pressed_key > 0) {
        if (harmonics == 0) {
          // execute one key press
          report_code_buffer[j++] = pressed_key + harmony_offset;
          led_flash_count = 3;
        } else {
          // execute two keys
          led_flash_count = 5;
          report_code_buffer[j++] = pressed_key;
          report_code_buffer[j++] = pressed_key + harmony_offset;
          // execute three keys
          if (harmonics == 2) {
            report_code_buffer[j++] = pressed_key + (harmony_offset * 2);
            led_flash_count = 7;
          }
        }
      }
    }

    while (j < REPORT_KEYCODE_COUNT) {
      report_code_buffer[j] = 0;
      ++j;
    }
    send_keyboard_report(report->modifier, report->reserved,
                         report_code_buffer);
  }
};