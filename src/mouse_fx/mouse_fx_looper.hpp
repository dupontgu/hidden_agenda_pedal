#include "custom_hid.hpp"
#include "tusb.h"

#define MOUSE_LOOP_BUFFER_SIZE 4096
#define MOUSE_LOOP_MAX_SPEED 2.5

class MouseLooper : public IMouseFx {
 private:
  typedef struct sample {
    uint32_t time_ms_offset;
    int8_t x;
    int8_t y;
  } sample_t;
  sample_t buffer[MOUSE_LOOP_BUFFER_SIZE];
  size_t loop_len;
  size_t buf_index;
  uint32_t record_start_time_ms;
  uint32_t loop_playback_start_time_ms;
  uint8_t latest_buttons_minus_right;
  float direction;
  float brightness;

 public:
  MouseLooper() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    loop_playback_start_time_ms = 0;
    update_parameter(param_percentage);
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) {
    brightness = percentage;
    direction = (percentage * 2.0f) - 1.0f;
  }

  void tick(uint32_t time_ms) {
    if (record_start_time_ms > 0 || loop_playback_start_time_ms == 0) return;

    sample_t next_sample = buffer[buf_index];
    uint32_t time_delta =
        (uint32_t)((float)(time_ms - loop_playback_start_time_ms) *
                   abs(direction * MOUSE_LOOP_MAX_SPEED));
    time_delta = time_delta < 2 ? 2 : time_delta;
    if (time_delta >= next_sample.time_ms_offset) {
      int8_t x = direction < 0.0f ? -next_sample.x : next_sample.x;
      int8_t y = direction < 0.0f ? -next_sample.y : next_sample.y;
      send_mouse_report(latest_buttons_minus_right, x, y, 0, 0);
      if (++buf_index >= loop_len) {
        loop_playback_start_time_ms = time_ms;
        buf_index = 0;
      }
    }
  }

  void deinit() {}

  void process_mouse_report(hid_mouse_report_t const *report,
                            uint32_t time_ms) {
    bool recording_btn_held = (report->buttons & 0b10) > 0;
    latest_buttons_minus_right = report->buttons & 0b11111101;
    if (record_start_time_ms == 0 && recording_btn_held) {
      record_start_time_ms = time_ms;
      buf_index = 0;
      loop_len = 0;
      log_line("recording started");
    } else if (record_start_time_ms > 0 && !recording_btn_held) {
      record_start_time_ms = 0;
      loop_playback_start_time_ms = time_ms;
      buf_index = 0;
      // after recording finishes, always start at 1X playback until user
      // touches knob
      direction = 1.0f / MOUSE_LOOP_MAX_SPEED;
      log_line("recording finished: %d", loop_len);
    }

    if (record_start_time_ms > 0) {
      uint32_t offset_time = time_ms - record_start_time_ms;
      buffer[buf_index++] = {offset_time, report->x, report->y};
      if (buf_index >= MOUSE_LOOP_BUFFER_SIZE) {
        buf_index = 0;
      }
      if (loop_len < MOUSE_LOOP_BUFFER_SIZE) loop_len++;
      send_mouse_report(latest_buttons_minus_right, report->x, report->y,
                        report->wheel, 0);
    }
  }
};
