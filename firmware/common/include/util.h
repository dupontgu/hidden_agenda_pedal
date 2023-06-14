#include <stdint.h>

#ifndef UTIL_H
#define UTIL_H
void log_line(const char *format, ...);
const char *get_serial_number();
uint8_t get_random_byte();
void reboot_to_uf2(unsigned int gpio, uint32_t events);

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(g) << 8) | ((uint32_t)(r) << 16) | (uint32_t)(b);
}

static inline uint32_t color_at_brightness(uint32_t color, float brightness) {
  uint8_t r = (uint8_t)((float)(uint8_t)(color >> 16) * brightness);
  uint8_t g = (uint8_t)((float)(uint8_t)((color & 0xFF00) >> 8) * brightness);
  uint8_t b = (uint8_t)((float)(uint8_t)((color & 0x00FF)) * brightness);
  return urgb_u32(r, g, b);
}
#endif