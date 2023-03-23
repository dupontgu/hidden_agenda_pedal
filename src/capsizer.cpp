/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp/board.h"
#include "hardware/adc.h"
#include "hardware/structs/rosc.h"
#include "kbd_fx/kbd_fx_delay.hpp"
#include "kbd_fx/kbd_fx_passthrough.hpp"
#include "kbd_fx/kbd_fx_tremolo.hpp"
#include "mouse_fx/mouse_fx_fuzz.hpp"
#include "mouse_fx/mouse_fx_passthrough.hpp"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "ws2812.pio.h"

#define GENERIC_DESKTOP_USAGE_PAGE 0x01
#define USAGE_MOUSE 0x02
#define USAGE_KEYBOARD 0x06

#define RANDOM_BIT (rosc_hw->randombit ? 1 : 0)
#define MS_SINCE_BOOT to_ms_since_boot(get_absolute_time())
// TODO update this
#define SOFT_BOOT_BTN_GPIO 0
// TODO update this
#define PIX_DATA_GPIO 29
#define FOOT_SW_GPIO 28
#define TOGGLE_1_GPIO 6
#define TOGGLE_2_GPIO 7
#define KNOB_ADC_GPIO 26

#define PIX_PIO pio0
#define PIX_PIO_SM 2

#define MAX_FX 4
#define MAX_REPORT 4

#define SW_MODE_SET 0
#define SW_MODE_MOM 1
#define SW_MODE_LATCH 2

#define ADC_DEAD_ZONE 0.015f

static struct {
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

MouseFuzz mouse_fuzz;
MousePassthrough mouse_passthrough(0x002F2F2F);
KeyboardTremolo keyboard_tremolo;
KeyboardPassthrough keyboard_passthrough;
KeyboardDelay keyboard_delay;
// LAST FX (at index MAX_FX) should always be passthrough! This is what gets run
// when pedal is "off"
static IMouseFx* mouse_fx[] = {&mouse_passthrough, &mouse_passthrough,
                               &mouse_passthrough, &mouse_passthrough,
                               &mouse_passthrough};
static IKeyboardFx* keyboard_fx[] = {
    &keyboard_delay, &keyboard_tremolo, &keyboard_passthrough,
    &keyboard_passthrough, &keyboard_passthrough};
static uint8_t active_device_type = HID_ITF_PROTOCOL_KEYBOARD;
static bool fx_enabled = true;
static uint8_t active_fx_slot = 0;
static uint8_t active_sw_mode = SW_MODE_SET;
static bool previous_foot_sw_value = 0;
static bool use_increased_dead_zone = 0;
static uint32_t frame_of_last_pix_update = 0;
static uint32_t frame_of_last_io_update = 0;
static float previous_adc_reading = -1.0f;

void log_line(const char* format, ...) {
  char buffer[256];
  va_list args;
  va_start(args, format);
  int count = vsnprintf(buffer, 253, format, args);
  buffer[count] = '\r';
  buffer[count + 1] = '\n';

  tud_cdc_write(buffer, (uint32_t)count + 2);
  tud_cdc_write_flush();

  va_end(args);
}

uint8_t get_random_byte() {
  uint8_t x = 0;
  for (size_t i = 0; i < 8; i++) {
    x |= RANDOM_BIT << i;
  }

  return x;
}

void on_fx_param_tweaked(float percentage) {
  mouse_fx[active_fx_slot]->update_parameter(percentage);
  keyboard_fx[active_fx_slot]->update_parameter(percentage);
}

inline float read_pot() {
  // expand range so we'll def get 0 and 1 on the ends
  float adc = ((((float)adc_read() / 4096.0f) * 1.03f) - 0.015);
  // clamp to range 0..1
  adc = adc < 0.0f ? 0.0f : (adc > 1.0f ? 1.0f : adc);
  return adc;
}

void update_from_pot() {
  float adc = read_pot();
  float delta = previous_adc_reading - adc;
  float dead_zone = use_increased_dead_zone ? ADC_DEAD_ZONE * 4 : ADC_DEAD_ZONE;
  // is there a less terrible way to write this? perhaps.
  if (!(adc == 1.0f && previous_adc_reading != 1.0f) &&
      !(adc == 0.0f && previous_adc_reading != 0.0f) && delta < dead_zone &&
      delta > -dead_zone) {
    return;
  }
  use_increased_dead_zone = 0;
  previous_adc_reading = adc;
  if (active_sw_mode != SW_MODE_SET) {
    on_fx_param_tweaked(adc);
  } else {
    uint8_t slot = adc * MAX_FX;
    if (slot == MAX_FX) slot--;
    if (slot != active_fx_slot) {
      log_line("fx slot: %u", slot);
      uint32_t time_ms = MS_SINCE_BOOT;
      mouse_fx[active_fx_slot]->deinit();
      mouse_fx[slot]->initialize(time_ms, adc);
      keyboard_fx[active_fx_slot]->deinit();
      keyboard_fx[slot]->initialize(time_ms, adc);
    }
    active_fx_slot = slot;
  }
}

void read_foot_switch() {
  bool current_val = gpio_get(FOOT_SW_GPIO);
  if (current_val == previous_foot_sw_value) {
    return;
  }
  log_line("foot switch: %u", (uint8_t)current_val);
  previous_foot_sw_value = current_val;
  if (active_sw_mode == SW_MODE_LATCH && current_val) {
    fx_enabled = !fx_enabled;
  } else if (active_sw_mode == SW_MODE_MOM) {
    fx_enabled = current_val;
  }
}

void read_toggle_switch() {
  uint8_t current_val = (gpio_get(TOGGLE_1_GPIO) ? 0 : 0b01) |
                        (gpio_get(TOGGLE_2_GPIO) ? 0 : 0b10);
  if (current_val == active_sw_mode) {
    return;
  }
  active_sw_mode = current_val;
  log_line("toggle switch: %u", current_val);
  // if we're switching modes, disable the fx and make the knob less sensitive
  fx_enabled = false;
  use_increased_dead_zone = 1;
}

void reboot_to_uf2(uint gpio, uint32_t events) {
  (void)gpio;
  (void)events;
  reset_usb_boot(0, 0);
}

void init_soft_boot() {
  gpio_init(SOFT_BOOT_BTN_GPIO);
  gpio_set_dir(SOFT_BOOT_BTN_GPIO, GPIO_IN);
  gpio_pull_up(SOFT_BOOT_BTN_GPIO);
  gpio_set_irq_enabled_with_callback(SOFT_BOOT_BTN_GPIO, GPIO_IRQ_EDGE_FALL,
                                     true, &reboot_to_uf2);
}

void init_pix() {
  uint offset = pio_add_program(PIX_PIO, &ws2812_program);

  ws2812_program_init(PIX_PIO, PIX_PIO_SM, offset, PIX_DATA_GPIO, 800000,
                      false);
}

void init_io() {
  gpio_init(TOGGLE_1_GPIO);
  gpio_set_dir(TOGGLE_1_GPIO, GPIO_IN);
  gpio_pull_up(TOGGLE_1_GPIO);

  gpio_init(TOGGLE_2_GPIO);
  gpio_set_dir(TOGGLE_2_GPIO, GPIO_IN);
  gpio_pull_up(TOGGLE_2_GPIO);

  gpio_init(FOOT_SW_GPIO);
  gpio_set_dir(FOOT_SW_GPIO, GPIO_IN);
  gpio_pull_up(FOOT_SW_GPIO);

  adc_init();
  adc_gpio_init(KNOB_ADC_GPIO);
  adc_select_input(0);
}

void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                       int8_t pan) {
  tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, pan);
}

