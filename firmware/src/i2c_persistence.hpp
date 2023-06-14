#include <stdio.h>
#include <stdlib.h>

#include "persistence.hpp"

#ifndef HA_PERS_H
#define HA_PERS_H

typedef struct ha_settings {
  uint8_t version;
  uint8_t active_fx_slot;
  uint8_t report_parse_mode;
  float led_brightness;
  uint32_t slot_colors[4];
} settings_t;

settings_t read_settings_from_persistence();
int write_settings_to_persistence(settings_t settings);
void init_persistence();

class I2cPersistence : public IPersistence {
 private:
  settings_t delegate;
  bool enable_write = true;

 public:
  void initialize() {
    init_persistence();
    delegate = read_settings_from_persistence();
  }
  inline uint8_t getVersion() { return delegate.version; }
  inline void setActiveFxSlot(uint8_t slot) {
    delegate.active_fx_slot = slot;
    write();
  }
  inline uint8_t getActiveFxSlot() { return delegate.active_fx_slot; }
  inline void setReportParseMode(uint8_t mode) {
    delegate.report_parse_mode = mode;
    write();
  }
  inline uint8_t getReportParseMode() { return delegate.report_parse_mode; }
  inline void setLedBrightness(float brightness) {
    delegate.led_brightness = brightness;
    write();
  }
  inline float getLedBrightness() { return delegate.led_brightness; }
  void setLedColor(uint8_t slot, uint32_t color) {
    delegate.slot_colors[slot] = color;
    write();
  }
  inline uint32_t getLedColor(uint8_t slot) {
    return delegate.slot_colors[slot];
  }
  inline void write() {
    if (enable_write) write_settings_to_persistence(delegate);
  }
};

#endif
