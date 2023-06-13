#include <stdlib.h>
#include <stdio.h>

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

// Singleton instance for now
extern settings_t active_settings;

#endif
