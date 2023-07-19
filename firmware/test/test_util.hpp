#include <cstdarg>
#include <iostream>
#include <string>
#include <vector>

#include "util.h"

static std::vector<std::string> log_collection;
static uint16_t reboot_count = 0;

void log_line(const char *format, ...) {
  static char buf[512];
  va_list args;
  va_start(args, format);
  int size = vsnprintf(buf, 512, format, args);
  buf[size] = 0;
  log_collection.push_back(std::string(buf));
  va_end(args);
}

void refresh_settings() {}

const char *get_serial_number() { return "abc"; }

uint8_t get_random_byte() { return 0b1010; }

void reboot_to_uf2(unsigned int gpio, uint32_t events) { reboot_count++; }

void dump_logs() {
  std::cout << "LOGS:" << std::endl;
  size_t lc = log_collection.size();
  for (size_t i = 0; i < lc; i++) {
    std::cout << "  " << log_collection[i] << std::endl;
  }
}

void reset() {
  log_collection.clear();
  reboot_count = 0;
}