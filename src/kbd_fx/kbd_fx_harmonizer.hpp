#include "custom_hid.hpp"
#include "tusb.h"

#define PRESSED_KEYS_COUNT REPORT_KEYCODE_COUNT / 2

class KeyboardHarmonizer : public IKeyboardFx {
  uint32_t indicator_color;
  uint8_t pressed_keys[PRESSED_KEYS_COUNT] = {0};
  uint8_t report_code_buffer[REPORT_KEYCODE_COUNT] = {0};
  uint8_t harmony_offset = 1;
  uint8_t harmonics = 0;

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
  KeyboardHarmonizer() { indicator_color = urgb_u32(30, 30, 80); }

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
    log_line("keyboard harmonizer init %u", 1);
    update_parameter(param_percentage);
  }

  uint32_t get_indicator_color() { return indicator_color; }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return indicator_color;
  }

  void update_parameter(float percentage) {
    uint8_t raw_percentage = (uint8_t)(percentage * 98.0);
    harmony_offset = raw_percentage % 33;
    harmonics = raw_percentage / 33;
    log_line("harmony %u %u", harmony_offset, harmonics);
  }

  void tick(uint32_t time_ms) { (void)time_ms; }

  void deinit() {}

  void process_keyboard_report(hid_keyboard_report_t const* report,
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
          report_code_buffer[j++] = pressed_key + harmony_offset;
        } else {
          report_code_buffer[j++] = pressed_key;
          report_code_buffer[j++] = pressed_key + harmony_offset;
          if (harmonics == 2) {
            report_code_buffer[j++] = pressed_key + (harmony_offset * 2);
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