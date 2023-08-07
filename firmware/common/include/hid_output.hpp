#include <stdint.h>

#ifndef COMMON_HID_OUTPUT
#define COMMON_HID_OUTPUT
class IHIDOutput {
 public:
  virtual void send_mouse_report(uint8_t buttons, int8_t x, int8_t y,
                                 int8_t wheel, int8_t pan) = 0;
  virtual void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                                    const uint8_t keycode[6]) = 0;
  virtual ~IHIDOutput() = default;
};
#endif