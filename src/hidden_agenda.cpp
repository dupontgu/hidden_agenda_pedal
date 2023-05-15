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
#include "hardware/watchdog.h"
#include "kbd_fx/kbd_fx_delay.hpp"
#include "kbd_fx/kbd_fx_harmonizer.hpp"
#include "kbd_fx/kbd_fx_passthrough.hpp"
#include "kbd_fx/kbd_fx_tremolo.hpp"
#include "mouse_fx/mouse_fx_looper.hpp"
#include "mouse_fx/mouse_fx_fuzz.hpp"
#include "mouse_fx/mouse_fx_passthrough.hpp"
#include "mouse_fx/mouse_fx_reverb.hpp"
#include "mouse_fx/mouse_fx_xover.hpp"
#include "persistence.h"
#include "pico/bootrom.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pio_usb.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "util.h"
#include "ws2812.pio.h"

#define GENERIC_DESKTOP_USAGE_PAGE 0x01
#define USAGE_MOUSE 0x02
#define USAGE_KEYBOARD 0x06

#define MS_SINCE_BOOT to_ms_since_boot(get_absolute_time())
// TODO update this
#define SOFT_BOOT_BTN_GPIO 0
// TODO update this
#define PIX_DATA_GPIO 29
#define FOOT_SW_GPIO 28
// TODO update!!!!
#define TOGGLE_1_GPIO 2
#define TOGGLE_2_GPIO 1
#define KNOB_ADC_GPIO 26

#define PIX_PIO pio0
#define PIX_PIO_SM 2

#define MAX_FX 4
#define MAX_REPORT 4

#define SW_MODE_SET 0
#define SW_MODE_MOM 1
#define SW_MODE_LATCH 2

#define ADC_DEAD_ZONE 0.015f

#define LOG_BUFFER_SIZE 1024

static struct {
  uint8_t report_count;
  tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];

MouseFuzz mouse_fuzz;
MouseLooper mouse_looper;
MousePassthrough mouse_passthrough;
MouseReverb mouse_reverb;
MouseXOver mouse_xover;
KeyboardTremolo keyboard_tremolo;
KeyboardPassthrough keyboard_passthrough;
KeyboardDelay keyboard_delay;
KeyboardHarmonizer keyboard_harmonizer;
// LAST FX (at index MAX_FX) should always be passthrough! This is what gets run
// when pedal is "off"
static IMouseFx* mouse_fx[] = {&mouse_looper, &mouse_reverb, &mouse_fuzz,
                               &mouse_xover, &mouse_passthrough};
static IKeyboardFx* keyboard_fx[] = {
    &keyboard_delay, &keyboard_tremolo, &keyboard_harmonizer,
    &keyboard_passthrough, &keyboard_passthrough};
static uint8_t active_device_type = HID_ITF_PROTOCOL_KEYBOARD;
static bool fx_enabled = false;
static settings_t settings;
static uint8_t active_sw_mode = SW_MODE_SET;
static bool use_increased_dead_zone = 0;

static char log_buffer[LOG_BUFFER_SIZE];
static size_t log_write_head = 0;
static size_t log_read_head = 0;

static char cdc_read_buffer[LOG_BUFFER_SIZE];
static size_t cdc_read_head = 0;

void log_line(const char* format, ...) {
  // shouldn't happen, but throw it away to be safe
  if (log_write_head >= (LOG_BUFFER_SIZE - 255)) {
    return;
  }
  va_list args;
  va_start(args, format);
  log_write_head += vsnprintf(log_buffer + log_write_head, 253, format, args);
  log_buffer[log_write_head] = '\r';
  log_buffer[log_write_head + 1] = '\n';
  log_buffer[log_write_head + 2] = 0;
  log_buffer[log_write_head + 3] = 0;
  log_write_head += 3;

  va_end(args);
}

void flush_log() {
  if (log_write_head) {
    size_t len = strlen(log_buffer + log_read_head);
    if (len > 0) {
      tud_cdc_write(log_buffer + log_read_head, len);
      tud_cdc_write_flush();
      log_read_head += len + 1;
    } else {
      log_read_head = 0;
      log_write_head = 0;
    }
  }
}

void process_cdc_input() {
  if (cdc_read_head) {
    if (cdc_read_buffer[cdc_read_head - 1] == '\r') {
      cdc_read_buffer[cdc_read_head - 1] = 0;
      // TODO - actually process the line rather than log it
      log_line(cdc_read_buffer);
      cdc_read_head = 0;
    }
    tud_cdc_read_flush();
  }
}

void on_fx_param_tweaked(float percentage) {
  mouse_fx[settings.active_fx_slot]->update_parameter(percentage);
  keyboard_fx[settings.active_fx_slot]->update_parameter(percentage);
}

inline float read_pot() {
  // expand range so we'll def get 0 and 1 on the ends
  float adc = ((((float)adc_read() / 4096.0f) * 1.03f) - 0.015);
  // clamp to range 0..1
  adc = adc < 0.0f ? 0.0f : (adc > 1.0f ? 1.0f : adc);
  return adc;
}

