#include "custom_hid.hpp"
#include "tusb.h"

// should absolutely always be 6, according to spec
#define REPORT_KEYCODE_COUNT 6
// this one we can change
#define DELAY_SLOT_COUNT 6

#define FLUSH_THRESHOLD_MS 20

typedef struct {
  uint32_t init_time;
  uint8_t code;
  uint8_t count;
  bool awaiting_release;
} delay_slot_t;

class KeyboardDelay : public IKeyboardFx {
  hid_keyboard_report_t latest_report;
  delay_slot_t slots[DELAY_SLOT_COUNT];
  float pixel_cycle_progress = 0.0;
  uint32_t pixel_last_update = 0;
  uint32_t last_flush_ms = 0;
  uint16_t delay_ms = 500;
  uint8_t last_report_key_count = 0;
  uint8_t slot_index = 0;
  uint8_t max_delay_count = 10;

  inline void resend_latest_report() {
    send_keyboard_report(latest_report.modifier, latest_report.reserved,
                         latest_report.keycode);
  }

  void release_all(bool allow_blank_report) {
    bool fire_blank_report = false;
    for (size_t i = 0; i < DELAY_SLOT_COUNT; i++) {
      if (slots[i].awaiting_release) {
        fire_blank_report = true;
      }
      slots[i].awaiting_release = false;
    }
    if (allow_blank_report && fire_blank_report) {
      resend_latest_report();
    }
  }

  inline void add_keycode_at(uint8_t index, uint8_t keycode, uint32_t time_ms) {
    slots[index].code = keycode;
    slots[index].init_time = time_ms;
    slots[index].count = 0;
  }

  void add_keycode_to_next_slot(uint8_t keycode, uint32_t time_ms) {
    add_keycode_at(slot_index, keycode, time_ms);
    if (++slot_index == DELAY_SLOT_COUNT) {
      slot_index = 0;
    }
  }

  inline void update_from_param(float param_percentage) {
    // TODO too fast still
    delay_ms = (param_percentage * 1500.0) + 150.0;
    max_delay_count = (int)(param_percentage * 12.0) + 1;
  }

 public:
  KeyboardDelay() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    update_from_param(param_percentage);
    pixel_last_update = time_ms;
    pixel_cycle_progress = 0.0;
    for (size_t i = 0; i < DELAY_SLOT_COUNT; i++) {
      slots[i].code = 0;
    }
  }

  uint32_t get_indicator_color() { return urgb_u32(0, 50, 50); }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    float delta = (float)(time_ms - pixel_last_update) / (float)delay_ms;
    pixel_last_update = time_ms;
    pixel_cycle_progress += delta;
    if (pixel_cycle_progress > 1.0) {
      pixel_cycle_progress = 0.0;
    }
    float adj_brightness = pixel_cycle_progress * pixel_cycle_progress * 0.5;
    return color_at_brightness(get_indicator_color(), adj_brightness);
  }

  void update_parameter(float percentage) { update_from_param(percentage); }

  void deinit() {
    latest_report.modifier = 0;
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      latest_report.keycode[i] = 0;
    }

    resend_latest_report();
  }

  void tick(uint32_t time_ms) {
    if (time_ms - last_flush_ms >= FLUSH_THRESHOLD_MS) {
      release_all(true);
      last_flush_ms = time_ms;
    }
    for (size_t i = 0; i < DELAY_SLOT_COUNT; i++) {
      if (slots[i].code > 0 && time_ms - slots[i].init_time >= delay_ms) {
        log_line("fire delay: %u", slots[i].code);
        last_flush_ms = time_ms;
        slots[i].count = slots[i].count + 1;
        slots[i].awaiting_release = true;
        slots[i].init_time = time_ms;
        uint8_t index = last_report_key_count == REPORT_KEYCODE_COUNT
                            ? REPORT_KEYCODE_COUNT - 1
                            : last_report_key_count;
        latest_report.keycode[index] = slots[i].code;
        resend_latest_report();
        latest_report.keycode[index] = 0;
        if (slots[i].count >= max_delay_count) {
          slots[i].code = 0;
          slots[i].count = 0;
        }
      }
    }
  }

  void process_keyboard_report(hid_keyboard_report_t const *report,
                               uint32_t time_ms) {
    release_all(false);
    latest_report.modifier = report->modifier;
    latest_report.reserved = report->reserved;
    last_report_key_count = 0;
    send_keyboard_report(report->modifier, report->reserved, report->keycode);
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      uint8_t code = report->keycode[i];
      latest_report.keycode[i] = code;
      if (code > 0) {
        last_report_key_count = i + 1;
        add_keycode_to_next_slot(code, time_ms);
      }
    }
  }
};