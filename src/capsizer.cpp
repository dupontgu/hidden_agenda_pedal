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
#include "hardware/structs/rosc.h"
#include "kbd_fx/kbd_fx_tremolo.hpp"
#include "mouse_fx/mouse_fx_fuzz.hpp"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "ws2812.pio.h"

#define RANDOM_BIT (rosc_hw->randombit ? 1 : 0)
// TODO update this
#define SOFT_BOOT_BTN_GPIO 29
// TODO delete this
#define PIX_POWER_GPIO 11
// TODO update this
#define PIX_DATA_GPIO 12

#define PIX_PIO pio0
#define PIX_PIO_SM 2

#define MAX_FX 4

#define SW_MODE_MOM 0
#define SW_MODE_LATCH 1
#define SW_MODE_SET 2

MouseFuzz mouse_fuzz;
KeyboardTremolo keyboard_tremolo;
static IMouseFx* mouse_fx[] = {&mouse_fuzz};
static IKeyboardFx* keyboard_fx[] = {&keyboard_tremolo};
static uint8_t active_device_type = HID_ITF_PROTOCOL_KEYBOARD;
static bool fx_enabled = true;
static uint8_t active_fx_slot = 0;
static uint8_t active_sw_mode = SW_MODE_SET;
static bool previous_foot_sw_value = 0;
static uint32_t time_of_last_pix_update = 0;

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

void read_pot() {
  // TODO read ADC
  float percentage = 0.0;
  if (active_sw_mode != SW_MODE_SET) {
    on_fx_param_tweaked(percentage);
  } else {
    // TODO
    uint8_t slot = percentage * MAX_FX;
    if (slot != active_fx_slot) {
      mouse_fx[slot]->initialize();
      keyboard_fx[slot]->initialize();
    }
    active_fx_slot = slot;
  }
}

void read_foot_switch() {
  // TODO read gpio
  bool current_val = 0;
  if (current_val == previous_foot_sw_value) {
    return;
  }
  previous_foot_sw_value = current_val;
  if (active_sw_mode == SW_MODE_LATCH) {
    fx_enabled = !fx_enabled;
  } else if (active_sw_mode == SW_MODE_MOM) {
    fx_enabled = current_val;
  }
}

void read_toggle_switch() {
  // TODO read gpio
  uint8_t current_val = SW_MODE_SET;
  // if we're switching modes, disable the fx
  if (current_val != active_sw_mode) {
    fx_enabled = false;
  }
  active_sw_mode = current_val;
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
  // NEO_PWR
  gpio_init(PIX_POWER_GPIO);
  gpio_set_dir(PIX_POWER_GPIO, GPIO_OUT);
  gpio_put(PIX_POWER_GPIO, 1);

  uint offset = pio_add_program(PIX_PIO, &ws2812_program);

  ws2812_program_init(PIX_PIO, PIX_PIO_SM, offset, PIX_DATA_GPIO, 800000,
                      false);
}

void send_mouse_report(uint8_t buttons, int8_t x, int8_t y, int8_t wheel,
                       int8_t pan) {
  tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, pan);
}

static inline void set_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(PIX_PIO, PIX_PIO_SM, pixel_grb << 8u);
}

void led_task() {
  uint32_t t = to_ms_since_boot(get_absolute_time()) / 30;
  if (t == time_of_last_pix_update) {
    return;
  }
  time_of_last_pix_update = t;
  if (!fx_enabled) {
    set_pixel(0);
    return;
  }
  uint32_t color = 0;
  if (active_sw_mode == SW_MODE_SET) {
    if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD) {
      color = keyboard_fx[active_fx_slot]->get_indicator_color();
    } else {
      color = mouse_fx[active_fx_slot]->get_indicator_color();
    }
  } else {
    if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD) {
      color = keyboard_fx[active_fx_slot]->get_current_pixel_value(t);
    } else {
      color = mouse_fx[active_fx_slot]->get_current_pixel_value(t);
    }
  }
  set_pixel(color);
}

void io_task() {
  read_toggle_switch();
  read_foot_switch();
  read_pot();
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

  mouse_fx[active_fx_slot]->initialize();
  keyboard_fx[active_fx_slot]->initialize();

  while (1) {
    tud_task();  // tinyusb device task
    led_task();
    io_task();
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
  (void)desc_report;
  (void)desc_len;

  // Interface protocol (hid_interface_protocol_enum_t)
  const char* protocol_str[] = {"None", "Keyboard", "Mouse"};
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

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
                               hid_keyboard_report_t const* report) {
  (void)dev_addr;
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, report->modifier,
                          (uint8_t*)report->keycode);
}

// send mouse report to usb device CDC
static void process_mouse_report(uint8_t dev_addr,
                                 hid_mouse_report_t const* report) {
  (void)dev_addr;
  mouse_fx[active_fx_slot]->process_mouse_report(report);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
  (void)len;
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch (itf_protocol) {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report(dev_addr, (hid_keyboard_report_t const*)report);
      break;

    case HID_ITF_PROTOCOL_MOUSE:
      process_mouse_report(dev_addr, (hid_mouse_report_t const*)report);
      break;

    default:
      break;
  }

  // continue to request to receive report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
    log_line("Error: cannot request report");
  }
}