void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                          const uint8_t keycode[6]) {
  log_line("latest report %u %u %u %u %u %u", keycode[0], keycode[1],
           keycode[2], keycode[3], keycode[4], keycode[5]);
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, (uint8_t*)keycode);
}

static inline void set_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(PIX_PIO, PIX_PIO_SM, pixel_grb << 8u);
}

void led_task(uint32_t time_ms) {
  uint32_t frame = time_ms / 30;
  if (frame == frame_of_last_pix_update) {
    return;
  }
  frame_of_last_pix_update = frame;
  uint32_t color = 0;
  if (active_sw_mode == SW_MODE_SET) {
    if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD || true) {  // TODO
      color = keyboard_fx[active_fx_slot]->get_indicator_color();
    } else {
      color = mouse_fx[active_fx_slot]->get_indicator_color();
    }
  } else if (fx_enabled) {
    if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD || true) {
      color = keyboard_fx[active_fx_slot]->get_current_pixel_value(time_ms);
    } else {
      color = mouse_fx[active_fx_slot]->get_current_pixel_value(time_ms);
    }
  }
  set_pixel(color);
}

void io_task(uint32_t time_ms) {
  uint32_t frame = time_ms / 20;
  if (frame == frame_of_last_io_update) {
    return;
  }
  frame_of_last_io_update = frame;
  read_toggle_switch();
  read_foot_switch();
  update_from_pot();
}

void fx_task(uint32_t time_ms) {
  if (fx_enabled) {
    mouse_fx[active_fx_slot]->tick(time_ms);
    keyboard_fx[active_fx_slot]->tick(time_ms);
  }
}

void core1_main() {
  sleep_ms(10);

  // Use tuh_configure() to pass pio configuration to the host stack
  // Note: tuh_configure() must be called before
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);

  // To run USB SOF interrupt in core1, init host stack for pio_usb (roothub
  // port1) on core1
  tuh_init(1);

  while (true) {
    tuh_task();  // tinyusb host task
  }
}

