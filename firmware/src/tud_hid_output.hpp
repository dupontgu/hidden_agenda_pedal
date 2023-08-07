#include "hid_fx.hpp"
#include "tusb.h"
#include "usb_descriptors.h"

#ifndef HA_TUD_HID_OUTPUT
#define HA_TUD_HID_OUTPUT

class TinyHIDOutput : public IHIDOutput {
 public:
  void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                         int8_t pan) {
    // log_line("m report %d %d %d", x, y, buttons);
    tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, pan);
  }
  void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                            const uint8_t keycode[6]) {
    (void)reserved;
    // log_line("k report %u %u %u %u %u %u", keycode[0], keycode[1],
    // keycode[2],
    //          keycode[3], keycode[4], keycode[5]);
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, (uint8_t*)keycode);
  }
};

#endif