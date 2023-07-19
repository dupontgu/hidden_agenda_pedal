#include "persistence.hpp"

class InMemoryPersistence : public IPersistence {
 public:
  InMemoryPersistence() {}
  void initialize() {}
  uint8_t getVersion() { return 1; }
  void setActiveFxSlot(uint8_t slot) { active_slot = slot; }
  uint8_t getActiveFxSlot() { return active_slot; }
  void setReportParseMode(uint8_t mode) { report_mode = mode; }
  uint8_t getReportParseMode() { return report_mode; }
  void setLedBrightness(float brightness) { led_brightness = brightness; }
  float getLedBrightness() { return led_brightness; }
  void setLedColor(uint8_t slot, uint32_t color) { slots[slot] = color; }
  uint32_t getLedColor(uint8_t slot) { return slots[slot]; }
  void setRawHidLogsEnabled(bool enabled) { raw_hid_logs_enabled = enabled; }
  bool getRawHidLogsEnabled() { return raw_hid_logs_enabled; }

 private:
  uint8_t active_slot;
  uint8_t report_mode;
  bool raw_hid_logs_enabled;
  float led_brightness;
  uint32_t slots[4] = {0, 0, 0, 0};
};