/*------------- MAIN -------------*/
int main(void) {
  // board_init();

  // init device stack on configured roothub port
  set_sys_clock_khz(120000, true);

  multicore_reset_core1();
  // all USB task run in core1
  multicore_launch_core1(core1_main);

  tud_init(BOARD_TUD_RHPORT);
  init_soft_boot();
  init_pix();
  init_io();

  float param_value = read_pot();
  uint32_t now = MS_SINCE_BOOT;
  mouse_fx[active_fx_slot]->initialize(now, param_value);
  keyboard_fx[active_fx_slot]->initialize(now, param_value);

  while (1) {
    uint32_t time_ms = MS_SINCE_BOOT;
    led_task(time_ms);
    io_task(time_ms);
    fx_task(time_ms);
    tud_task();  // tinyusb device task
  }

  return 0;
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void) {}

// Invoked when device is unmounted
void tud_umount_cb(void) {}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en) { (void)remote_wakeup_en; }

// Invoked when usb bus is resumed
void tud_resume_cb(void) {}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report,
                                uint16_t len) {
  (void)report;
  (void)instance;
  (void)len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t* buffer,
                               uint16_t reqlen) {
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const* buffer,
                           uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)bufsize;
}

//--------------------------------------------------------------------+
// Host HID
//--------------------------------------------------------------------+

// Invoked when device with hid interface is mounted
// Report descriptor is also available for use.
// tuh_hid_parse_report_descriptor() can be used to parse common/simple enough
// descriptor. Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE,
// it will be skipped therefore report_desc = NULL, desc_len = 0
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = {"None", "Keyboard", "Mouse"};
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  hid_info[instance].report_count = tuh_hid_parse_report_descriptor(
      hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
  log_line("HID has %u reports", hid_info[instance].report_count);
  for (size_t i = 0; i < hid_info[instance].report_count; i++) {
    tuh_hid_report_info_t r = hid_info[instance].report_info[i];
    log_line("report id: %u, report usage: %u", r.report_id, r.usage);
  }

  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  log_line("[%04x:%04x][%u] HID Interface%u, Protocol = %s", vid, pid, dev_addr,
           instance, protocol_str[itf_protocol]);

  // Receive report from boot keyboard & mouse only
  // tuh_hid_report_received_cb() will be invoked when report is available
  if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD ||
      itf_protocol == HID_ITF_PROTOCOL_MOUSE) {
    active_device_type = itf_protocol;
    if (!tuh_hid_receive_report(dev_addr, instance)) {
      log_line("Error: cannot request report");
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
  log_line("[%u] HID Interface%u is unmounted", dev_addr, instance);
}

static inline bool find_key_in_report(hid_keyboard_report_t const* report,
                                      uint8_t keycode) {
  for (uint8_t i = 0; i < 6; i++) {
    if (report->keycode[i] == keycode) return true;
  }

  return false;
}

static void process_kbd_report(uint8_t dev_addr,
                               hid_keyboard_report_t const* report,
                               uint32_t time_ms) {
  (void)dev_addr;
  uint8_t slot = fx_enabled ? active_fx_slot : MAX_FX;
  keyboard_fx[slot]->process_keyboard_report(report, time_ms);
}

// send mouse report to usb device CDC
static void process_mouse_report(uint8_t dev_addr,
                                 hid_mouse_report_t const* report,
                                 uint32_t time_ms) {
  (void)dev_addr;
  uint8_t slot = fx_enabled ? active_fx_slot : MAX_FX;
  mouse_fx[slot]->process_mouse_report(report, time_ms);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
  uint8_t itf_protocol = HID_ITF_PROTOCOL_NONE;
  uint8_t const* report_offset = report;

  if (hid_info[instance].report_count > 1) {
    // we know this is a composite report, so extract the id and start the
    // reading the data one byte over
    report_offset = report + 1;
    uint8_t id = report[0];

    // find the tuh_hid_report_info_t that matches the id of the report we just
    // received
    for (size_t i = 0; i < hid_info[instance].report_count; i++) {
      tuh_hid_report_info_t info = hid_info[instance].report_info[i];
      if (info.report_id == id) {
        if (info.usage_page == GENERIC_DESKTOP_USAGE_PAGE) {
          if (info.usage == USAGE_MOUSE) {
            itf_protocol = HID_ITF_PROTOCOL_MOUSE;
          } else if (info.usage == USAGE_KEYBOARD) {
            itf_protocol = HID_ITF_PROTOCOL_KEYBOARD;
          }
        }
        // TODO handle other usage pages, Consume Control,
        break;
      }
    }
  } else {
    itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  }

  uint32_t time_ms = MS_SINCE_BOOT;
  switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report(dev_addr, (hid_keyboard_report_t const*)report_offset,
                         time_ms);
      break;

    case HID_ITF_PROTOCOL_MOUSE:
      process_mouse_report(dev_addr, (hid_mouse_report_t const*)report_offset,
                           time_ms);
      break;

    default:
      break;
  }

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    log_line("Error: cannot request report");
  }
}
