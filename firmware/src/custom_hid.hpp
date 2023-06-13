#include <stdlib.h>
#include "util.h"

// should absolutely always be 6, according to spec
#define REPORT_KEYCODE_COUNT 6

void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                       int8_t pan);
void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                          const uint8_t keycode[6]);

#ifndef CUSTOM_HID_H
#define CUSTOM_HID_H
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(g) << 8) | ((uint32_t)(r) << 16) | (uint32_t)(b);
}

static inline uint32_t color_at_brightness(uint32_t color, float brightness) {
  uint8_t r = (uint8_t)((float)(uint8_t)(color >> 16) * brightness);
  uint8_t g = (uint8_t)((float)(uint8_t)((color & 0xFF00) >> 8) * brightness);
  uint8_t b = (uint8_t)((float)(uint8_t)((color & 0x00FF)) * brightness);
  return urgb_u32(r, g, b);
}

class IFx {
 public:
  virtual void initialize(uint32_t time_ms, float param_percentage);
  virtual void deinit();
  virtual uint32_t get_current_pixel_value(uint32_t time_ms);
  virtual void update_parameter(float percentage);
  virtual void tick(uint32_t time_ms);

  uint32_t get_indicator_color() { return indicator_color; }

  void set_indicator_color(uint32_t c) { indicator_color = c; }

 protected:
  uint32_t indicator_color = 0xFF666666;
};

class IMouseFx : public IFx {
 public:
  virtual void process_mouse_report(hid_mouse_report_t const *report,
                                    uint32_t time_ms) = 0;
  virtual ~IMouseFx() {}
};

class IKeyboardFx : public IFx {
 public:
  virtual void process_keyboard_report(hid_keyboard_report_t const *report,
                                       uint32_t time_ms) = 0;
  virtual ~IKeyboardFx() {}
};
#endif