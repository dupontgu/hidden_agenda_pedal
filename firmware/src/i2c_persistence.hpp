#include <stdio.h>
#include <stdlib.h>

#include "persistence.hpp"

#ifndef HA_PERS_H
#define HA_PERS_H
#define FLAG_RAW_LOGS 0b1
#define FLAG_FLASHING_ENABLED 0b10
#define FLAG_INVERT_FOOTSWITCH 0b100

typedef struct ha_settings {
  // VERSION ALWAYS FIRST!!
  uint8_t version;
  uint8_t active_fx_slot;
  uint8_t report_parse_mode;
  uint8_t flags;
  float led_brightness;
  uint32_t slot_colors[4];
} settings_t;

settings_t read_settings_from_persistence();
settings_t get_defaults();
int write_settings_to_persistence(settings_t settings);
void init_persistence();

class I2cPersistence : public IPersistence {
 private:
  settings_t delegate;
  bool enable_write = true;
  inline void set_bit_flag(bool enabled, uint8_t flag) {
    delegate.flags = enabled ? delegate.flags | flag
                             : delegate.flags & ~flag;
  }

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
  inline void setRawHidLogsEnabled(bool enabled) {
    set_bit_flag(enabled, FLAG_RAW_LOGS);
    write();
  }
  inline bool areRawHidLogsEnabled() { return delegate.flags & FLAG_RAW_LOGS; }
  inline void setFlashingEnabled(bool enabled) {
    set_bit_flag(enabled, FLAG_FLASHING_ENABLED);
    write();
  }
  inline bool isFlashingEnabled() { return delegate.flags & FLAG_FLASHING_ENABLED; }
  void setLedColor(uint8_t slot, uint32_t color) {
    delegate.slot_colors[slot] = color;
    write();
  }
  inline void setShouldInvertFootswitch(bool invert) {
    set_bit_flag(invert, FLAG_INVERT_FOOTSWITCH);
    write();
  }
  inline bool shouldInvertFootswitch() { return delegate.flags & FLAG_INVERT_FOOTSWITCH; }
  inline uint32_t getLedColor(uint8_t slot) {
    return delegate.slot_colors[slot];
  }
  void resetToDefaults() {
    delegate = get_defaults();
    write();
  }
  inline void write() {
    if (enable_write) write_settings_to_persistence(delegate);
  }
};

#endif
