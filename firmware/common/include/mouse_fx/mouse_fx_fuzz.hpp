#include <algorithm>

#include "custom_hid.hpp"
#include "hid_fx.hpp"

#define FILTER_BUF_SIZE 50

class MouseFuzz : public IMouseFx {
  using IMouseFx::IMouseFx;

 private:
  typedef struct sample {
    int8_t x;
    int8_t y;
  } sample_t;

  uint32_t last_mouse_report_time = 0;
  // hold last FILTER_BUF_SIZE x + y samples for simple low pass filter
  sample_t filter_buf[FILTER_BUF_SIZE];
  size_t filter_index = 0;
  // whether to add or remove "noise" (this is effectively [heh] two fx in one)
  bool add_noise;
  float noise_param;
  float filter_param;
  float last_noise_value;
  ha_mouse_report_t last_report;

  inline void add_filter_sample(int8_t x, int8_t y) {
    filter_buf[filter_index] = {x, y};
    if (++filter_index >= FILTER_BUF_SIZE) {
      filter_index = 0;
    }
  }

  inline sample_t get_filtered_samples() {
    size_t count = (size_t)(filter_param * (float)FILTER_BUF_SIZE);
    if (count < 1) count = 1;
    int16_t sum_x = 0, sum_y = 0;
    for (size_t i = filter_index + (FILTER_BUF_SIZE - count), j = 0; j < count;
         i++, j++) {
      sample_t s = filter_buf[i % FILTER_BUF_SIZE];
      sum_x += s.x;
      sum_y += s.y;
    }
    int8_t x_ave = sum_x / (int16_t)count;
    int8_t y_ave = sum_y / (int16_t)count;
    return {x_ave, y_ave};
  }

  inline float get_filtered_average() {
    size_t count = (size_t)(filter_param * (float)FILTER_BUF_SIZE);
    if (count < 1) count = 1;
    float sum_x = 0.0f, sum_y = 0.0f;
    for (size_t i = filter_index + (FILTER_BUF_SIZE - count), j = 0; j < count;
         i++, j++) {
      sample_t s = filter_buf[i % FILTER_BUF_SIZE];
      sum_x += s.x;
      sum_y += s.y;
    }
    float x_ave = sum_x / (float)count;
    float y_ave = sum_y / (float)count;
    return std::max(abs(x_ave), abs(y_ave)) / 127.0f;
  }

 public:

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    update_parameter(param_percentage);
    log_line("Mouse fuzz/filter initialized");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    static bool sent_flicker_last_tick = false;
    if (add_noise) {
      if (last_noise_value > 0.0f && !sent_flicker_last_tick) {
        float brightness = (last_noise_value * 0.3f) + 0.4f;
        sent_flicker_last_tick = true;
        last_noise_value = 0.0f;
        return color_at_brightness(indicator_color, brightness);
      } else {
        // low value is inversely related to noise_param to emphasize flickering
        // when noise_param is higher
        sent_flicker_last_tick = false;
        return color_at_brightness(indicator_color,
                                   ((1.0f - noise_param) / 3.5) + 0.05f);
      }
    } else {
      float brightness = (get_filtered_average() * 1.2f) + 0.05f;
      brightness = std::min(0.9f, brightness);
      return color_at_brightness(indicator_color, brightness);
    }
  }

  void update_parameter(float percentage) {
    add_noise = percentage > 0.5f;
    if (add_noise) {
      noise_param = (percentage - 0.5f) * 2;
    } else {
      filter_param = 1.0f - (percentage * 2.0f);
    }
  }

  void tick(uint32_t time_ms) {
    static uint32_t last_dummy_sample = 0;
    uint32_t delta_time = time_ms - last_mouse_report_time;
    if (!add_noise && delta_time > 25 && delta_time < 250) {
      if (time_ms - last_dummy_sample > 5) {
        last_report.x = 0;
        last_report.y = 0;
        process_with_filter(&last_report, time_ms);
        last_dummy_sample = time_ms;
      }
    }
  }

  void deinit() {}

  void process_with_noise(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    float adj_noise = noise_param / 3.0f;
    float x_noise = (float)(int8_t)get_random_byte() * adj_noise;
    float y_noise = (float)(int8_t)get_random_byte() * adj_noise;
    int8_t x = (int8_t)round(x_noise + (float)report->x);
    int8_t y = (int8_t)round(y_noise + (float)report->y);
    last_noise_value = std::min(abs(x_noise), abs(y_noise)) / 127.0f;
    hid_output->send_mouse_report(report->buttons, x, y, report->wheel, report->pan);
  }

  void process_with_filter(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    last_report = *report;
    add_filter_sample(report->x, report->y);
    sample_t filtered = get_filtered_samples();
    hid_output->send_mouse_report(report->buttons, filtered.x, filtered.y, report->wheel,
                      report->pan);
  }

  void process_mouse_report(ha_mouse_report_t const *report, uint32_t time_ms) {
    last_mouse_report_time = time_ms;
    if (add_noise) {
      process_with_noise(report, time_ms);
    } else {
      process_with_filter(report, time_ms);
    }
  }
};
