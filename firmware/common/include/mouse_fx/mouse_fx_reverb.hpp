#include "custom_hid.hpp"
#include "hid_fx.hpp"

#define REVERB_BUF_SIZE 8
#define REVERB_DEBOUNCE 12
#define MIN_VELOCITY_SCALAR 0.86
#define MAX_VELOCITY_SCALAR 0.994

class MouseReverb : public IMouseFx {
  using IMouseFx::IMouseFx;

 private:
  struct vec2 {
    uint8_t x;
    uint8_t y;
  } vec2_t;
  float velocity_scalar = MIN_VELOCITY_SCALAR;
  float current_velocity = 0.0;
  uint32_t last_sample_time_ms = 0;
  uint8_t last_buttons = 0;
  int8_t x_buf[REVERB_BUF_SIZE] = {0};
  int8_t y_buf[REVERB_BUF_SIZE] = {0};
  size_t buf_index = 0;

  inline void add_sample(int8_t x, int8_t y) {
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
    size_t end = start + REVERB_BUF_SIZE - 1;
    float total_samples = 0;
    for (size_t i = start; i < end; i++) {
      int count = (i - start) + 1;
      sum += buf[i % REVERB_BUF_SIZE] * count;
      total_samples += count;
    }
    return sum / total_samples;
  }

 public:
  void initialize(uint32_t time_ms, float param_percentage) {
    (void)time_ms;
    update_parameter(param_percentage);
    log_line("Mouse reverb initialized");
  }

  uint32_t get_current_pixel_value(uint32_t time_ms) {
    (void)time_ms;
    float brightness = (current_velocity * 0.7f) + 0.1f;
    return color_at_brightness(indicator_color, brightness);
  }

  void update_parameter(float percentage) {
    velocity_scalar =
        MIN_VELOCITY_SCALAR + ((1.0 - MIN_VELOCITY_SCALAR) * percentage);
    if (velocity_scalar > MAX_VELOCITY_SCALAR) {
      velocity_scalar = MAX_VELOCITY_SCALAR;
    }
  }

  void tick(uint32_t time_ms) {
    static uint32_t last_reverb_time_ms = 0;
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

    if (x != 0 || y != 0) {
      hid_output->send_mouse_report(last_buttons, x, y, 0, 0);
    } else if (current_velocity < 0.1) {
      for (size_t i = 0; i < REVERB_BUF_SIZE; i++) {
        add_sample(0, 0);
      }
    }
  }

  void deinit() {}

  void process_mouse_report(ha_mouse_report_t const *report, uint32_t time_ms) {
    static int8_t pending_x = 0;
    static int8_t pending_y = 0;
    current_velocity = 1.0;
    last_buttons = report->buttons;
    pending_x += report->x;
    pending_y += report->y;
    if (time_ms - last_sample_time_ms > REVERB_DEBOUNCE) {
      add_sample(pending_x, pending_y);
      pending_x = 0;
      pending_y = 0;
      last_sample_time_ms = time_ms;
    }
    hid_output->send_mouse_report(report->buttons, report->x, report->y,
                                  report->wheel, 0);
  }
};
