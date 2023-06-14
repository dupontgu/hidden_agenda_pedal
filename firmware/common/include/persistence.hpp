#include <stdint.h>

#ifndef COMMON_PERSISTENCE
#define COMMON_PERSISTENCE
class IPersistence {
 public:
  virtual void initialize() = 0;
  virtual uint8_t getVersion() = 0;
  virtual void setActiveFxSlot(uint8_t slot) = 0;
  virtual uint8_t getActiveFxSlot() = 0;
  virtual void setReportParseMode(uint8_t mode) = 0;
  virtual uint8_t getReportParseMode() = 0;
  virtual void setLedBrightness(float slot) = 0;
  virtual float getLedBrightness() = 0;
  virtual void setLedColor(uint8_t slot, uint32_t color) = 0;
  virtual uint32_t getLedColor(uint8_t slot) = 0;
  virtual ~IPersistence() = default;
};
#endif