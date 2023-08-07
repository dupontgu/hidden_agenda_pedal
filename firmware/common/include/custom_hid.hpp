#include <stdlib.h>

#include "util.h"

#ifndef CUSTOM_HID_H
#define CUSTOM_HID_H

// should absolutely always be 6, according to spec
#define REPORT_KEYCODE_COUNT 6

// Should match
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
#define HID_KEY_A 0x04
#define HID_KEY_0 0x27
#define HID_KEY_MINUS 0x2D
#define HID_KEY_SLASH 0x38
#define HID_KEY_BACKSPACE 0x2A
#define HID_KEY_ARROW_RIGHT 0x4F
#define HID_KEY_ARROW_LEFT 0x50
#define HID_KEY_ARROW_DOWN 0x51
#define HID_KEY_ARROW_UP 0x52
#define HID_KEY_SPACE 0x2C
#define HID_KEY_ENTER 0x28

// IMPORTANT!!! should be identical to hid_mouse_report_t:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
// WE ARE DOING SOME DANGEROUS CASTING :)
typedef struct TU_ATTR_PACKED {
  uint8_t
      buttons;  /**< buttons mask for currently pressed buttons in the mouse. */
  int8_t x;     /**< Current delta x movement of the mouse. */
  int8_t y;     /**< Current delta y movement on the mouse. */
  int8_t wheel; /**< Current delta wheel movement on the mouse. */
  int8_t pan;   // using AC Pan
} ha_mouse_report_t;

// IMPORTANT!!! should be identical to hid_keyboard_report_t:
// https://github.com/hathach/tinyusb/blob/master/src/class/hid/hid.h
// WE ARE DOING SOME DANGEROUS CASTING :)
typedef struct TU_ATTR_PACKED {
  uint8_t modifier;   /**< Keyboard modifier (KEYBOARD_MODIFIER_* masks). */
  uint8_t reserved;   /**< Reserved for OEM use, always set to 0. */
  uint8_t keycode[6]; /**< Key codes of the currently pressed keys. */
} ha_keyboard_report_t;

#endif