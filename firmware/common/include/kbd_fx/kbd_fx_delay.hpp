#include <algorithm>
#include <cmath>

#include "custom_hid.hpp"
#include "hid_fx.hpp"

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
  using IKeyboardFx::IKeyboardFx;
  ha_keyboard_report_t latest_report;
  delay_slot_t slots[DELAY_SLOT_COUNT];
  float cycle_progress = 0.0;
  int16_t max_delay_count = 1;
  uint32_t pixel_last_update = 0;
  uint16_t delay_ms = 500;
  uint8_t last_report_key_count = 0;
  uint8_t slot_index = 0;
  uint16_t remaining_repeats = 0;

  inline void resend_latest_report() {
    hid_output->send_keyboard_report(
        latest_report.modifier, latest_report.reserved, latest_report.keycode);
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

 public:
  void initialize(uint32_t time_ms, float param_percentage) {
    log_line("Keyboard delay initialized");
    update_parameter(param_percentage);
    pixel_last_update = time_ms;
    cycle_progress = 0.0;
    for (size_t i = 0; i < DELAY_SLOT_COUNT; i++) {
      slots[i].code = 0;
    }
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    float delta = (float)(time_ms - pixel_last_update) / (float)delay_ms;
    pixel_last_update = time_ms;
    cycle_progress += delta;
    // sawtooth loop forever
    if (cycle_progress > 1.0) {
      cycle_progress = 0.0;
    }

    // get dimmer as remaining repeats approaches 0
    float repeat_brightness = (float)remaining_repeats / (float)max_delay_count;
    // repeat_brightness < 0 means we're repeating infinitely
    if (repeat_brightness < 0.0 || repeat_brightness > 1.0f) {
      repeat_brightness = 1.0f;
    }
    float adj_brightness = (cycle_progress * repeat_brightness * 0.9f) + 0.1f;
    return color_at_brightness(indicator_color, adj_brightness);
  }

  void update_parameter(float percentage) {
    uint16_t int_p = std::lroundf(percentage * 99.0f);
    switch (int_p) {
      case 0 ... 24:
        delay_ms = 100;
        break;
      case 25 ... 49:
        delay_ms = 300;
        break;
      case 50 ... 74:
        delay_ms = 650;
        break;
      default:
        delay_ms = 1200;
        break;
    }
    int16_t last_delay_count = max_delay_count;
    max_delay_count = ((int_p % 25) / 2) + 1;
    if (max_delay_count > 11) {
      max_delay_count = -1;
    }
    if (last_delay_count != max_delay_count) {
      log_line("Keyboard delay repeats: %d", max_delay_count);
    }
  }

  void deinit() {
    latest_report.modifier = 0;
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      latest_report.keycode[i] = 0;
    }

    resend_latest_report();
  }

  void tick(uint32_t time_ms) {
    static uint32_t last_flush_ms = 0;
    if (time_ms - last_flush_ms >= FLUSH_THRESHOLD_MS) {
      release_all(true);
      last_flush_ms = time_ms;
    }

    // keep track of the key with the most remaining repeats left for animation
    int16_t local_remaining_max = 0;
    for (size_t i = 0; i < DELAY_SLOT_COUNT; i++) {
      if (slots[i].code > 0) {
        int16_t remaining = max_delay_count - slots[i].count;
        if (abs(remaining) > local_remaining_max) {
          local_remaining_max = remaining;
        }

        // this key is due for a repeat emission
        if (time_ms - slots[i].init_time >= delay_ms) {
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
          // key has repeated enough times, turn it off
          // max_delay_count < 0 means repeat forever, so don't reset
          if (slots[i].count >= max_delay_count && max_delay_count > 0) {
            slots[i].code = 0;
            slots[i].count = 0;
          }
        }
      }
    }
    remaining_repeats = local_remaining_max;
  }

  void process_keyboard_report(ha_keyboard_report_t const *report,
                               uint32_t time_ms) {
    release_all(false);
    latest_report.modifier = report->modifier;
    latest_report.reserved = report->reserved;
    last_report_key_count = 0;
    hid_output->send_keyboard_report(report->modifier, report->reserved,
                                     report->keycode);
    for (size_t i = 0; i < REPORT_KEYCODE_COUNT; i++) {
      uint8_t code = report->keycode[i];
      latest_report.keycode[i] = code;
      if (code > 0) {
        // restart animation if any key is pressed
        cycle_progress = 0.0;
        last_report_key_count = i + 1;
        add_keycode_to_next_slot(code, time_ms);
      }
    }
  }
};