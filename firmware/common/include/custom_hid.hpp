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

// IMPORTANT!!! should be identical to hid_mouse_report_t:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
// WE ARE DOING SOME DANGEROUS CASTING :)
typedef struct TU_ATTR_PACKED
{
  uint8_t buttons; /**< buttons mask for currently pressed buttons in the mouse. */
  int8_t  x;       /**< Current delta x movement of the mouse. */
  int8_t  y;       /**< Current delta y movement on the mouse. */
  int8_t  wheel;   /**< Current delta wheel movement on the mouse. */
  int8_t  pan;     // using AC Pan
} ha_mouse_report_t;

// IMPORTANT!!! should be identical to hid_keyboard_report_t:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
// WE ARE DOING SOME DANGEROUS CASTING :)

typedef struct TU_ATTR_PACKED
{
  uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
  uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
  uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} ha_keyboard_report_t;


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
  virtual void process_mouse_report(ha_mouse_report_t const *report,
                                    uint32_t time_ms) = 0;
  virtual ~IMouseFx() {}
};

class IKeyboardFx : public IFx {
 public:
  virtual void process_keyboard_report(ha_keyboard_report_t const *report,
                                       uint32_t time_ms) = 0;
  virtual ~IKeyboardFx() {}
};
#endif