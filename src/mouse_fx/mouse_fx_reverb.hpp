#include "custom_hid.hpp"
#include "tusb.h"

#define REVERB_BUF_SIZE 18
#define REVERB_DEBOUNCE 10
#define MIN_VELOCITY_SCALAR 0.92
#define MAX_VELOCITY_SCALAR 0.998

class MouseReverb : public IMouseFx {
 private:
  struct vec2 {
    uint8_t x;
    uint8_t y;
  } vec2_t;
  float velocity_scalar = MIN_VELOCITY_SCALAR;
  float current_velocity = 0.0;
  uint32_t last_sample_time_ms = 0;
  uint32_t last_reverb_time_ms = 0;
  uint8_t last_buttons = 0;
  int8_t x_buf[REVERB_BUF_SIZE] = {0};
  int8_t y_buf[REVERB_BUF_SIZE] = {0};
  size_t buf_index = 0;

  void add_sample(int8_t x, int8_t y) {
    if (++buf_index == REVERB_BUF_SIZE) {
      buf_index = 0;
    }
    x_buf[buf_index] = x;
    y_buf[buf_index] = y;
  }

  inline float buf_average(int8_t *buf) {
    float sum = 0.0;
    // start from earliest sample
    size_t start = buf_index + 1;
    // skip last couple
    size_t end = start + REVERB_BUF_SIZE - 2;
    float total_samples = 0;
    for (size_t i = start; i < end; i++) {
      int count = (i - start) + 1;
      sum += buf[i % REVERB_BUF_SIZE] * count;
      total_samples += count;
    }
    return sum / total_samples;
  }

 public:
  MouseReverb() {}

  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    update_parameter(param_percentage);
    log_line("m vrb init");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    float brightness = (current_velocity * 0.7f) + 0.1f;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) { 
    velocity_scalar = MIN_VELOCITY_SCALAR + ((1.0 - MIN_VELOCITY_SCALAR) * percentage);
    if (velocity_scalar > MAX_VELOCITY_SCALAR) {
      velocity_scalar = MAX_VELOCITY_SCALAR;
    }
  }

  void tick(uint32_t time_ms) {
    if (current_velocity <= 0.02) {
      return;
    } else if (time_ms - last_sample_time_ms < REVERB_DEBOUNCE) {
      return;
    } else if (time_ms - last_reverb_time_ms < REVERB_DEBOUNCE) {
      return;
    } 
    last_reverb_time_ms = time_ms;
    current_velocity = current_velocity * velocity_scalar;
    int8_t x = (int8_t)(buf_average(x_buf) * current_velocity);
    int8_t y = (int8_t)(buf_average(y_buf) * current_velocity);
    
    // no reason to keep going, clear the buffer
    if (x == 0 && y == 0) {
      for (size_t i = 0; i < REVERB_BUF_SIZE; i++){
        add_sample(0, 0);
      }
    } else {
      send_mouse_report(last_buttons, x, y, 0, 0);
    }
  }

  void deinit() {}

  void process_mouse_report(hid_mouse_report_t const *report,
                            uint32_t time_ms) {
    last_sample_time_ms = time_ms;
    current_velocity = 1.0;
    last_buttons = report->buttons;
    add_sample(report->x, report->y);
    log_line("m samp %d, %d", report->x, report->y);
    send_mouse_report(report->buttons, report->x, report->y, report->wheel, 0);
  }
};
