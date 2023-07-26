#include "custom_hid.hpp"

#define FILTER_BUF_SIZE 50

class MouseFuzz : public IMouseFx {
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

 public:
  MouseFuzz() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    (void)param_percentage;
    log_line("Mouse fuzz/filter initialized");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    // TODO
    (void)time_ms;
    return indicator_color;
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
    // TODO add filter samples 0,0 if time since last report > n
    if (time_ms - last_mouse_report_time > 700) {
      add_filter_sample(0, 0);
      last_mouse_report_time = time_ms;
    }
  }

  void deinit() {}

  void process_with_noise(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    float adj_noise = noise_param / 3.0f;
    int8_t x = (int8_t)round(((float)(int8_t)get_random_byte() * adj_noise) +
                             (float)report->x);
    int8_t y = (int8_t)round(((float)(int8_t)get_random_byte() * adj_noise) +
                             (float)report->y);
    send_mouse_report(report->buttons, x, y, report->wheel, report->pan);
  }

  void process_with_filter(ha_mouse_report_t const *report, uint32_t time_ms) {
    (void)time_ms;
    add_filter_sample(report->x, report->y);
    sample_t filtered = get_filtered_samples();
    send_mouse_report(report->buttons, filtered.x, filtered.y, report->wheel,
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