void update_from_pot() {
  static float previous_adc_reading = read_pot();
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
    uint8_t active_fx_slot = settings.active_fx_slot;
    if (slot != active_fx_slot) {
      log_line("fx slot: %u", slot);
      uint32_t time_ms = MS_SINCE_BOOT;
      mouse_fx[active_fx_slot]->deinit();
      mouse_fx[slot]->initialize(time_ms, adc);
      keyboard_fx[active_fx_slot]->deinit();
      keyboard_fx[slot]->initialize(time_ms, adc);
      settings.active_fx_slot = slot;
      int written = write_settings_to_persistence(settings);
      log_line("wrote pers: %d", written);
    }
    
  }
}

void read_foot_switch() {
  static bool previous_foot_sw_value = 0;
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
  // log_line("m report %d %d %d", x, y, buttons);
  tud_hid_mouse_report(REPORT_ID_MOUSE, buttons, x, y, wheel, pan);
}

void send_keyboard_report(uint8_t modifier, uint8_t reserved,
                          const uint8_t keycode[6]) {
  (void)reserved;
  log_line("k report %u %u %u %u %u %u", keycode[0], keycode[1], keycode[2],
           keycode[3], keycode[4], keycode[5]);
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, (uint8_t*)keycode);
}

static inline void set_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(PIX_PIO, PIX_PIO_SM, pixel_grb << 8u);
}

void led_task(uint32_t time_ms) {
  static uint32_t frame_of_last_pix_update = 0;
  uint32_t frame = time_ms / 30;
  if (frame == frame_of_last_pix_update) {
    return;
  }
  frame_of_last_pix_update = frame;
  uint32_t color = 0;
  uint8_t active_fx_slot = settings.active_fx_slot;
  if (active_sw_mode == SW_MODE_SET) {
    // blink
    if (frame % 20 > 10) {
      color = 0;
    } else if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD) {
      color = keyboard_fx[active_fx_slot]->get_indicator_color();
    } else {
      color = mouse_fx[active_fx_slot]->get_indicator_color();
    }
  } else if (fx_enabled) {
    if (active_device_type == HID_ITF_PROTOCOL_KEYBOARD) {
      color = keyboard_fx[active_fx_slot]->get_current_pixel_value(time_ms);
    } else {
      color = mouse_fx[active_fx_slot]->get_current_pixel_value(time_ms);
    }
  }
  set_pixel(color);
}

void io_task(uint32_t time_ms) {
  static uint32_t frame_of_last_io_update = 0;
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
    mouse_fx[settings.active_fx_slot]->tick(time_ms);
    keyboard_fx[settings.active_fx_slot]->tick(time_ms);
  }
}

void init_settings() {
  init_persistence();
  settings = read_settings_from_persistence();
  for (size_t i = 0; i < MAX_FX; i++) {
    uint32_t color = settings.slot_colors[i];
    mouse_fx[i]->set_indicator_color(color);
    keyboard_fx[i]->set_indicator_color(color);
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
  // init device stack on configured roothub port
  set_sys_clock_khz(120000, true);

  init_soft_boot();
  watchdog_enable(300, 1);
  multicore_reset_core1();
  // all USB task run in core1
  multicore_launch_core1(core1_main);

  tud_init(BOARD_TUD_RHPORT);
  init_pix();
  init_io();
  init_settings();

  float param_value = read_pot();
  uint32_t now = MS_SINCE_BOOT;
  mouse_fx[settings.active_fx_slot]->initialize(now, param_value);
  keyboard_fx[settings.active_fx_slot]->initialize(now, param_value);

  while (1) {
    uint32_t time_ms = MS_SINCE_BOOT;
    led_task(time_ms);
    io_task(time_ms);
    fx_task(time_ms);
    flush_log();
    tud_task();  // tinyusb device task
    process_cdc_input();
    watchdog_update();
  }

  return 0;
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
  log_line("%u reports", hid_info[instance].report_count);
  for (size_t i = 0; i < hid_info[instance].report_count; i++) {
    tuh_hid_report_info_t r = hid_info[instance].report_info[i];
    log_line("id: %u, usage: %u", r.report_id, r.usage);
  }

  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  log_line("[%04x:%04x][%u] HID%u, proto=%s", vid, pid, dev_addr, instance,
           protocol_str[itf_protocol]);

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
  log_line("[%u] HID%u unmounted", dev_addr, instance);
}

static void process_kbd_report(uint8_t dev_addr,
                               hid_keyboard_report_t const* report,
                               uint32_t time_ms) {
  (void)dev_addr;
  active_device_type = HID_ITF_PROTOCOL_KEYBOARD;
  uint8_t slot = fx_enabled ? settings.active_fx_slot : MAX_FX;
  keyboard_fx[slot]->process_keyboard_report(report, time_ms);
}

// send mouse report to usb device CDC
static void process_mouse_report(uint8_t dev_addr,
                                 hid_mouse_report_t const* report,
                                 uint32_t time_ms) {
  (void)dev_addr;
  active_device_type = HID_ITF_PROTOCOL_MOUSE;
  uint8_t slot = fx_enabled ? settings.active_fx_slot : MAX_FX;
  mouse_fx[slot]->process_mouse_report(report, time_ms);
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
  (void)len;
  uint8_t itf_protocol = HID_ITF_PROTOCOL_NONE;
  uint8_t const* report_offset = report;

  if (hid_info[instance].report_count > 2) {
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

void tud_cdc_rx_cb(uint8_t itf) {
  (void)itf;
  if (tud_cdc_available()) {
    cdc_read_head += tud_cdc_read(cdc_read_buffer + cdc_read_head,
                                  LOG_BUFFER_SIZE - cdc_read_head);
  }
}
