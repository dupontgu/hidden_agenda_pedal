#include <stdint.h>

#ifndef UTIL_H
#define UTIL_H
void log_line(const char *format, ...);
const char *get_serial_number();
uint8_t get_random_byte();
void reboot_to_uf2(unsigned int gpio, uint32_t events);
#endif